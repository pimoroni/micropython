# fatlfs — editable FAT drive over littlefs

`fatlfs` exposes a board's littlefs storage to a USB host as an editable FAT16
drive, so files can be copied and edited over USB while the on-device filesystem
stays a single resilient littlefs volume. There is no real FAT partition: the FAT
is synthesized on read, and host writes are staged in RAM and committed to
littlefs at flush barriers (SCSI SYNCHRONIZE CACHE, write-idle pauses, eject) in
one reconcile pass. Files are keyed by their FAT first-cluster, so rename/move
preserve data instead of copying it. A file larger than the staging cap spills
its tail clusters to a temporary littlefs file.

* Portable core: `extmod/fatlfs/` (depends only on littlefs + a fixed arena).
* rp2 glue: `ports/rp2/fatlfs_rp2.c` (USB-MSC + module) and `fatlfs_boot.c`
  (double-tap reset detection). The core routes all allocation through
  `fatlfs_arena.c` (`-DFATLFS_ARENA_ALLOC`), a fixed arena carved from the GC
  heap, so it never fragments the MicroPython heap.

## Using it (rp2 / Raspberry Pi Pico 2)

Build the opt-in board variant:

```
make BOARD=RPI_PICO2 BOARD_VARIANT=FATLFS
```

This sets `MICROPY_FATLFS=1` (which implies `MICROPY_HW_USB_MSC`). A stock build
is unchanged; `MICROPY_FATLFS` defaults to 0.

MSC runs as a **dedicated reboot mode**, never alongside a running app (see SRAM
below). Enter it from the REPL:

```python
import fatlfs
fatlfs.reboot_msc()   # warm-reboots straight into the USB drive
```

or by **double-tapping reset** within ~1 s (needs a RUN→GND button; a bare Pico 2
has none, and a full power-cycle won't work — the flag lives in the always-on
power domain and is lost when power is removed).

The drive mounts as a FAT16 volume; on eject the board commits staged writes to
littlefs and reboots to normal mode. `fatlfs.msc_requested()` reports whether
this boot was selected for MSC; `fatlfs.msc_mode()` enters the service loop in
place (for testing).

## Footprint

**Build size (enabling the variant vs stock RPI_PICO2):** ~16.5 KB flash, of
which ~10.5 KB is fatlfs itself (`fatlfs.c` ~9 KB) and ~6 KB is the USB-MSC class
it pulls in. Static RAM grows ~7 KB — almost entirely the 4 KB MSC endpoint
buffer plus the deeper USB task queue; fatlfs's own static state is ~24 bytes.

**Runtime SRAM** is a single arena taken from the GC heap only while MSC mode is
active, and freed by the reboot on exit. The default Pico 2 config is a 320 KB
arena for a 3 MB / 512 B-cluster (6144-cluster) volume:

| Part | Size | Notes |
|---|---|---|
| Copy buffer (staging) | **96 KB** | `FATLFS_STAGING_MAX`; host writes in flight before spill/commit |
| Fixed per-cluster metadata | **~168 KB** | 28 B × clusters; biggest item is `owner[]` at 48 KB |
| Root dir + lfs caches | ~12 KB | 256 root entries (8 KB) + 4 file caches (4 KB) |
| Headroom | ~44 KB | committed snapshot (~0.5 KB per file/dir) + live dir tables |

So of the arena, the copy buffer is ~96 KB and the fixed metadata is ~180 KB;
the rest is per-file scratch that grows with the number of files.

## Largest filesystem in RP2350 SRAM

The binding cost is the per-cluster metadata, a flat **28 bytes per cluster**
(`fat`+`staging`+`dir_bytes`+`owner`+`spillhead`+`prev`), independent of cluster
size. The RP2350 has 520 KB SRAM; in dedicated MSC mode the ~460 KB GC heap is
nearly free. Budgeting ~400 KB for the arena (minimal staging + overhead) leaves
room for roughly **13,000 clusters**. Volume = clusters × cluster size:

* **512 B clusters** (best small-file packing, used by the variant): SRAM-bound
  to a **~6–7 MB** volume.
* **4 KB clusters**: 28 B covers 4 KB of volume, so the same budget spans tens of
  MB — SRAM is no longer the limit. A 16 MB flash needs only ~115 KB of metadata
  (4096 clusters); the ceiling becomes FAT16's 65524-cluster max and the flash
  size, not RAM. The trade-off is wasted copy buffer space per small file.

Tunables (`ports/rp2/fatlfs_rp2.c`): `FATLFS_ARENA_BYTES`, `FATLFS_STAGING_MAX`,
`FATLFS_ROOT_ENTRIES`, and the `cluster_size` / `cluster_count` in `expose_real`.

## Constraints

* FAT16 only (4085–65524 clusters). 512 B clusters keep a ~3 MB volume above the
  FAT12 boundary; FAT12 is not implemented.
* Dedicated MSC mode only on low-RAM boards — it cannot coexist with a running
  app that also needs the heap.
* The staging commit happens off the USB callback; `CFG_TUD_TASK_QUEUE_SZ` must
  be deep (256) or the USB event queue overflows during flash commits.
* OS "trash" files (`._*`, `.DS_Store`, `Thumbs.db`, ...) are dropped, not stored.
