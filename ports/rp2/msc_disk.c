/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include "tusb.h"
#if CFG_TUD_MSC
#include "device/usbd_pvt.h"
#include "mpconfigboard.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "modrp2.h"

// This implementation does Not support Flash sector caching.
#if MICROPY_FATFS_MAX_SS != FLASH_SECTOR_SIZE
#error MICROPY_FATFS_MAX_SS must be the same size as FLASH_SECTOR_SIZE
#endif

// The exposed region defaults to the whole user storage partition. A board can
// expose a different flash region by defining both of these in mpconfigboard.h,
// in which case it is also responsible for its own filesystem mounts at boot.
#ifndef MICROPY_HW_USB_MSC_FLASH_OFFSET
#define MICROPY_HW_USB_MSC_FLASH_OFFSET (PICO_FLASH_SIZE_BYTES - MICROPY_HW_FLASH_STORAGE_BYTES)
#endif
#ifndef MICROPY_HW_USB_MSC_FLASH_BYTES
#define MICROPY_HW_USB_MSC_FLASH_BYTES  (MICROPY_HW_FLASH_STORAGE_BYTES)
#endif

#define BLOCK_SIZE          (FLASH_SECTOR_SIZE)
#define BLOCK_COUNT         (MICROPY_HW_USB_MSC_FLASH_BYTES / BLOCK_SIZE)
#define FLASH_BASE_ADDR     (MICROPY_HW_USB_MSC_FLASH_OFFSET)
#define FLASH_MMAP_ADDR     (XIP_BASE + FLASH_BASE_ADDR)

#define WRITE_BUSY_STATUS_TIMEOUT 1000000llu

static bool ejected = false;
static bool ready = false;
static volatile bool eject_event = false;
static absolute_time_t last_write = 0;

// Counts the host's writes, moving just before each changes the flash, so a reader that sees it
// unchanged across a read knows nothing was written beneath it. A held write counts once it
// goes. Wraps, so only compare it.
static volatile uint32_t write_count = 0;

// Set when the media is re-presented, and reported once as unit attention 28h
// (not-ready-to-ready change) so the host discards its cached view of the disk.
static bool attention = false;
static bool presented_before = false;

// Check the media is present and no unit attention is pending, setting the
// sense data otherwise.
static bool media_available(uint8_t lun) {
    if (ejected || !ready) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    if (attention) {
        attention = false;
        tud_msc_set_sense(lun, SCSI_SENSE_UNIT_ATTENTION, 0x28, 0x00);
        return false;
    }
    return true;
}

bool rp2_tud_set_msc_ready() {
    if(ready) {
        return false;
    }
    ejected = false;
    eject_event = false;
    ready = true;
    attention = presented_before;
    presented_before = true;
    return true;
}

bool rp2_tud_set_msc_unready() {
    if(!ready) {
        return false;
    }
    ready = false;
    return true;
}

// Reports a host eject since the media was last presented. The host owns the
// decision to eject; what happens next (remount, reboot) is Python's policy.
bool rp2_tud_msc_ejected() {
    return eject_event;
}

bool rp2_tud_is_msc_busy() {
    if(last_write == 0) return false;
    return  absolute_time_diff_us(last_write, get_absolute_time()) < WRITE_BUSY_STATUS_TIMEOUT;
}

uint32_t rp2_tud_msc_write_count(void) {
    return write_count;
}

