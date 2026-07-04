/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Copyright (c) 2026 Philip Howard
 */

// rp2 glue for fatlfs: expose the board's littlefs storage to a USB host as a
// FAT16 drive so files can be copied/edited over USB while the on-device
// filesystem stays littlefs (one resilient, unified partition). FAT16 so the
// advertised volume matches the real ~3 MB partition (FAT32's cluster minimum
// would force >= 32 MB); an oversized copy fails upfront in the host's UI.
//
// This is the MicroPython module glue + the C API that ports/rp2/msc_disk.c
// calls to serve the live USB-MSC drive. The portable FAT16<->littlefs core is
// in extmod/fatlfs/; the embedded arena allocator is fatlfs_arena.c.
//
// On this port MSC runs as a *dedicated* reboot mode (entered via a double-tap
// reset or fatlfs.reboot_msc()), never coexisting with a running Python app:
// fatlfs's fixed per-cluster arrays plus a staging buffer are ~300 KB, which
// only fits the RP2350's ~460 KB GC heap when the heap is otherwise idle. Host
// writes are STAGED in RAM inside the USB callback and committed to littlefs at
// flush barriers (SYNCHRONIZE CACHE, write-idle pauses, eject) in one reconcile
// pass; files larger than the staging cap spill their tails to flash.

#include <string.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/vfs.h"
#include "lib/littlefs/lfs2.h"
#include "fatlfs.h"
#include "fatlfs_arena.h"

// FAT volume label shown for the drive; overridable per board in mpconfigboard.h.
#ifndef MICROPY_HW_FATLFS_LABEL
#define MICROPY_HW_FATLFS_LABEL "PICO"
#endif

// Memory budget for a plain Pico 2 (3 MB littlefs, no PSRAM). The arena is carved
// from the GC heap via m_new, so it must fit alongside the MicroPython runtime in
// dedicated MSC mode (~460 KB heap, otherwise idle). The fixed FAT-mirror arrays
// for a 3 MB / 512 B-cluster (6144-cluster) volume are ~185 KB; the rest is
// staging (in-RAM host writes before they spill/commit) plus reconcile scratch.
// A file larger than the staging cap spills its tail clusters to flash. These are
// the levers to tune if RAM is tight or copies of large files are common.
#define FATLFS_ARENA_BYTES   (320u * 1024)
#define FATLFS_STAGING_MAX   (96u * 1024)     // in-RAM before spill-to-flash
#define FATLFS_ROOT_ENTRIES  256              // fixed FAT16 root dir capacity

// TinyUSB / rp2 glue, forward-declared (tusb.h isn't on the qstr-scan path).
extern void mp_usbd_task(void);
extern void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms);

// Double-tap boot selection (ports/rp2/fatlfs_boot.c).
extern bool fatlfs_msc_requested(void);
extern void fatlfs_request_msc_reboot(void);

// The mounted VfsLfs2 object layout (mirrors extmod/vfs_lfs.c's
// mp_obj_vfs_lfs2_t) so we can reach the live lfs2_t of the flash filesystem.
extern const mp_obj_type_t mp_type_vfs_lfs2;
typedef struct {
    mp_obj_base_t base;
    mp_vfs_blockdev_t blockdev;
    bool enable_mtime;
    vstr_t cur_dir;
    struct lfs2_config config;
    lfs2_t lfs;
} fatlfs_lfs2_vfs_t;

static fatlfs_t *v;                       // the mounted shim (in the arena)
static bool setup_done;
static bool readonly_mode = true;
static volatile bool host_ejected;
static volatile uint32_t last_write_ms;   // last host WRITE10 (copy-to/edit)
static volatile bool flush_requested;     // host issued SYNCHRONIZE CACHE
static uint32_t last_flush_ms;            // last incremental commit
static bool spilled_session;              // a large file spilled: defer commits to eject
static bool flush_failed;                 // an incremental commit failed; stop retry churn
static bool write_since_flush;            // host wrote since the last commit attempt

// Incremental commit pacing. macOS bulk copies can stream with no gaps and never
// send SYNCHRONIZE CACHE, so idle/barrier flushing alone leaves everything for
// one long freeze at eject. So we also flush periodically during an active copy.
// This is safe because reconcile preserves in-flight orphan data (a file whose
// data is written but whose directory entry isn't yet). FLUSH_IDLE_MS is the
// quiet-gap trigger; FLUSH_PERIOD_MS caps how long staged writes sit in a gapless copy.
#define FLUSH_IDLE_MS   300
#define FLUSH_PERIOD_MS 750

