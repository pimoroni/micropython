/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#ifndef MICROPY_INCLUDED_PY_BUILTIN_H
#define MICROPY_INCLUDED_PY_BUILTIN_H

#include "py/obj.h"

typedef enum {
    MP_IMPORT_STAT_NO_EXIST,
    MP_IMPORT_STAT_DIR,
    MP_IMPORT_STAT_FILE,
} mp_import_stat_t;

#if MICROPY_VFS

// Delegate to the VFS for import stat and builtin open.

#define mp_builtin_open_obj mp_vfs_open_obj

mp_import_stat_t mp_vfs_import_stat(const char *path);
mp_obj_t mp_vfs_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);

MP_DECLARE_CONST_FUN_OBJ_KW(mp_vfs_open_obj);

static inline mp_import_stat_t mp_import_stat(const char *path) {
    return mp_vfs_import_stat(path);
}

static inline mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_vfs_open(n_args, args, kwargs);
}

#else

// A port can provide implementations of these functions.
mp_import_stat_t mp_import_stat(const char *path);

// HACK: hang the PicoSystem builtin defs off builtin.h

extern const mp_obj_type_t PicosystemBuffer_type;
extern const mp_obj_type_t PicosystemVoice_type;

// picosystem.cpp

MP_DECLARE_CONST_FUN_OBJ_0(picosystem_init_obj);
MP_DECLARE_CONST_FUN_OBJ_0(picosystem_reset_obj);
MP_DECLARE_CONST_FUN_OBJ_0(picosystem_tick_obj);

// stats.cpp

MP_DECLARE_CONST_FUN_OBJ_0(picosystem_stats_obj);

// voice.cpp

MP_DECLARE_CONST_FUN_OBJ_KW(picosystem_play_obj);

// state.cpp

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_pen_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_clip_obj);
MP_DECLARE_CONST_FUN_OBJ_1(picosystem_blend_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_target_obj);
MP_DECLARE_CONST_FUN_OBJ_2(picosystem_camera_obj);
MP_DECLARE_CONST_FUN_OBJ_1(picosystem_spritesheet_obj);

// primitives.cpp

MP_DECLARE_CONST_FUN_OBJ_2(picosystem_pixel_obj);
MP_DECLARE_CONST_FUN_OBJ_3(picosystem_hline_obj);
MP_DECLARE_CONST_FUN_OBJ_3(picosystem_vline_obj);

MP_DECLARE_CONST_FUN_OBJ_0(picosystem_clear_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_rect_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_frect_obj);

MP_DECLARE_CONST_FUN_OBJ_3(picosystem_circle_obj);
MP_DECLARE_CONST_FUN_OBJ_3(picosystem_fcircle_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_ellipse_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_fellipse_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR(picosystem_poly_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR(picosystem_fpoly_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_line_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_blit_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_sprite_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_text_obj);
MP_DECLARE_CONST_FUN_OBJ_1(picosystem_text_width_obj);

// utility.cpp

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_rgb_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_hsv_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_intersects_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_intersection_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_contains_obj);

// hardware.cpp

MP_DECLARE_CONST_FUN_OBJ_1(picosystem_pressed_obj);
MP_DECLARE_CONST_FUN_OBJ_1(picosystem_button_obj);
MP_DECLARE_CONST_FUN_OBJ_0(picosystem_battery_obj);
MP_DECLARE_CONST_FUN_OBJ_3(picosystem_led_obj);
MP_DECLARE_CONST_FUN_OBJ_1(picosystem_backlight_obj);


mp_obj_t mp_builtin___import__(size_t n_args, const mp_obj_t *args);
>>>>>>> 89ef12a97 (PicoSystem: HACK: Move builtin defs to builtin.h.)
mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);

// A port can provide this object.
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_open_obj);

#endif

mp_obj_t mp_builtin___import__(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_micropython_mem_info(size_t n_args, const mp_obj_t *args);

MP_DECLARE_CONST_FUN_OBJ_VAR(mp_builtin___build_class___obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin___import___obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin___repl_print___obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_abs_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_all_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_any_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_bin_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_callable_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_compile_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_chr_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_delattr_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_dir_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_divmod_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_eval_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_exec_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_execfile_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_getattr_obj);
MP_DECLARE_CONST_FUN_OBJ_3(mp_builtin_setattr_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp_builtin_globals_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_hasattr_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_hash_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_help_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_hex_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_id_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_isinstance_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_issubclass_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_iter_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_len_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp_builtin_locals_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_max_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_min_obj);
#if MICROPY_PY_BUILTINS_NEXT2
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_next_obj);
#else
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_next_obj);
#endif
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_oct_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_ord_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_pow_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_print_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_repr_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_round_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_sorted_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_sum_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_input_obj);

MP_DECLARE_CONST_FUN_OBJ_2(mp_namedtuple_obj);

MP_DECLARE_CONST_FUN_OBJ_2(mp_op_contains_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_op_getitem_obj);
MP_DECLARE_CONST_FUN_OBJ_3(mp_op_setitem_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_op_delitem_obj);

extern const mp_obj_module_t mp_module___main__;
extern const mp_obj_module_t mp_module_builtins;
extern const mp_obj_module_t mp_module_uarray;
extern const mp_obj_module_t mp_module_collections;
extern const mp_obj_module_t mp_module_io;
extern const mp_obj_module_t mp_module_math;
extern const mp_obj_module_t mp_module_cmath;
extern const mp_obj_module_t mp_module_micropython;
extern const mp_obj_module_t mp_module_ustruct;
extern const mp_obj_module_t mp_module_sys;
extern const mp_obj_module_t mp_module_gc;
extern const mp_obj_module_t mp_module_thread;

extern const mp_obj_dict_t mp_module_builtins_globals;

// extmod modules
extern const mp_obj_module_t mp_module_uasyncio;
extern const mp_obj_module_t mp_module_uerrno;
extern const mp_obj_module_t mp_module_uctypes;
extern const mp_obj_module_t mp_module_uzlib;
extern const mp_obj_module_t mp_module_ujson;
extern const mp_obj_module_t mp_module_uos;
extern const mp_obj_module_t mp_module_ure;
extern const mp_obj_module_t mp_module_uheapq;
extern const mp_obj_module_t mp_module_uhashlib;
extern const mp_obj_module_t mp_module_ucryptolib;
extern const mp_obj_module_t mp_module_ubinascii;
extern const mp_obj_module_t mp_module_urandom;
extern const mp_obj_module_t mp_module_uselect;
extern const mp_obj_module_t mp_module_ussl;
extern const mp_obj_module_t mp_module_utimeq;
extern const mp_obj_module_t mp_module_machine;
extern const mp_obj_module_t mp_module_lwip;
extern const mp_obj_module_t mp_module_uwebsocket;
extern const mp_obj_module_t mp_module_webrepl;
extern const mp_obj_module_t mp_module_framebuf;
extern const mp_obj_module_t mp_module_btree;
extern const mp_obj_module_t mp_module_ubluetooth;
extern const mp_obj_module_t mp_module_uplatform;

extern const char MICROPY_PY_BUILTINS_HELP_TEXT[];

#endif // MICROPY_INCLUDED_PY_BUILTIN_H
