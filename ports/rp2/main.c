/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
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
 */

#include <stdio.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/stackctrl.h"
#include "extmod/modbluetooth.h"
#include "extmod/modnetwork.h"
#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"
#include "tusb.h"
#include "uart.h"
#include "modmachine.h"
#include "modrp2.h"
#include "mpbthciport.h"
#include "genhdr/mpversion.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/rtc.h"
#include "hardware/structs/rosc.h"

extern uint8_t __StackTop, __StackBottom;
static char gc_heap[162 * 1024]; // Leave room for PicoSystem display buffer

// Embed version info in the binary in machine readable form
bi_decl(bi_program_version_string(MICROPY_GIT_TAG));

// Add a section to the picotool output similar to program features, but for frozen modules
// (it will aggregate BINARY_INFO_ID_MP_FROZEN binary info)
bi_decl(bi_program_feature_group_with_flags(BINARY_INFO_TAG_MICROPYTHON,
    BINARY_INFO_ID_MP_FROZEN, "frozen modules",
    BI_NAMED_GROUP_SEPARATE_COMMAS | BI_NAMED_GROUP_SORT_ALPHA));

int main(int argc, char **argv) {
    set_sys_clock_khz(250000, true);

    #if MICROPY_HW_ENABLE_UART_REPL
    bi_decl(bi_program_feature("UART REPL"))
    setup_default_uart();
    mp_uart_init();
    #endif

    #if MICROPY_HW_ENABLE_USBDEV
    bi_decl(bi_program_feature("USB REPL"))
    tusb_init();
    #endif

    #if MICROPY_PY_THREAD
    bi_decl(bi_program_feature("thread support"))
    mp_thread_init();
    #endif

    // Start and initialise the RTC
    datetime_t t = {
        .year = 2021,
        .month = 1,
        .day = 1,
        .dotw = 4, // 0 is Monday, so 4 is Friday
        .hour = 0,
        .min = 0,
        .sec = 0,
    };
    rtc_init();
    rtc_set_datetime(&t);

    // Initialise stack extents and GC heap.
    mp_stack_set_top(&__StackTop);
    mp_stack_set_limit(&__StackTop - &__StackBottom - 256);
    gc_init(&gc_heap[0], &gc_heap[MP_ARRAY_SIZE(gc_heap)]);

    for (;;) {

        // Initialise MicroPython runtime.
        mp_init();
        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

        // Initialise sub-systems.
        readline_init0();
        machine_pin_init();
        rp2_pio_init();

        #if MICROPY_PY_BLUETOOTH
        mp_bluetooth_hci_init();
        #endif
        #if MICROPY_PY_NETWORK
        mod_network_init();
        #endif

        // Execute _boot.py to set up the filesystem.
        pyexec_frozen_module("_boot.py");

        // Execute user scripts.
        int ret = pyexec_file_if_exists("boot.py");
        if (ret & PYEXEC_FORCED_EXIT) {
            goto soft_reset_exit;
        }
        if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
            ret = pyexec_file_if_exists("main.py");
            if (ret & PYEXEC_FORCED_EXIT) {
                goto soft_reset_exit;
            }
        }

        for (;;) {
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                if (pyexec_raw_repl() != 0) {
                    break;
                }
            } else {
                if (pyexec_friendly_repl() != 0) {
                    break;
                }
            }
        }

    soft_reset_exit:
        mp_printf(MP_PYTHON_PRINTER, "MPY: soft reboot\n");
        #if MICROPY_PY_NETWORK
        mod_network_deinit();
        #endif
        rp2_pio_deinit();
        #if MICROPY_PY_BLUETOOTH
        mp_bluetooth_deinit();
        #endif
        machine_pin_deinit();
        #if MICROPY_PY_THREAD
        mp_thread_deinit();
        #endif
        gc_sweep_all();
        mp_deinit();
    }

    return 0;
}

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif
    gc_collect_end();
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught exception %p\n", val);
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(val));
    for (;;) {
        __breakpoint();
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    panic("Assertion failed");
}
#endif

#define POLY (0xD5)