// ---- C API consumed by ports/rp2/msc_disk.c (the live USB-MSC callbacks) ----
bool fatlfs_active(void) {
    return setup_done && !host_ejected;
}
bool fatlfs_readonly(void) {
    return readonly_mode;
}
void fatlfs_msc_capacity(uint32_t *block_count, uint16_t *block_size) {
    if (setup_done) {
        *block_count = (uint32_t)fatlfs_block_count(v);
        *block_size = (uint16_t)fatlfs_block_size(v);
    } else {
        *block_count = 0;
        *block_size = 512;
    }
}
int32_t fatlfs_msc_read(uint32_t lba, uint32_t off, void *buf, uint32_t len) {
    if (!setup_done) {
        return -1;
    }
    uint32_t sec = lba + off / 512;         // off is a byte offset within the xfer
    uint32_t n = len / 512;
    if (fatlfs_read(v, sec, n, buf) != FATLFS_OK) {
        return -1;
    }
    return (int32_t)(n * 512);
}
// Stage only - the MSC callback must NOT touch flash. An lfs2 commit is many
// flash ops with IRQs masked; over a bulk burst that starves the USB task and
// overflows its event queue. Commit happens from the msc loop at eject.
int32_t fatlfs_msc_write(uint32_t lba, uint32_t off, uint8_t *buf, uint32_t len) {
    if (!setup_done) {
        return -1;
    }
    last_write_ms = mp_hal_ticks_ms();
    write_since_flush = true;
    uint32_t sec = lba + off / 512;
    uint32_t n = len / 512;
    if (fatlfs_write(v, sec, n, buf) != FATLFS_OK) {
        return -1;
    }
    return (int32_t)(n * 512);
}
// Host issued START STOP UNIT (eject). Do NOT reboot here (staged writes aren't
// committed yet); the msc loop notices, flushes to flash, then reboots.
void fatlfs_msc_eject(void) {
    host_ejected = true;
}
// Host issued SYNCHRONIZE CACHE - a consistency barrier where staged writes are
// safe to commit. Record it; the service loop does the flash-touching flush.
void fatlfs_msc_sync(void) {
    flush_requested = true;
}

// ---- set up the FAT16 view over the mounted flash littlefs ----
static void expose_real(bool rw) {
    // A stale v from a previous expose must not be reachable from the MSC
    // callbacks while we re-init the arena underneath it.
    setup_done = false;
    readonly_mode = !rw;
    host_ejected = false;

    lfs2_t *real_lfs = NULL;
    for (mp_vfs_mount_t *m = MP_STATE_VM(vfs_mount_table); m != NULL; m = m->next) {
        if (mp_obj_get_type(m->obj) == &mp_type_vfs_lfs2) {
            fatlfs_lfs2_vfs_t *vp = MP_OBJ_TO_PTR(m->obj);
            real_lfs = &vp->lfs;
            break;
        }
    }
    if (real_lfs == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("no littlefs mounted"));
    }

    // One big buffer, kept alive by a GC root, carved up by the arena allocator
    // that the fatlfs core's malloc/free/calloc/realloc route to.
    uint8_t *arena = MP_STATE_PORT(fatlfs_arena_buf);
    if (arena == NULL) {
        arena = m_new(uint8_t, FATLFS_ARENA_BYTES);
        MP_STATE_PORT(fatlfs_arena_buf) = arena;
    }
    fatlfs_arena_init(arena, FATLFS_ARENA_BYTES);
    v = NULL;

    // Honest volume sizing: advertise the real littlefs partition capacity so a
    // too-big copy fails upfront in the host's UI instead of silently at eject.
    uint32_t part_bytes = 0;
    if (real_lfs->cfg && real_lfs->cfg->block_count) {
        part_bytes = real_lfs->cfg->block_count * real_lfs->cfg->block_size;
    }

    fatlfs_config_t cfg = {
        .lfs = real_lfs,
        // 512 B clusters keep the honest cluster count for a 3 MB volume inside
        // FAT16's [4085, 65524] window (6144 clusters); larger clusters would
        // drop below 4085 and force FAT12, which the core doesn't implement.
        .cluster_size = 512,
        .cluster_count = part_bytes / 512,   // ~real capacity; mount clamps to FAT16 window
        .root_entries = FATLFS_ROOT_ENTRIES,
        .staging_max_bytes = FATLFS_STAGING_MAX,
        // Unique serial per expose so the host doesn't serve a stale cached mount.
        .volume_id = 0x1337c0deu ^ mp_hal_ticks_ms(),
        .keep_os_trash = 0,            // drop ._ / .DS_Store instead of storing them
    };
    memset(cfg.volume_label, ' ', sizeof(cfg.volume_label));
    const char *lbl = MICROPY_HW_FATLFS_LABEL;
    for (int i = 0; i < 11 && lbl[i]; i++) {
        cfg.volume_label[i] = lbl[i];
    }

    int rc = fatlfs_mount(&cfg, &v);
    if (rc != FATLFS_OK || v == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("fatlfs mount failed"));
    }
    fatlfs_defer_spill(v, 1);          // never spill (flash) from the USB callback

    last_write_ms = 0;
    last_flush_ms = mp_hal_ticks_ms();
    spilled_session = false;
    flush_failed = false;
    write_since_flush = false;
    setup_done = true;
}

