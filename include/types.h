#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#include <swc.h>
#include <wayland-server.h>

enum {
	BTN_LEFT   = 0x110,
	BTN_RIGHT  = 0x111,
	BTN_MIDDLE = 0x112,
};

enum {
	MOD1 = SWC_MOD_ALT,
	MOD4 = SWC_MOD_LOGO,
	SHFT = SWC_MOD_SHIFT,
	CTRL = SWC_MOD_CTRL,
};

union arg {
	int             i;
	uint32_t        u;
	float           f;
	const void*     v;
};

struct bind {
	uint32_t        type;
	uint32_t        mods;
	uint32_t        ksym;
	union           arg arg;
	void            (*fn)(void* data, uint32_t time, uint32_t value, uint32_t state);
};

struct client {
	struct          wl_list tiled_link;
	struct          wl_list float_link;
	struct          swc_window* win;
	struct          screen* scr;
	bool            mapped;
	bool            floating;
	bool            fullscreen;
	int32_t         x;
	int32_t         y;
	uint32_t        w;
	uint32_t        h;
	uint32_t        ws;
	int32_t         height_bias;
	int32_t         saved_float_x;
	int32_t         saved_float_y;
	uint32_t        saved_float_w;
	uint32_t        saved_float_h;
};

struct config {
	uint32_t        motion_throttle_hz;
	uint32_t        master_width;
	uint32_t        master_resize;
	uint32_t        border_col_active;
	uint32_t        border_col_normal;
	uint32_t        border_width;
	uint32_t        outer_border_col_active;
	uint32_t        outer_border_col_normal;
	uint32_t        outer_border_width;
	uint32_t        gaps;
	int32_t         float_x;
	int32_t         float_y;
	uint32_t        float_w;
	uint32_t        float_h;
	float           zoom_level;
};

struct grab {
	bool            active;
	bool            resize;
	struct          client* c;
};

struct screen {
	struct          wl_list link;
	struct          swc_screen* scr;
	int32_t         x;
	int32_t         y;
	uint32_t        w;
	uint32_t        h;
};

struct wm {
	struct          wl_display* dpy;
	struct          wl_event_loop* ev_loop;
	struct          wl_list screens;
	struct          wl_list tiled;
	struct          wl_list floating;
	struct          screen* sel_screen;
	struct          client* sel_client;
	struct          grab grab;
	struct          wl_event_source* zoom_timer;
	float           zoom_target;
	struct          wl_event_source* pan_timer;
	bool            pan_active;
	int32_t         pan_last_x;
	int32_t         pan_last_y;
	struct          wl_event_source* sel_timer;
	struct          wl_event_source* repeat_timer;
	void            (*repeat_fn)(void*, uint32_t, uint32_t, uint32_t);
	int32_t         repeat_arg_i;
	int             repeat_interval;
	struct          swc_rectangle repeat_geom;
	int32_t         repeat_anchor_x;
	int32_t         repeat_anchor_y;
	struct          swc_rectangle spawn_geometry;
	bool            spawn_pending;
	bool            selecting;
	bool            draw_mode;
	int32_t         sel_start_x;
	int32_t         sel_start_y;
	bool            global_floating;
	uint8_t         ws;
};

#endif /* TYPES_H */