uint8_t rosc_random_u8(size_t cycles) {
    static uint8_t r;
    for (size_t i = 0; i < cycles; ++i) {
        r = ((r << 1) | rosc_hw->randombit) ^ (r & 0x80 ? POLY : 0);
        mp_hal_delay_us_fast(1);
    }
    return r;
}

uint32_t rosc_random_u32(void) {
    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
        value = value << 8 | rosc_random_u8(32);
    }
    return value;
}

const char rp2_help_text[] =
    "Welcome to PicoSystem - Powered by MicroPython!\n"
    "\n"
    "Boot options:\n"
    " * Hold X to flash a new .uf2 (game)\n"
    " * Hold A to enter the launcher (MicroPython only)\n"
    "\n"
    "\n"
    "For help, please visit: https://github.com/pimoroni/picosystem/tree/main/micropython.\n"
    "\n"
    "\n"
    "API quick reference:\n"
    "State:\n"
    "  pen(r, g, b)            -- set the drawing colour, \n"
    "                             r,g,b should be between 0 and 15\n"
    "  alpha(a)                -- global alpha, 0 - 15\n"
    "  clip(x, y, w, h)        -- clipping rectangle\n"
    "  cursor(x, y)            -- text cursor\n"
    "  blend(ALPHA/COPY/MASK)  -- blend mode\n"
    "\n"
    "Text:\n"
    "  text(\"Text\")            -- text\n"
    "  text(\"Text\", x, y)      -- text at x, y\n"
    "  w, h = measure(\"Text\")  -- measure text\n"
    "\n"
    "Primitives:\n"
    "  clear()                 -- clear the screen to pen colour\n"
    "  pixel(x, y)             -- single pixel\n"
    "  sprite(i, x, y)         -- sprite\n"
    "  rect(x, y, w, h)        -- rectangle\n"
    "  frect(x, y, w, h)       -- filled rectangle\n"
    "  circle(x, y, r)         -- circle\n"
    "  fcircle(x, y, r)        -- filled circle\n"
    "  ellipse(x, y, rx, ry)   -- ellipse\n"
    "  fellipse(x, y, rx, ry)  -- filled ellipse\n"
    "  poly((0, 0), (1, 1) ..) -- polygon\n"
    "  fpoly((0, 0), (1, 1) ..)-- filled polygon\n"
    "  hline(x, y, len)        -- horiontal line (fast)\n"
    "  vline(x, y, len)        -- vertical line (fast)\n"
    "  line(x1, y1, x2, y2)    -- arbitrary line\n"
    "\n"
    "Sound:\n"
    "  v = Voice()             -- new default voice\n"
    "  v.play(pitch)           -- play a note\n"
    "  v.play(pitch, duration) -- play a note for duration ms\n"
    "  v.play(pitch, dur, vol) -- Volume (0-100)\n"
    "  v.envelope(a, d, s, r)  -- set the voice Attack (ms),\n"
    "                             Decay (ms), Sustain (0-100), Release (ms)"
    "\n"
    "Utilities:\n"
    "  rgb(r, g, b)            -- create RGB (16-bit ARGB) colour\n"
    "  hsv(h, s, v)            -- convert HSV to RGB\n"
    "  flip()                  -- display your changes\n"
    "\n"
    "Minimal app:\n"
    "-----------------------------------------------------------------------\n"
    "\n"
    "def update(tick):\n"
    "    pass\n"
    "\n"
    "def draw(tick):\n"
    "    pen(0, 0, 0)\n"
    "    clear()\n"
    "    pen(15, 15, 15)\n"
    "    text(\"Hello World\", 42, 57)\n"
    "\n"
    "start()\n"
    "\n"
    "-----------------------------------------------------------------------\n"
    "\n"
    "Useful control commands:\n"
    "  CTRL-C -- interrupt a running program\n"
    "  CTRL-D -- on a blank line, do a soft reset of the board\n"
    "  CTRL-E -- on a blank line, enter paste mode\n"
    "\n"
    "For further help on a specific object, type help(obj)\n"
    "For a list of available modules, type help('modules')\n"
;
