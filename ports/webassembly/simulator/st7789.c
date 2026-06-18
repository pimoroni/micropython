// A drop-in replacement for the Tufty 2350 st7789 display driver
// (tufty2350/modules/c/st7789). It exposes the same `st7789.ST7789` interface
// but, instead of driving a real panel, flips a shared RGBA framebuffer to the
// web worker for rendering to a canvas.
//
// The framebuffer is exposed via the buffer protocol so the badgeware library
// can wrap it directly: image(display.WIDTH, display.HEIGHT, memoryview(display)).

#include "py/objmodule.h"
#include "py/runtime.h"
#include <emscripten.h>
#include <emscripten/em_asm.h>

#if MICROPY_ENABLE_VM_YIELD
// Defined in main.c: resets the cooperative-yield throttle, since the
// emscripten_sleep() in update() has just given the event loop a turn.
extern void mp_js_yield_reset(void);
#endif

// 320x240 RGBA, large enough for the hires mode; lores uses the first quarter.
static uint32_t framebuffer[320 * 240];

// Display mode: 0 = lores (160x120), 1 = hires (320x240). Set via fullres().
static int st7789_mode = 0;

typedef struct _st7789_obj_t {
    mp_obj_base_t base;
} st7789_obj_t;

extern const mp_obj_type_t st7789_ST7789_type;

static mp_obj_t st7789_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)type; (void)n_args; (void)n_kw; (void)args;
    st7789_obj_t *self = mp_obj_malloc(st7789_obj_t, &st7789_ST7789_type);
    return MP_OBJ_FROM_PTR(self);
}

// update([fullres]) - flip the framebuffer to the worker. The real driver's
// update() takes no argument and uses the mode set by fullres(); an optional
// argument is accepted as a convenience and updates the mode in passing.
static mp_obj_t st7789_update(size_t n_args, const mp_obj_t *args) {
    if(n_args > 1) {
        st7789_mode = mp_obj_is_true(args[1]) ? 1 : 0;
    }
    if(st7789_mode) {
        EM_ASM({
            let data = Module.HEAPU8.slice($0, $0 + 320 * 240 * 4);
            WorkerGlobalScope.worker.flip_hires(data);
        }, (uint8_t *)framebuffer);
    } else {
        EM_ASM({
            let data = Module.HEAPU8.slice($0, $0 + 160 * 120 * 4);
            WorkerGlobalScope.worker.flip_lores(data);
        }, (uint8_t *)framebuffer);
    }
    emscripten_sleep(6);
#if MICROPY_ENABLE_VM_YIELD
    mp_js_yield_reset();
#endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7789_update_obj, 1, 2, st7789_update);

// fullres(mode) - select hires (True, 320x240) or lores (False, 160x120).
static mp_obj_t st7789_fullres(mp_obj_t self_in, mp_obj_t mode_in) {
    (void)self_in;
    st7789_mode = mp_obj_is_true(mode_in) ? 1 : 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(st7789_fullres_obj, st7789_fullres);

// backlight(value) - 0.0..1.0, forwarded to the worker for visualisation.
static mp_obj_t st7789_backlight(mp_obj_t self_in, mp_obj_t value_in) {
    (void)self_in;
    EM_ASM({
        if(typeof WorkerGlobalScope !== 'undefined' && WorkerGlobalScope.worker) {
            WorkerGlobalScope.worker.backlight = $0 / 255;
        }
    }, (int)(mp_obj_get_float(value_in) * 255));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(st7789_backlight_obj, st7789_backlight);

// The following match the hardware driver's interface but have no effect in
// the simulator.
static mp_obj_t st7789_set_vsync(mp_obj_t self_in, mp_obj_t sync_in) {
    (void)self_in; (void)sync_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(st7789_set_vsync_obj, st7789_set_vsync);

static mp_obj_t st7789_command(mp_obj_t self_in, mp_obj_t reg_in, mp_obj_t data_in) {
    (void)self_in; (void)reg_in; (void)data_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(st7789_command_obj, st7789_command);

static mp_obj_t st7789_set_max_pio_clock(mp_obj_t self_in, mp_obj_t value_in) {
    (void)self_in; (void)value_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(st7789_set_max_pio_clock_obj, st7789_set_max_pio_clock);

static mp_obj_t st7789___del__(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(st7789___del___obj, st7789___del__);

// Expose the framebuffer via the buffer protocol (memoryview(display), etc).
static mp_int_t st7789_get_framebuffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)self_in; (void)flags;
    bufinfo->buf = framebuffer;
    bufinfo->len = sizeof(framebuffer);
    bufinfo->typecode = 'B';
    return 0;
}

static void st7789_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    (void)self_in;
    if(dest[0] == MP_OBJ_NULL) {
        if(attr == MP_QSTR_WIDTH)  { dest[0] = mp_obj_new_int(st7789_mode ? 320 : 160); return; }
        if(attr == MP_QSTR_HEIGHT) { dest[0] = mp_obj_new_int(st7789_mode ? 240 : 120); return; }
    }
    dest[1] = MP_OBJ_SENTINEL;
}

static const mp_rom_map_elem_t st7789_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&st7789___del___obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&st7789_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_fullres), MP_ROM_PTR(&st7789_fullres_obj) },
    { MP_ROM_QSTR(MP_QSTR_backlight), MP_ROM_PTR(&st7789_backlight_obj) },
    { MP_ROM_QSTR(MP_QSTR_command), MP_ROM_PTR(&st7789_command_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_max_pio_clock), MP_ROM_PTR(&st7789_set_max_pio_clock_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_vsync), MP_ROM_PTR(&st7789_set_vsync_obj) },
};
static MP_DEFINE_CONST_DICT(st7789_locals_dict, st7789_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    st7789_ST7789_type,
    MP_QSTR_ST7789,
    MP_TYPE_FLAG_NONE,
    make_new, st7789_make_new,
    attr, st7789_attr,
    buffer, st7789_get_framebuffer,
    locals_dict, &st7789_locals_dict
);

static const mp_rom_map_elem_t st7789_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_st7789) },
    { MP_ROM_QSTR(MP_QSTR_ST7789), MP_ROM_PTR(&st7789_ST7789_type) },
};
static MP_DEFINE_CONST_DICT(st7789_globals, st7789_globals_table);

const mp_obj_module_t mp_module_st7789 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&st7789_globals,
};

MP_REGISTER_MODULE(MP_QSTR_st7789, mp_module_st7789);
