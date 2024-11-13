#include "py/runtime.h"
#include "extmod/network_cyw43.h"
#include "extmod/modnetwork.h"
#include "lib/cyw43-driver/src/cyw43.h"
#include "pico/unique_id.h"

void cyw43_irq_deinit(void);
void cyw43_irq_init(void);

#if CYW43_PIN_WL_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
int cyw43_set_pins_wl(uint pins[CYW43_PIN_INDEX_WL_COUNT]);
#endif

#if CYW43_PIO_CLOCK_DIV_DYNAMIC
// Defined in cyw43_bus_pio_spi.c
void cyw43_set_pio_clkdiv_int_frac8(uint32_t clock_div_int, uint8_t clock_div_frac8);
#endif

// Overridden functions
mp_obj_t network_cyw43_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t bluetooth_ble_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args);

static void rp2_network_cyw43_init(uint *pins, uint32_t clock_div_int, uint8_t clock_div_frac8) {
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

    #if CYW43_PIN_WL_DYNAMIC
    // check if the pins have actually changed
    for (int i = 0; i < CYW43_PIN_INDEX_WL_COUNT; i++) {
        if (pins[i] != cyw43_get_pin_wl(i)) {
            // re-initialise cyw43. This can fail if the pins are invalid (gpio base is incompatible) or the pio is in use
            int error = cyw43_set_pins_wl(pins);
            if (error == PICO_ERROR_RESOURCE_IN_USE) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("wifi in use"));
            } else if (error != PICO_OK) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("wifi pins invalid"));
            }
            cyw43_irq_deinit();
            cyw43_irq_init();
            break;
        }
    }
    #endif
    #if CYW43_PIO_CLOCK_DIV_DYNAMIC
    // set the pio clock divisor - has no effect if the pio is in use
    if (clock_div_int > 0) {
        cyw43_set_pio_clkdiv_int_frac8(clock_div_int, clock_div_frac8);
    }
    #endif
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

    // Set the pins - order defined by cyw43_pin_index_t
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
    #else
    uint *pins = NULL;
    #endif

    #if CYW43_PIO_CLOCK_DIV_DYNAMIC
    uint32_t clock_div_int = args[ARG_div_int].u_int;
    uint8_t clock_div_frac8 = (uint8_t)args[ARG_div_frac].u_int;
    #else
    uint32_t clock_div_int = 0;
    uint8_t clock_div_frac8 = 0;
    #endif

    rp2_network_cyw43_init(pins, clock_div_int, clock_div_frac8);
    const mp_obj_t base_args[] = {  mp_obj_new_int(args[ARG_interface].u_int) };
    return network_cyw43_make_new(type, 1, 0, base_args);
}

mp_obj_t rp2_bluetooth_ble_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_pin_on, ARG_pin_out, ARG_pin_in, ARG_pin_wake, ARG_pin_clock, ARG_pin_cs, ARG_pin_dat, ARG_div_int, ARG_div_frac };
    static const mp_arg_t allowed_args[] = {
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

    // Set the pins - order defined by cyw43_pin_index_t
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
    #else
    uint *pins = NULL;
    #endif

    #if CYW43_PIO_CLOCK_DIV_DYNAMIC
    uint32_t clock_div_int = args[ARG_div_int].u_int;
    uint8_t clock_div_frac8 = (uint8_t)args[ARG_div_frac].u_int;
    #else
    uint32_t clock_div_int = 0;
    uint8_t clock_div_frac8 = 0;
    #endif

    rp2_network_cyw43_init(pins, clock_div_int, clock_div_frac8);
    return bluetooth_ble_make_new(NULL, 0, 0, NULL);
}