// Dedicated blocking USB-MSC service loop. Presents the littlefs as a USB drive,
// stages host writes, commits them to flash incrementally and at eject, then
// warm-reboots back to normal mode. Never returns.
void fatlfs_run_msc_mode(void) {
    expose_real(true);

    for (;;) {
        // Pump USB (stage host writes) and relieve RAM pressure. A file larger
        // than the staging cap must spill to flash; each spilled cluster masks
        // IRQs, so we bound the spill and pump USB fewer times while over the cap,
        // so the host's writes NAK and throttle - natural backpressure that lets
        // the spill keep up instead of ballooning the buffer to an out-of-arena
        // write failure (which the host would retry forever). Under the cap we
        // pump freely but stop the instant we cross it, bounding overshoot.
        if (fatlfs_staged_bytes(v) > fatlfs_staging_max(v)) {
            fatlfs_spill(v, 64);         // drain ~32 KB to flash
            mp_usbd_task();              // then accept a little (throttled)
            mp_usbd_task();
            spilled_session = true;
        } else {
            for (int i = 0; i < 64; i++) {
                mp_usbd_task();
                if (fatlfs_staged_bytes(v) > fatlfs_staging_max(v)) {
                    break;
                }
            }
        }

        // Incremental commit for the common case (small files): flush at a
        // SYNCHRONIZE CACHE barrier, a write-idle pause, or periodically through a
        // gapless copy. Keeps the eject flush short. Once anything has SPILLED we
        // stop until eject: reconciling a large file that's still being written
        // churns its spill temp, and a write-idle pause can't be told from a
        // mid-file gap. A failed commit stops incremental retries; the eject flush
        // is the one retry that matters. idle/periodic re-arm only on new writes.
        uint32_t now = mp_hal_ticks_ms();
        bool idle = write_since_flush && last_write_ms && (uint32_t)(now - last_write_ms) > FLUSH_IDLE_MS;
        bool periodic = write_since_flush && last_flush_ms && (uint32_t)(now - last_flush_ms) > FLUSH_PERIOD_MS;
        if (!host_ejected && !spilled_session && !flush_failed && fatlfs_dirty(v) &&
            (flush_requested || idle || periodic)) {
            flush_requested = false;
            write_since_flush = false;
            if (fatlfs_flush(v) != FATLFS_OK) {
                flush_failed = true;
            }
            last_flush_ms = mp_hal_ticks_ms();
        } else if (flush_requested) {
            flush_requested = false;   // nothing staged; barrier already satisfied
        }

        if (host_ejected) {
            fatlfs_flush(v);                            // commit every staged write
            uint32_t t = mp_hal_ticks_ms();             // let host finish its unmount
            while ((uint32_t)(mp_hal_ticks_ms() - t) < 200) {
                mp_usbd_task();
            }
            watchdog_reboot(0, 0, 0);                   // clean reboot -> normal mode
            for (;;) {
            }
        }
    }
}

// ---- Python surface (module `fatlfs`) ----

// fatlfs.reboot_msc(): reboot straight into dedicated USB-MSC mode. Does not
// return (the board warm-reboots and comes back up serving the drive).
static mp_obj_t fl_reboot_msc(void) {
    fatlfs_request_msc_reboot();
    return mp_const_none;   // unreachable
}
static MP_DEFINE_CONST_FUN_OBJ_0(fl_reboot_msc_obj, fl_reboot_msc);

// fatlfs.msc_requested(): True if this boot was selected for MSC mode (double-tap
// reset or a prior reboot_msc()). The boot path uses this to enter msc_mode().
static mp_obj_t fl_msc_requested(void) {
    return mp_obj_new_bool(fatlfs_msc_requested());
}
static MP_DEFINE_CONST_FUN_OBJ_0(fl_msc_requested_obj, fl_msc_requested);

// fatlfs.msc_mode(): enter the dedicated MSC service loop in place (blocking;
// reboots to normal mode on eject). Mainly for testing - production entry is the
// double-tap / reboot_msc() path handled from the boot sequence.
static mp_obj_t fl_msc_mode(void) {
    fatlfs_run_msc_mode();
    return mp_const_none;   // unreachable
}
static MP_DEFINE_CONST_FUN_OBJ_0(fl_msc_mode_obj, fl_msc_mode);

static const mp_rom_map_elem_t fatlfs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fatlfs) },
    { MP_ROM_QSTR(MP_QSTR_reboot_msc), MP_ROM_PTR(&fl_reboot_msc_obj) },
    { MP_ROM_QSTR(MP_QSTR_msc_requested), MP_ROM_PTR(&fl_msc_requested_obj) },
    { MP_ROM_QSTR(MP_QSTR_msc_mode), MP_ROM_PTR(&fl_msc_mode_obj) },
};
static MP_DEFINE_CONST_DICT(fatlfs_globals, fatlfs_globals_table);

const mp_obj_module_t fatlfs_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&fatlfs_globals,
};
MP_REGISTER_MODULE(MP_QSTR_fatlfs, fatlfs_module);
MP_REGISTER_ROOT_POINTER(void *fatlfs_arena_buf);
