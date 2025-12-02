
#include "py/runtime.h"

extern const mp_obj_type_t type_Shapes;
extern const mp_obj_type_t type_Brushes;
extern const mp_obj_type_t type_Image;
extern const mp_obj_type_t type_Font;
extern const mp_obj_type_t type_PixelFont;
extern const mp_obj_type_t type_Matrix;

// modinput
#define BUTTON_HOME 0b100000
#define BUTTON_A    0b010000
#define BUTTON_B    0b001000
#define BUTTON_C    0b000100
#define BUTTON_UP   0b000010
#define BUTTON_DOWN 0b000001

extern void modinput_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);
extern mp_obj_t modinput_poll();
static MP_DEFINE_CONST_FUN_OBJ_1(modinput_poll_obj, modinput_poll);

const mp_rom_map_elem_t modinput_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_poll), MP_ROM_PTR(&modinput_poll_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_HOME), MP_ROM_INT(BUTTON_HOME) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_A),    MP_ROM_INT(BUTTON_A) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_B),    MP_ROM_INT(BUTTON_B) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_C),    MP_ROM_INT(BUTTON_C) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_UP),   MP_ROM_INT(BUTTON_UP) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_DOWN), MP_ROM_INT(BUTTON_DOWN) },
};

static MP_DEFINE_CONST_DICT(modinput_globals, modinput_globals_table);
const mp_obj_module_t modinput = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modinput_globals,
};

MP_REGISTER_MODULE(MP_QSTR__io, modinput);
MP_REGISTER_MODULE_DELEGATION(modinput, modinput_attr);


// modpicovector
extern void modpicovector_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);
extern mp_obj_t modpicovector___init__(void);
extern mp_obj_t modpicovector_dda(size_t n_args, const mp_obj_t *pos_args);
static MP_DEFINE_CONST_FUN_OBJ_0(modpicovector___init___obj, modpicovector___init__);
static MP_DEFINE_CONST_FUN_OBJ_VAR(modpicovector_dda_obj, 5, modpicovector_dda);

static const mp_rom_map_elem_t modpicovector_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_modpicovector) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&modpicovector___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_dda), MP_ROM_PTR(&modpicovector_dda_obj) },
    { MP_ROM_QSTR(MP_QSTR_brushes),  MP_ROM_PTR(&type_Brushes) },
    { MP_ROM_QSTR(MP_QSTR_shapes),  MP_ROM_PTR(&type_Shapes) },
    { MP_ROM_QSTR(MP_QSTR_Image),  MP_ROM_PTR(&type_Image) },
    { MP_ROM_QSTR(MP_QSTR_Font),  MP_ROM_PTR(&type_Font) },
    { MP_ROM_QSTR(MP_QSTR_PixelFont),  MP_ROM_PTR(&type_PixelFont) },
    { MP_ROM_QSTR(MP_QSTR_Matrix),  MP_ROM_PTR(&type_Matrix) },
    { MP_ROM_QSTR(MP_QSTR_io),  MP_ROM_PTR(&modinput) },
};
static MP_DEFINE_CONST_DICT(modpicovector_globals, modpicovector_globals_table);


// Define module object.
const mp_obj_module_t modpicovector = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modpicovector_globals,
};

MP_REGISTER_MODULE(MP_QSTR_picovector, modpicovector);
MP_REGISTER_MODULE_DELEGATION(modpicovector, modpicovector_attr);