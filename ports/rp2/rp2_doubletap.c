/*
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"


#include "pico/time.h"
#include "pico/bootrom.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#if !PICO_RP2040
#include "hardware/powman.h"
#endif

#ifndef DOUBLE_RESET_TIMEOUT_MS
#define DOUBLE_RESET_TIMEOUT_MS 500
#endif

#if PICO_RP2040

// RP2040 stores a token in RAM, which is retained over assertion of the RUN pin.

static const uint32_t magic_token[] = {
        0xf01681de, 0xbd729b29, 0xd359be7a,
};

static uint32_t __uninitialized_ram(magic_location)[count_of(magic_token)];

static inline bool double_tap_flag_is_set(void) {
    for (uint i = 0; i < count_of(magic_token); i++) {
        if (magic_location[i] != magic_token[i]) {
            return false;
        }
    }
    return true;
}

static inline void set_double_tap_flag(void) {
    for (uint i = 0; i < count_of(magic_token); i++) {
        magic_location[i] = magic_token[i];
    }
}

static inline void clear_double_tap_flag(void) {
    magic_location[0] = 0;
}

#else

// Newer microcontrollers have a purpose-made register which is retained over
// RUN events, for detecting double-tap events. The ROM has built-in support
// for this, but this library can also use the same hardware feature.
// (Also, RAM is powered down when the RUN pin is asserted, so it's a bad
// place to put the token!)
//
// Note if ROM support is also enabled (via DOUBLE_TAP in OTP BOOT_FLAGS) then
// we never reach this point with the double tap flag still set. The window
// is the sum of the delay added by this library and the delay added by the
// ROM. It's not recommended to enable both, but it works.

static inline bool double_tap_flag_is_set(void) {
    return powman_hw->chip_reset & POWMAN_CHIP_RESET_DOUBLE_TAP_BITS;
}

static inline void set_double_tap_flag(void) {
    powman_set_bits(&powman_hw->chip_reset, POWMAN_CHIP_RESET_DOUBLE_TAP_BITS);
}

static inline void clear_double_tap_flag(void) {
    powman_clear_bits(&powman_hw->chip_reset, POWMAN_CHIP_RESET_DOUBLE_TAP_BITS);
}

#endif

/* Check for double reset and enter USB MSC mode if detected
 *
 * This function is registered to run automatically before main(). The
 * algorithm is:
 *
 *   1. Check for magic token in memory; enter USB MSC mode if found.
 *   2. Initialise that memory with that magic token.
 *   3. Do nothing for a short while (few hundred ms).
 *   4. Clear the magic token.
 *   5. Continue with normal boot.
 *
 * Resetting the device twice quickly will interrupt step 3, leaving the token
 * in place so that the second boot will go to the bootloader.
 */
extern bool rp2_set_msc_ready();

static void __attribute__((constructor)) boot_double_tap_check(void) {
#if !PICO_RP2040
    //if (!(powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_RUN_LOW_BITS)) return;
#endif
    gpio_init(8);
    gpio_set_dir(8, true);
    if (!double_tap_flag_is_set()) {
        // Arm, wait, then disarm and continue booting
        set_double_tap_flag();

        for(int i = 0; i < DOUBLE_RESET_TIMEOUT_MS / 50; i++) {
            gpio_put(8, !gpio_get(8));
            busy_wait_us(50 * 1000);
        }
        gpio_put(8, false);
        clear_double_tap_flag();
        return;
    }
    clear_double_tap_flag();
    gpio_put(8, true);
    rp2_set_msc_ready();
}

