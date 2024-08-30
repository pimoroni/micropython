/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Damien P. George
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

#include "hardware/structs/ioqspi.h"
#include "hardware/structs/qmi.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "rp2_psram.h"

// Notes on the PSRAM IC supported
//
// Most boards seem to support a variant of the following PSRAM IC:
//
//      apmemory APS6404L-3SQR-ZR
//      https://www.mouser.com/ProductDetail/AP-Memory/APS6404L-3SQR-ZR?qs=IS%252B4QmGtzzpDOdsCIglviw%3D%3D
//
// The origin of this logic is from the Circuit Python code that was downloaded from:
//     https://github.com/raspberrypi/pico-sdk-rp2350/issues/12#issuecomment-2055274428
//
// The returned ID value for the above IC
const uint8_t PSRAM_ID = 0x5D;

// From the above datasheet, PSRAM SPI command codes

const uint8_t PSRAM_CMD_QUAD_END = 0xF5;
const uint8_t PSRAM_CMD_QUAD_ENABLE = 0x35;
const uint8_t PSRAM_CMD_READ_ID = 0x9F;
const uint8_t PSRAM_CMD_RSTEN = 0x66;
const uint8_t PSRAM_CMD_RST = 0x99;
const uint8_t PSRAM_CMD_QUAD_READ = 0xEB;
const uint8_t PSRAM_CMD_QUAD_WRITE = 0x38;
const uint8_t PSRAM_CMD_NOOP = 0xFF;


// For PSRAM timing calculations - to use int math, we work in femto seconds (fs) (1e-15),
// NOTE: This idea is from micro python work on psram support.

#define SEC_TO_FS 1000000000000000ll

// max select pulse width = 8us => 8e6 ns => 8000 ns => 8000 * 1e6 fs => 8000e6 fs
// Additionally, the MAX select is in units of 64 clock cycles - will use a constant that
// takes this into account - so 8000e6 fs / 64 = 125e6 fs
const uint32_t PSRAM_MAX_SELECT_FS64 = 125000000;

// min deselect pulse width = 18ns => 18 * 1e6 fs => 18e6 fs
const uint32_t PSRAM_MIN_DESELECT_FS = 18000000;

// Define the divisor limit for at 100MHz
#define PSRAM_DIVISOR_LIMIT 100000000

size_t __no_inline_not_in_flash_func(psram_detect)() {

    int psram_size = 0;

    uint32_t intr_stash = save_and_disable_interrupts();

    // Try and read the PSRAM ID via direct_csr.
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;

    // Need to poll for the cooldown on the last XIP transfer to expire
    // (via direct-mode BUSY flag) before it is safe to perform the first
    // direct-mode operation
    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
    }

    // Exit out of QMI in case we've inited already
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;

    //  Transmit the command to exit QPI quad mode - read ID as standard SPI
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS | QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB | PSRAM_CMD_QUAD_END;

    while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
    }

    (void)qmi_hw->direct_rx;

    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);

    // Read the id
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    uint8_t kgd = 0;  // known good die
    uint8_t eid = 0;  // density/size

    for (size_t i = 0; i < 7; i++)
    {
        if (i == 0) {
            qmi_hw->direct_tx = PSRAM_CMD_READ_ID;
        } else {
            qmi_hw->direct_tx = PSRAM_CMD_NOOP;
        }

        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS) == 0) {
        }

        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0) {
        }

        if (i == 5) {
            kgd = qmi_hw->direct_rx;
        } else if (i == 6) {
            eid = qmi_hw->direct_rx;
        } else {
            (void)qmi_hw->direct_rx;
        }
    }

    // Disable direct csr.
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

    if (kgd == PSRAM_ID) {
        psram_size = 1024 * 1024; // 1 MiB
        uint8_t size_id = eid >> 5;
        if (eid == 0x26 || size_id == 2) {
            psram_size *= 8; // 8 MiB
        } else if (size_id == 0) {
            psram_size *= 2; // 2 MiB
        } else if (size_id == 1) {
            psram_size *= 4; // 4 MiB
        }
    }

    restore_interrupts(intr_stash);
    return psram_size;
}


//-----------------------------------------------------------------------------
/// @brief Update the PSRAM timing configuration based on system clock
///
/// @param max_psram_freq The maximum frequency the PSRAM can operate at.
//
// Note: Made this into a fuction incase it's needed to be called if the system clock changes.
//
 void __no_inline_not_in_flash_func(rp2_psram_set_timing)(const uint32_t max_psram_freq)
{
    // Set PSRAM timing for APS6404
    //
    // Note: The max SCK frequency of the APS6404 depends on the VDD voltage supplied. 
    //       The max frequency is 133MHz for VDD = 3.0V, 109MHz for VDD = 3.3V
    //       See the pages 1 of the above datasheet for more details. Since this depends on 
    //       the board design, this is a parameter passed to this function.
    //
    // Using an rxdelay equal to the divisor isn't enough when running the APS6404 close to 133MHz.
    // So: don't allow running at divisor 1 above 100MHz (because delay of 2 would be too late),
    // and add an extra 1 to the rxdelay if the divided clock is > 100MHz (i.e. sys clock > 200MHz).
    
    const uint32_t clock_hz = clock_get_hz(clk_sys);
    uint8_t divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > PSRAM_DIVISOR_LIMIT) {
        divisor = 2;
    }
    uint8_t rxdelay = divisor;
    if (clock_hz / divisor > PSRAM_DIVISOR_LIMIT) {
        rxdelay += 1;
    }

    // clock period in femto seconds
    const uint32_t clock_period_fs = SEC_TO_FS / clock_hz;

    // - Max select must be <= 8us.  The value is given in multiples of 64 system clocks.    
    // So maxFS / (64 * clock_period_fs) = max_select = PSRAM_MAX_SELECT_FS64/clock_period_fs
    const uint8_t max_select = PSRAM_MAX_SELECT_FS64 / clock_period_fs;  // 125 = 8000ns / 64

    // - Min deselect must be >= 18ns.  The value is given in system clock cycles - ceil(divisor / 2).
    const uint8_t min_deselect = (PSRAM_MIN_DESELECT_FS + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;

    qmi_hw->m[1].timing = 1 << QMI_M1_TIMING_COOLDOWN_LSB |
        QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Break between pages.
        max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
        min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
        rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
        divisor << QMI_M1_TIMING_CLKDIV_LSB;
}

