/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Copyright (c) 2026 Philip Howard
 */

// Double-tap-reset selection of the dedicated fatlfs USB-MSC boot mode (RP2350).
//
// A single bit in the RP2350 always-on power domain (POWMAN_CHIP_RESET
// DOUBLE_TAP, bit 0) survives a warm reset. It is set on the first reset and
// cleared by a one-shot alarm once a short window closes; a *second* reset
// inside that window - or a software watchdog reboot with the bit already set -
// selects USB-MSC mode instead of the normal app. Detection is non-blocking:
// the first reset arms the flag and returns immediately, so normal boots never
// stall by the window length.
//
// This lives in its own translation unit (not on the qstr-scan path) so it can
// freely include the pico-sdk hardware headers; fatlfs_rp2.c only forward-
// declares the three entry points below.

#include <stdbool.h>
#include <stdint.h>

#include "hardware/regs/addressmap.h"
#include "hardware/regs/powman.h"
#include "hardware/watchdog.h"
#include "pico/time.h"

// Every write to a POWMAN register requires the 0x5afe password in bits 31:16.
#define POWMAN_PASSWORD     0x5afe0000u
#define DTAP_BIT            POWMAN_CHIP_RESET_DOUBLE_TAP_BITS   // 0x00000001
#define DTAP_WINDOW_MS      1000

// CHIP_RESET and its atomic set/clear aliases (RP2350: +0x2000 set, +0x3000 clear).
static volatile uint32_t *const cr =
    (volatile uint32_t *)(POWMAN_BASE + POWMAN_CHIP_RESET_OFFSET);
static volatile uint32_t *const cr_set =
    (volatile uint32_t *)(POWMAN_BASE + POWMAN_CHIP_RESET_OFFSET + 0x2000u);
static volatile uint32_t *const cr_clr =
    (volatile uint32_t *)(POWMAN_BASE + POWMAN_CHIP_RESET_OFFSET + 0x3000u);

static bool s_enter_msc;

static inline bool flag_set(void) {
    return (*cr & DTAP_BIT) != 0;
}
static inline void flag_arm(void) {
    *cr_set = POWMAN_PASSWORD | DTAP_BIT;
}
static inline void flag_clr(void) {
    *cr_clr = POWMAN_PASSWORD | DTAP_BIT;
}

// One-shot alarm: clears the flag once the double-tap window closes. Runs in the
// background so boot proceeds immediately after arming.
static int64_t fatlfs_clear_flag_cb(alarm_id_t id, void *user) {
    (void)id;
    (void)user;
    flag_clr();
    return 0;   // don't reschedule
}

// Decide whether this boot should enter USB-MSC mode. Call once, early in main().
void fatlfs_boot_check(void) {
    if (watchdog_caused_reboot()) {
        // A software/watchdog reboot. Only enter MSC mode if a program explicitly
        // asked for it (fatlfs_request_msc_reboot set the flag first); otherwise
        // this is an ordinary reset and must not arm or false-trigger the window.
        if (flag_set()) {
            flag_clr();
            s_enter_msc = true;
        }
        return;
    }

    // A genuine power-on or RUN-pin (button) reset: run the double-tap window.
    // The DOUBLE_TAP bit lives in the always-on domain, so a first press's arm
    // survives the second press's core reset.
    if (flag_set()) {
        // (B) Second press within the window -> double-tap.
        flag_clr();
        s_enter_msc = true;
        return;
    }

    // (A) First press: arm the flag, schedule it to clear after the window, and
    //     keep booting. No sleep here - that's the whole point.
    flag_arm();
    add_alarm_in_ms(DTAP_WINDOW_MS, fatlfs_clear_flag_cb, NULL, false);
}

bool fatlfs_msc_requested(void) {
    return s_enter_msc;
}

// Software trigger (fatlfs.reboot_msc()): set the flag then warm-reboot so the
// next boot's watchdog path enters MSC mode. Does not return.
void fatlfs_request_msc_reboot(void) {
    flag_arm();
    watchdog_reboot(0, 0, 0);
    for (;;) {
    }
}
