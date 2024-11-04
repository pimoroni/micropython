#include "py/runtime.h"
#include "extmod/network_cyw43.h"
#include "extmod/modnetwork.h"
#include "lib/cyw43-driver/src/cyw43.h"
#include "pico/unique_id.h"

void cyw43_irq_deinit(void);
void cyw43_irq_init(void);

#if CYW43_PIN_WL_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
extern int cyw43_set_pins_wl(uint pins[CYW43_PIN_INDEX_WL_COUNT]);
#endif

#if CYW43_PIO_CLOCK_DIV_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
extern void cyw43_set_pio_clock_divisor(uint16_t clock_div_int, uint8_t clock_div_frac);
#endif

static void rp2_network_cyw43_init(void) {
    static bool cyw43_init_done;
    if (!cyw43_init_done) {
        cyw43_init(&cyw43_state);
        cyw43_irq_init();
        cyw43_post_poll_hook(); // enable the irq
        cyw43_init_done = true;
    }
    uint8_t buf[8];
    memcpy(&buf[0], "PICO", 4);

    // Use unique id to generate the default AP ssid.
    const char hexchr[16] = "0123456789ABCDEF";
    pico_unique_board_id_t pid;
    pico_get_unique_board_id(&pid);
    buf[4] = hexchr[pid.id[7] >> 4];
    buf[5] = hexchr[pid.id[6] & 0xf];
    buf[6] = hexchr[pid.id[5] >> 4];
    buf[7] = hexchr[pid.id[4] & 0xf];
    cyw43_wifi_ap_set_ssid(&cyw43_state, 8, buf);
    cyw43_wifi_ap_set_auth(&cyw43_state, CYW43_AUTH_WPA2_AES_PSK);
    cyw43_wifi_ap_set_password(&cyw43_state, 8, (const uint8_t *)"picoW123");
}

mp_obj_t rp2_network_cyw43_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_interface, ARG_pin_on, ARG_pin_out, ARG_pin_in, ARG_pin_wake, ARG_pin_clock, ARG_pin_cs, ARG_pin_dat, ARG_div_int, ARG_div_frac };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_interface, MP_ARG_INT, {.u_int = MOD_NETWORK_STA_IF} },
        #if CYW43_PIN_WL_DYNAMIC
        { MP_QSTR_pin_on, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_out, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_in, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_wake, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_clock, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_cs, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_pin_dat, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        #endif
        #if CYW43_PIO_CLOCK_DIV_DYNAMIC
        { MP_QSTR_div_int, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_div_frac, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } },
        #endif
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    rp2_network_cyw43_init();

    // Set the pins
    #if CYW43_PIN_WL_DYNAMIC
    #define SET_PIN_ARG(ARG_ENUM, DEFAULT) args[ARG_ENUM].u_obj != MP_OBJ_NULL ? mp_hal_get_pin_obj(args[ARG_ENUM].u_obj) : (DEFAULT)
    uint pins[CYW43_PIN_INDEX_WL_COUNT] = {
        SET_PIN_ARG(ARG_pin_on, CYW43_DEFAULT_PIN_WL_REG_ON),
        SET_PIN_ARG(ARG_pin_out, SET_PIN_ARG(ARG_pin_dat, CYW43_DEFAULT_PIN_WL_DATA_OUT)),
        SET_PIN_ARG(ARG_pin_in, SET_PIN_ARG(ARG_pin_dat, CYW43_DEFAULT_PIN_WL_DATA_IN)),
        SET_PIN_ARG(ARG_pin_wake, SET_PIN_ARG(ARG_pin_dat, CYW43_DEFAULT_PIN_WL_HOST_WAKE)),
        SET_PIN_ARG(ARG_pin_clock, CYW43_DEFAULT_PIN_WL_CLOCK),
        SET_PIN_ARG(ARG_pin_cs, CYW43_DEFAULT_PIN_WL_CS),
    };

    // re-initialise cyw43
    cyw43_irq_deinit();
    cyw43_set_pins_wl(pins);
    cyw43_irq_init();
    #endif

    #if CYW43_PIO_CLOCK_DIV_DYNAMIC
    // set the pio clock divisor
    if (args[ARG_div_int].u_int > 0) {
        cyw43_set_pio_clock_divisor((uint16_t)args[ARG_div_int].u_int, (uint16_t)args[ARG_div_frac].u_int);
    }
    #endif

    if (n_args == 0 || mp_obj_get_int(all_args[ARG_interface]) == MOD_NETWORK_STA_IF) {
        return network_cyw43_get_interface(MOD_NETWORK_STA_IF);
    } else {
        return network_cyw43_get_interface(MOD_NETWORK_AP_IF);
    }
}
