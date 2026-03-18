#ifndef NEUWM_H
#define NEUWM_H

#include <stdint.h>

#include "types.h"

extern void focus_next(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void focus_prev(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void kill_sel(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void master_next(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void master_prev(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void master_resize(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void mouse_move(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void mouse_resize(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void new_screen(struct swc_screen* scr);
extern void new_window(struct swc_window* win);
extern void new_device(struct libinput_device* dev);
extern void quit(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void spawn(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void toggle_float(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void toggle_float_global(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_move_x(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_move_y(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_resize_w(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_resize_h(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_resize_w_right(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_resize_h_bottom(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void float_scale(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void fullscreen(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void workspace_goto(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void workspace_moveto(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void zoom_axis(void* data, uint32_t time, uint32_t axis, int32_t value120);
extern void mouse_click(void* data, uint32_t time, uint32_t value, uint32_t state);
extern void draw_mode_toggle(void* data, uint32_t time, uint32_t value, uint32_t state);
extern struct wm wm;

#endif /* NEUWM_H */
