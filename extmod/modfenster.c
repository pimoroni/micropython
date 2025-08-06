#include "py/runtime.h"
#include "fenster.h"

#define W 320
#define H 240

typedef struct _modfenster_obj_t {
    mp_obj_base_t base;
    struct fenster *f;
} modfenster_obj_t;

uint32_t buf[W * H] = {0};

struct fenster f = {
    .title = "MicroPython",
    .width = W,
    .height = H,
    .scale = 4,
    .buf = buf
};

void fenster_init(void) {
    //f.buf = m_tracked_calloc(W * H, sizeof(uint32_t));
    fenster_open(&f);
}

void fenster_deinit(void) {
    fenster_close(&f);
    //m_tracked_free(f.buf);
}

static mp_obj_t modfenster_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    modfenster_obj_t *self = mp_obj_malloc_with_finaliser(modfenster_obj_t, type);

    //f.buf = m_tracked_calloc(W * H, sizeof(uint32_t));
    self->f = &f;

    //fenster_open(self->f);

    return MP_OBJ_FROM_PTR(self);
}

mp_int_t modfenster_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    modfenster_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->f->buf;
    bufinfo->len = self->f->width * self->f->height * sizeof(uint32_t);
    bufinfo->typecode = 'B';
    return 0;
}

static mp_obj_t modfenster_time(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_int_from_ll(fenster_time());
}
static MP_DEFINE_CONST_FUN_OBJ_1(modfenster_time_obj, modfenster_time);


static mp_obj_t modfenster_sleep(mp_obj_t self_in, mp_obj_t time_in) {
    (void)self_in;
    fenster_sleep(mp_obj_get_ll(time_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(modfenster_sleep_obj, modfenster_sleep);

bool f_pressed = false;

static mp_obj_t modfenster_loop(mp_obj_t self_in) {
    modfenster_obj_t *self = MP_OBJ_TO_PTR(self_in);
    for(int i = 0; i < 255; i++) {
        if(self->f->keys[i]) {
            printf("Keys: ");
            break;
        }
    }
    for(int i = 0; i < 255; i++) {
        if(self->f->keys[i]) {
            printf("%d ", i);
        }
    }
    for(int i = 0; i < 255; i++) {
        if(self->f->keys[i]) {
            printf("\n");
            break;
        }
    }
    if (self->f->keys[0]) {
        //
        while(fenster_loop(self->f) == 0) {
            if (!self->f->keys[0]) {
                mp_raise_type_arg(&mp_type_SystemExit, MP_ROM_INT(255));
            }
        }
    }
    return mp_obj_new_int(fenster_loop(self->f));
}
static MP_DEFINE_CONST_FUN_OBJ_1(modfenster_loop_obj, modfenster_loop);


static mp_obj_t modfenster_pixel(mp_obj_t self_in, mp_obj_t xy_in, mp_obj_t c_in) {
    modfenster_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_tuple_t *xy = MP_OBJ_TO_PTR(xy_in);
    int x = mp_obj_get_int(xy->items[0]);
    int y = mp_obj_get_int(xy->items[1]);;
    fenster_pixel(self->f, x, y) = mp_obj_get_int(c_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(modfenster_pixel_obj, modfenster_pixel);


static mp_obj_t modfenster__del__(mp_obj_t self_in) {
    modfenster_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //fenster_close(self->f);
    //m_tracked_free(self->f->buf);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(modfenster__del___obj, modfenster__del__);


static const mp_rom_map_elem_t modfenster_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&modfenster_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_loop), MP_ROM_PTR(&modfenster_loop_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&modfenster_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&modfenster_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&modfenster__del___obj) },
};
static MP_DEFINE_CONST_DICT(modfenster_locals_dict, modfenster_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    type_Fenster,
    MP_QSTR_Fenster,
    MP_TYPE_FLAG_NONE,
    make_new, modfenster_make_new,
    buffer, modfenster_get_buffer,
    locals_dict, &modfenster_locals_dict
);

static const mp_rom_map_elem_t modfenster_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fenster) },
    { MP_ROM_QSTR(MP_QSTR_Fenster),  MP_ROM_PTR(&type_Fenster) },
};
static MP_DEFINE_CONST_DICT(modfenster_globals, modfenster_globals_table);

// Define module object.
const mp_obj_module_t modfenster = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modfenster_globals,
};

MP_REGISTER_MODULE(MP_QSTR_fenster, modfenster);