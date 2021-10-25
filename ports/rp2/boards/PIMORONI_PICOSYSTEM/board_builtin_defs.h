#define MICROPY_BOARD_BUILTIN_DEFS \
    extern const mp_obj_type_t PicosystemBuffer_type; \
    extern const mp_obj_type_t PicosystemVoice_type; \
\
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_init_obj); \
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_logo_obj); \
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_start_obj); \
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_quit_obj); \
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_flip_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_stats_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_KW(picosystem_play_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_pen_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_alpha_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_clip_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_blend_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_target_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_camera_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_cursor_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_spritesheet_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_2(picosystem_pixel_obj); \
    MP_DECLARE_CONST_FUN_OBJ_3(picosystem_hline_obj); \
    MP_DECLARE_CONST_FUN_OBJ_3(picosystem_vline_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_clear_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_rect_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_frect_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_3(picosystem_circle_obj); \
    MP_DECLARE_CONST_FUN_OBJ_3(picosystem_fcircle_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_ellipse_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_fellipse_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR(picosystem_poly_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR(picosystem_fpoly_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_line_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_blit_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_sprite_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_text_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_measure_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_rgb_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_hsv_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_intersects_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_intersection_obj); \
    MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(picosystem_contains_obj); \
\
    MP_DECLARE_CONST_FUN_OBJ_1(picosystem_pressed_obj); \
    MP_DECLARE_CONST_FUN_OBJ_1(picosystem_button_obj); \
    MP_DECLARE_CONST_FUN_OBJ_0(picosystem_battery_obj); \
    MP_DECLARE_CONST_FUN_OBJ_3(picosystem_led_obj); \
    MP_DECLARE_CONST_FUN_OBJ_1(picosystem_backlight_obj);