size_t __no_inline_not_in_flash_func(rp2_psram_init)(uint cs_pin, const uint32_t max_psram_freq) {

    gpio_set_function(cs_pin, GPIO_FUNC_XIP_CS1);

    size_t psram_size = psram_detect();

    if (!psram_size) {
        return 0;
    }

    // After the PSRAM detect operation, and this is startup, do a RESETEN, RESET and quad enable

    // Enable QPI mode on the PSRAM - use "direct mode" to send the command
    // From the flash boot ASM code (in the SDK):
    //     Need to use direct serial mode to send SR commands. Choose a
    //     conservative direct-mode divisor (5 MHz at 150 MHz clk_sys)
    //     since the XIP-mode divisor may be unsafe without an RX delay.
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | QMI_DIRECT_CSR_EN_BITS;

    // Need to poll for the cooldown on the last XIP transfer to expire
    // (via direct-mode BUSY flag) before it is safe to perform the first
    // direct-mode operation
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) {
        ;
    }
    // RESETEN, RESET and quad enable
    for (uint8_t i = 0; i < 3; i++)
    {
        qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
        if (i == 0)
            qmi_hw->direct_tx = PSRAM_CMD_RSTEN;
        else if (i == 1)
            qmi_hw->direct_tx = PSRAM_CMD_RST;
        else
            qmi_hw->direct_tx = PSRAM_CMD_QUAD_ENABLE;

        while ((qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS) != 0)
        {
        }
        qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS);
        for (size_t j = 0; j < 20; j++)
            asm("nop");

        (void)qmi_hw->direct_rx;
    }

    // Disable direct csr.
    qmi_hw->direct_csr &= ~(QMI_DIRECT_CSR_ASSERT_CS1N_BITS | QMI_DIRECT_CSR_EN_BITS);

    // setup the PSRAM timing
    rp2_psram_set_timing(max_psram_freq);

    // Set PSRAM commands and formats
    qmi_hw->m[1].rfmt =
           (QMI_M1_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_PREFIX_WIDTH_LSB |
            QMI_M1_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_RFMT_ADDR_WIDTH_LSB |
            QMI_M1_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_RFMT_SUFFIX_WIDTH_LSB |
            QMI_M1_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_RFMT_DUMMY_WIDTH_LSB |
            QMI_M1_RFMT_DATA_WIDTH_VALUE_Q << QMI_M1_RFMT_DATA_WIDTH_LSB |
            QMI_M1_RFMT_PREFIX_LEN_VALUE_8 << QMI_M1_RFMT_PREFIX_LEN_LSB |
            QMI_M1_RFMT_DUMMY_LEN_VALUE_24 << QMI_M1_RFMT_DUMMY_LEN_LSB | 
            QMI_M1_RFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_RFMT_SUFFIX_LEN_LSB );

    qmi_hw->m[1].rcmd = PSRAM_CMD_QUAD_READ << QMI_M1_RCMD_PREFIX_LSB;

    qmi_hw->m[1].wfmt =
          ( QMI_M1_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_PREFIX_WIDTH_LSB | 
            QMI_M1_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M1_WFMT_ADDR_WIDTH_LSB | 
            QMI_M1_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M1_WFMT_SUFFIX_WIDTH_LSB | 
            QMI_M1_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M1_WFMT_DUMMY_WIDTH_LSB | 
            QMI_M1_WFMT_DUMMY_LEN_VALUE_NONE << QMI_M1_WFMT_DUMMY_LEN_LSB |
            QMI_M1_WFMT_DATA_WIDTH_VALUE_Q << QMI_M1_WFMT_DATA_WIDTH_LSB | 
            QMI_M1_WFMT_PREFIX_LEN_VALUE_8 << QMI_M1_WFMT_PREFIX_LEN_LSB | 
            QMI_M1_WFMT_SUFFIX_LEN_VALUE_NONE << QMI_M1_WFMT_SUFFIX_LEN_LSB );

    qmi_hw->m[1].wcmd = PSRAM_CMD_QUAD_WRITE << QMI_M1_WCMD_PREFIX_LSB;

    // Enable writes to PSRAM
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    return psram_size;
}