// The media as the XIP window maps it, for reads that bypass the filesystem
const uint8_t *rp2_tud_msc_media(size_t *bytes) {
    *bytes = MICROPY_HW_USB_MSC_FLASH_BYTES;
    return (const uint8_t *)FLASH_MMAP_ADDR;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    memcpy(vendor_id, MICROPY_HW_USB_MSC_INQUIRY_VENDOR_STRING, MIN(strlen(MICROPY_HW_USB_MSC_INQUIRY_VENDOR_STRING), 8));
    memcpy(product_id, MICROPY_HW_USB_MSC_INQUIRY_PRODUCT_STRING, MIN(strlen(MICROPY_HW_USB_MSC_INQUIRY_PRODUCT_STRING), 16));
    memcpy(product_rev, MICROPY_HW_USB_MSC_INQUIRY_REVISION_STRING, MIN(strlen(MICROPY_HW_USB_MSC_INQUIRY_REVISION_STRING), 4));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return media_available(lun);
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    *block_size = BLOCK_SIZE;
    *block_count = BLOCK_COUNT;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (load_eject) {
        if (start) {
            // load disk storage
            ejected = false;
        } else {
            // unload disk storage
            ejected = true;
            ready = false;
            eject_event = true;
        }
    }
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    // Refuse while the device holds the media, so the host doesn't cache stale data.
    if (!media_available(lun)) {
        return -1;
    }
    uint32_t count = bufsize / BLOCK_SIZE;
    memcpy(buffer, (void *)(FLASH_MMAP_ADDR + lba * BLOCK_SIZE), count * BLOCK_SIZE);
    return count * BLOCK_SIZE;
}

static int32_t write_blocks(uint32_t lba, uint8_t *buffer, uint32_t bufsize) {
    uint32_t count = bufsize / BLOCK_SIZE;
    write_count++;
    // The port's own section, not just the interrupts: it also suspends the other
    // core, commits dirty PSRAM writes before the XIP cache is invalidated, and puts
    // the flash timing back afterwards, all of which a write from here needs too
    uint32_t state = begin_critical_flash_section();
    flash_range_erase(FLASH_BASE_ADDR + lba * BLOCK_SIZE, count * BLOCK_SIZE);
    flash_range_program(FLASH_BASE_ADDR + lba * BLOCK_SIZE, buffer, count * BLOCK_SIZE);
    end_critical_flash_section(state);
    return count * BLOCK_SIZE;
}

// The first write after an idle spell can be held for up to write_hold_us, so the board
// can quieten what a flash write disturbs before the writes begin: each leaves the flash
// unreadable, with interrupts off, for tens of milliseconds. It goes early on
// rp2.release_msc_writes(). The held buffer is TinyUSB's, kept until the write is done.
static uint32_t write_hold_us = 0;
static volatile bool write_held = false;
static uint8_t held_lun;
static uint32_t held_lba;
static uint8_t *held_buffer;
static uint32_t held_bufsize;
static alarm_id_t held_alarm;

// Run in the USB task, from the alarm or a release, whichever comes first
static void write_held_blocks(void *param) {
    (void)param;
    if (!write_held) {
        return;
    }
    write_held = false;
    cancel_alarm(held_alarm);
    // Taken back by the board meanwhile, so the write fails as one arriving now would
    int32_t result = media_available(held_lun)
        ? write_blocks(held_lba, held_buffer, held_bufsize) : TUD_MSC_RET_ERROR;
    last_write = get_absolute_time();
    tud_msc_async_io_done(result, false);
}

static int64_t write_hold_expired(alarm_id_t id, void *user_data) {
    usbd_defer_func(write_held_blocks, NULL, true);
    return 0;
}

uint32_t rp2_tud_hold_msc_writes(uint32_t hold_us) {
    uint32_t previous = write_hold_us;
    write_hold_us = hold_us;
    return previous;
}

void rp2_tud_release_msc_writes(void) {
    if (write_held) {
        usbd_defer_func(write_held_blocks, NULL, false);
    }
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    // Refuse while the device holds the media, which may be modifying the filesystem.
    if (!media_available(lun)) {
        return -1;
    }
    // Checked before last_write moves, which is what makes the board busy at once
    bool idle = !rp2_tud_is_msc_busy();
    last_write = get_absolute_time();
    if (write_hold_us && idle && !write_held) {
        held_lun = lun;
        held_lba = lba;
        held_buffer = buffer;
        held_bufsize = bufsize;
        write_held = true;
        // Negative is no alarm free, in which case the write goes now
        held_alarm = add_alarm_in_us(write_hold_us, write_hold_expired, NULL, true);
        if (held_alarm >= 0) {
            return TUD_MSC_RET_ASYNC;
        }
        write_held = false;
    }
    return write_blocks(lba, buffer, bufsize);
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    int32_t resplen = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Sync the logical unit if needed.
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }
    return resplen;
}
#endif
