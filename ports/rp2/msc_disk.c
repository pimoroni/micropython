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
#include "mpconfigboard.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

// This implementation does Not support Flash sector caching.
#if MICROPY_FATFS_MAX_SS != FLASH_SECTOR_SIZE
#error MICROPY_FATFS_MAX_SS must be the same size as FLASH_SECTOR_SIZE
#endif

#define BLOCK_SIZE          (FLASH_SECTOR_SIZE)
#define BLOCK_COUNT         (MICROPY_HW_FLASH_STORAGE_BYTES / BLOCK_SIZE)
#define FLASH_BASE_ADDR     (PICO_FLASH_SIZE_BYTES - MICROPY_HW_FLASH_STORAGE_BYTES)
#define FLASH_MMAP_ADDR     (XIP_BASE + FLASH_BASE_ADDR)

// Optional fatlfs shim (extmod/fatlfs/ + ports/rp2/fatlfs_rp2.c). When a fatlfs
// build is linked and it has exposed the drive, these route the MSC block device
// onto the FAT16<->littlefs translation instead of raw flash. Weak, so a plain
// MSC build (no fatlfs) behaves exactly as before: fatlfs_active() is false and
// every branch below takes the stock raw-flash path.
bool __attribute__((weak)) fatlfs_active(void) {
    return false;
}
bool __attribute__((weak)) fatlfs_readonly(void) {
    return false;
}
int32_t __attribute__((weak)) fatlfs_msc_read(uint32_t lba, uint32_t off, void *buf, uint32_t len);
int32_t __attribute__((weak)) fatlfs_msc_write(uint32_t lba, uint32_t off, uint8_t *buf, uint32_t len);
void __attribute__((weak)) fatlfs_msc_capacity(uint32_t *block_count, uint16_t *block_size);
void __attribute__((weak)) fatlfs_msc_eject(void);
void __attribute__((weak)) fatlfs_msc_sync(void);

static bool ejected = false;

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
    #if MICROPY_FATLFS
    // In a fatlfs build the MSC LUN only has media while fatlfs has exposed the
    // drive (dedicated MSC mode). In normal operation the raw-flash region is a
    // littlefs, which must NEVER be presented to the host as a disk - it would
    // look unformatted and invite the host to reformat over the filesystem. So
    // report "medium not present" and no disk appears.
    if (!fatlfs_active()) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    return true;
    #else
    if (ejected) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    return true;
    #endif
}

// Invoked to check if the medium is writable.
bool tud_msc_is_writable_cb(uint8_t lun) {
    (void)lun;
    #if MICROPY_FATLFS
    // Only writable while fatlfs has exposed the drive read-write.
    return fatlfs_active() && !fatlfs_readonly();
    #else
    return true;    // raw flash is writable
    #endif
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    if (fatlfs_active()) {
        fatlfs_msc_capacity(block_count, block_size);
        return;
    }
    #if MICROPY_FATLFS
    // No media unless fatlfs is exposing the drive (see test_unit_ready above).
    *block_size = 512;
    *block_count = 0;
    #else
    *block_size = BLOCK_SIZE;
    *block_count = BLOCK_COUNT;
    #endif
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
            if (fatlfs_active()) {
                // Notify the fatlfs service loop, which flushes staged writes to
                // littlefs and reboots to normal mode. Don't just mark ejected.
                fatlfs_msc_eject();
            }
            ejected = true;
        }
    }
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    if (fatlfs_active()) {
        return fatlfs_msc_read(lba, offset, buffer, bufsize);
    }
    #if MICROPY_FATLFS
    // No media unless fatlfs is exposing the drive: never serve the raw littlefs.
    (void)lun;
    (void)lba;
    (void)offset;
    (void)buffer;
    (void)bufsize;
    return -1;
    #else
    uint32_t count = bufsize / BLOCK_SIZE;
    memcpy(buffer, (void *)(FLASH_MMAP_ADDR + lba * BLOCK_SIZE), count * BLOCK_SIZE);
    return count * BLOCK_SIZE;
    #endif
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    if (fatlfs_active()) {
        return fatlfs_msc_write(lba, offset, buffer, bufsize);
    }
    #if MICROPY_FATLFS
    // No media unless fatlfs is exposing the drive: never let the host write to
    // the raw littlefs region (e.g. a "reformat" attempt) - it would corrupt it.
    (void)lun;
    (void)lba;
    (void)offset;
    (void)buffer;
    (void)bufsize;
    return -1;
    #else
    uint32_t count = bufsize / BLOCK_SIZE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_BASE_ADDR + lba * BLOCK_SIZE, count * BLOCK_SIZE);
    flash_range_program(FLASH_BASE_ADDR + lba * BLOCK_SIZE, buffer, count * BLOCK_SIZE);
    restore_interrupts(ints);
    return count * BLOCK_SIZE;
    #endif
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

        case 0x35:  // SCSI SYNCHRONIZE CACHE (10)
            // A host flush barrier: staged writes are safe to commit. Record it
            // for the fatlfs service loop (which does the flash-touching commit
            // later, never in this callback). Return success whether or not
            // fatlfs is active - a raw device is already durable, and stock
            // MicroPython letting this fall through to an error makes hosts retry.
            if (fatlfs_active()) {
                fatlfs_msc_sync();
            }
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
