#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <swc.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include "config.h"
#include "types.h"
#include "util.h"
#include "neuwm.h"
#include <sys/wait.h>
#include <libinput.h>

static void autostart(void);
static void focus(struct client* c, bool raise);
static void on_screen_destroy(void* data);
static void on_screen_usable_geometry_changed(void* data);
static void on_win_destroy(void* data);
static void on_win_entered(void* data);
static void setup(void);
static void setup_binds(void);
static void setup_cursor(void);
static void set_floating(struct client* c, bool floating, bool raise);
static void tile(struct screen* s);

static int zoom_tick(void* data);
static int pan_drag_tick(void* data);
static int sel_tick(void* data);
static int next_cascade_slot(void);

static uint32_t master_width = 0;

struct wm wm;
const struct swc_manager manager = {
	.new_screen = new_screen, .new_window = new_window, .new_device = new_device,
};
struct swc_window_handler window_handler = {
	.destroy = on_win_destroy, .entered = on_win_entered,
};
struct swc_screen_handler screen_handler = {
	.destroy = on_screen_destroy,
	.usable_geometry_changed = on_screen_usable_geometry_changed,
};

static void focus(struct client* c, bool raise)
{
	if (swc_get_zoom() != 1.0f) {
		wm.zoom_target = 1.0f;
		if (!wm.zoom_timer)
			wm.zoom_timer = wl_event_loop_add_timer(wm.ev_loop, zoom_tick, NULL);
		if (wm.zoom_timer)
			wl_event_source_timer_update(wm.zoom_timer, 1);
	}
	if (wm.sel_client)
		swc_window_set_border(
			wm.sel_client->win,
			cfg.border_col_normal, cfg.border_width,
			cfg.outer_border_col_normal, cfg.outer_border_width
		);

	if (c)
		swc_window_set_border(
			c->win,
			cfg.border_col_active, cfg.border_width,
			cfg.outer_border_col_active, cfg.outer_border_width
		);

	if (raise && c && c->floating)
		set_floating(c, true, true);

	swc_window_focus(c ? c->win : NULL);
	wm.sel_client = c;
}

static void on_screen_destroy(void* data)
{
	struct screen* s = data;

	if (!s)
		return;

	wl_list_remove(&s->link);

	if (wm.sel_screen == s) {
		if (wl_list_empty(&wm.screens))
			wm.sel_screen = NULL;
		else
			wm.sel_screen = wl_container_of(wm.screens.next, wm.sel_screen, link);
	}

	free(s);
}

static void on_screen_usable_geometry_changed(void* data)
{
	struct screen* s = data;

	tile(s);
}

static void on_win_destroy(void* data)
{
	struct client* c = data;
	struct client* next;

	if (!c)
		return;

	if (wm.grab.active && wm.grab.c == c) {
		wm.grab.active = false;
		wm.grab.c = NULL;
	}

	if (c->floating)
		wl_list_remove(&c->float_link);
	else
		wl_list_remove(&c->tiled_link);

	if (wm.sel_client == c) {
		wm.sel_client = NULL;
		next = first_float(c->scr);
		if (!next)
			next = first_tiled(c->scr);
		focus(next, true);
	}

	tile(c->scr);
	free(c);
}

static void on_win_entered(void* data)
{
	if (wm.grab.active)
		return;

	struct client* c = data;
	if (!c || c->ws != wm.ws)
		return;

	if (swc_get_zoom() != 1.0f)
		return;

	focus(c, false);
}

static void setup_cursor(void)
{
	const struct nein_cursor_meta *arrow = &nein_cursor_metadata[NEIN_CURSOR_WHITEARROW];

	swc_set_cursor_mode(SWC_CURSOR_MODE_COMPOSITOR);
	swc_set_cursor_image(SWC_CURSOR_DEFAULT,
		&nein_cursor_data[arrow->offset],
		arrow->width, arrow->height,
		arrow->hotspot_x, arrow->hotspot_y);
}

static void setup(void)
{
	wm.dpy = wl_display_create();
	if (!wm.dpy)
		die(EXIT_FAILURE, "wl_display_create failed");

	wl_list_init(&wm.screens);
	wl_list_init(&wm.tiled);
	wl_list_init(&wm.floating);
	wm.sel_client       = NULL;
	wm.sel_screen       = NULL;
	wm.grab.active      = false;
	wm.grab.resize      = false;
	wm.grab.c           = NULL;
	wm.global_floating  = false;
	wm.ws               = 1;
	wm.zoom_target      = 1.0f;
	wm.zoom_timer       = NULL;
	wm.pan_active       = false;
	wm.pan_last_x       = 0;
	wm.pan_last_y       = 0;
	wm.pan_timer        = NULL;
	wm.draw_mode        = false;
	wm.spawn_pending    = false;
	wm.selecting        = false;
	wm.sel_start_x      = 0;
	wm.sel_start_y      = 0;
	wm.sel_timer        = NULL;
	wm.repeat_timer     = NULL;
	wm.repeat_fn        = NULL;
	wm.repeat_arg_i     = 0;

	wm.ev_loop = wl_display_get_event_loop(wm.dpy);
	if (!swc_initialize(wm.dpy, wm.ev_loop, &manager))
		die(EXIT_FAILURE, "swc_initialize failed\n");

	setup_cursor();
	setup_binds();

	const char* sock;
	sock = wl_display_add_socket_auto(wm.dpy);
	if (!sock)
		die(EXIT_FAILURE, "wl_display_add_socket_auto failed\n");
	setenv("WAYLAND_DISPLAY", sock, 1);
	_log(stderr, "WAYLAND_DISPLAY=%s\n", sock);

	signal(SIGINT,  sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGQUIT, sig_handler);

    autostart();
}

static void setup_binds(void)
{
	for (size_t i = 0; i < LENGTH(binds); i++) {
		const struct bind* b = &binds[i];
		swc_add_binding(b->type, b->mods, b->ksym, b->fn, (void*)&b->arg);
	}
	swc_add_axis_binding(MOD4, 0, zoom_axis, NULL);
	swc_add_binding(SWC_BINDING_BUTTON, SWC_MOD_ANY, BTN_LEFT, mouse_click, NULL);
}

static void autostart(void)
{
	const char* progs[] = { AUTOSTART };
	for (size_t i = 0; i < sizeof(progs) / sizeof(progs[0]); i++) {
		pid_t pid = fork();
		if (pid == 0) {
			setsid();
			if (fork() == 0) {
				execl("/bin/sh", "sh", "-c", progs[i], NULL);
				_exit(1);
			}
			_exit(0);
		} else if (pid > 0) {
			waitpid(pid, NULL, 0);
		}
	}
}

static void set_floating(struct client* c, bool floating, bool raise)
{
	if (!c)
		return;

	if (floating) {
		if (!c->floating) {
			c->floating = true;
			wl_list_remove(&c->tiled_link);
			wl_list_insert(&wm.floating, &c->float_link);

			struct swc_rectangle geom;
			if (c->saved_float_w > 0) {
				geom.x      = c->saved_float_x;
				geom.y      = c->saved_float_y;
				geom.width  = c->saved_float_w;
				geom.height = c->saved_float_h;
			} else {
				int slot = next_cascade_slot();
				geom.x      = cfg.float_x + slot * 100;
				geom.y      = cfg.float_y + slot * 100;
				geom.width  = cfg.float_w;
				geom.height = cfg.float_h;
			}
			swc_window_set_geometry(c->win, &geom);
		} else if (raise) {
			wl_list_remove(&c->float_link);
			wl_list_insert(&wm.floating, &c->float_link);
		}

		swc_window_set_stacked(c->win);
	} else {
		if (c->floating) {
			struct swc_rectangle geom;
			swc_window_get_geometry(c->win, &geom);
			c->saved_float_x = geom.x;
			c->saved_float_y = geom.y;
			c->saved_float_w = geom.width;
			c->saved_float_h = geom.height;

			c->floating = false;
			wl_list_remove(&c->float_link);
			wl_list_insert(&wm.tiled, &c->tiled_link);
		}

		swc_window_set_tiled(c->win);
	}
}

static void tile(struct screen* s)
{
	struct      client* c;
	struct      screen* screen;
	struct      swc_rectangle geom;
	struct      swc_rectangle* scr_geom;
	int32_t     out_gaps = cfg.gaps + cfg.border_width + cfg.outer_border_width;
	int32_t     in_gaps  = cfg.gaps + (cfg.border_width + cfg.outer_border_width) * 2;
	int32_t     x;
	int32_t     y;
	uint32_t    w;
	uint32_t    h;

	if (!s) {
		wl_list_for_each(screen, &wm.screens, link)
			tile(screen);
		return;
	}

	scr_geom = &s->scr->usable_geometry;

	size_t n = 0;
	wl_list_for_each(c, &wm.tiled, tiled_link) {
		if (is_tiled(c, s))
			n++;
	}
	if (n == 0)
		return;

	x = scr_geom->x + out_gaps;
	y = scr_geom->y + out_gaps;
	w = scr_geom->width  - (uint32_t)(out_gaps * 2);
	h = scr_geom->height - (uint32_t)(out_gaps * 2);

	if (n == 1) {
		wl_list_for_each(c, &wm.tiled, tiled_link) {
			if (!is_tiled(c, s))
				continue;

			geom.x = x;
			geom.y = y;
			geom.width  = w;
			geom.height = h;

			swc_window_set_geometry(c->win, &geom);
			return;
		}
	}

	size_t i = 0;
	int32_t stack_y = y;
	wl_list_for_each(c, &wm.tiled, tiled_link) {
		if (!is_tiled(c, s))
			continue;

		if (master_width == 0)
			master_width = ((w - (uint32_t)in_gaps) * cfg.master_width) / 100;

		if (i == 0) {
			geom.x = x;
			geom.y = y;
			geom.width  = master_width;
			geom.height = h;
		} else {
			uint32_t stackn = (uint32_t)(n - 1);
			uint32_t idx    = (uint32_t)(i - 1);
			uint32_t base_h = (h - (uint32_t)in_gaps * (stackn - 1)) / stackn;
			int32_t win_h;

			if (idx == stackn - 1) {
				win_h = (int32_t)(y + (int32_t)h - stack_y);
			} else {
				win_h = (int32_t)base_h + c->height_bias;
			}
			if (win_h < 20) win_h = 20;

			geom.x      = x + (int32_t)master_width + in_gaps;
			geom.y      = stack_y;
			geom.width  = w - master_width - (uint32_t)in_gaps;
			geom.height = (uint32_t)win_h;
			stack_y    += win_h + in_gaps;
		}

		swc_window_set_geometry(c->win, &geom);
		i++;
	}
}

static const int zoom_timerms   = 16;
static const int repeat_delay_ms     = 200;
static const int repeat_rate_ms      = 16;  /* floating ops: ~60fps */
static const int repeat_tile_rate_ms = 50;  /* tiling ops: ~20fps, gives clients time to respond */

static int zoom_tick(void* data)
{
	(void)data;
	float current = swc_get_zoom();
	float diff = wm.zoom_target - current;

	if (diff > -0.01f && diff < 0.01f) {
		swc_set_zoom(wm.zoom_target);
		if (wm.zoom_timer) {
			wl_event_source_remove(wm.zoom_timer);
			wm.zoom_timer = NULL;
		}
		sync_window_visibility();
		return 0;
	}

	float step = diff / 4.0f;
	if (step > 0 && step < 0.01f) step = 0.01f;
	if (step < 0 && step > -0.01f) step = -0.01f;

	swc_set_zoom(current + step);
	wl_event_source_timer_update(wm.zoom_timer, zoom_timerms);
	return 0;
}

void zoom_axis(void* data, uint32_t time, uint32_t axis, int32_t value120)
{
	(void)data;
	(void)time;
	(void)axis;

	float delta = (value120 < 0) ? 0.1f : -0.1f;
	wm.zoom_target += delta;
	if (wm.zoom_target < 0.1f) wm.zoom_target = 0.1f;
	if (wm.zoom_target > 4.0f) wm.zoom_target = 4.0f;

	if (!wm.zoom_timer)
		wm.zoom_timer = wl_event_loop_add_timer(wm.ev_loop, zoom_tick, NULL);
	if (wm.zoom_timer)
		wl_event_source_timer_update(wm.zoom_timer, 1);
}

static int pan_drag_tick(void* data)
{
	(void)data;

	if (!wm.pan_active) return 0;

	int32_t fx, fy;
	if (!swc_cursor_position(&fx, &fy)) {
		wl_event_source_timer_update(wm.pan_timer, 16);
		return 0;
	}

	int32_t x = wl_fixed_to_int(fx);
	int32_t y = wl_fixed_to_int(fy);
	int32_t dx = x - wm.pan_last_x;
	int32_t dy = y - wm.pan_last_y;
	wm.pan_last_x = x;
	wm.pan_last_y = y;

	if (dx != 0 || dy != 0) {
		struct client* c;
		wl_list_for_each(c, &wm.floating, float_link) {
			struct swc_rectangle geom;
			swc_window_get_geometry(c->win, &geom);
			geom.x += dx;
			geom.y += dy;
			swc_window_set_geometry(c->win, &geom);
		}
	}

	wl_event_source_timer_update(wm.pan_timer, 16);
	return 0;
}

static int sel_tick(void* data)
{
	(void)data;
	if (!wm.selecting) return 0;
	int32_t fx, fy;
	if (!swc_cursor_position(&fx, &fy)) {
		wl_event_source_timer_update(wm.sel_timer, 16);
		return 0;
	}
	int32_t x = wl_fixed_to_int(fx);
	int32_t y = wl_fixed_to_int(fy);
	swc_overlay_set_box(wm.sel_start_x, wm.sel_start_y, x, y,
		0xffff69b4, 2);
	wl_event_source_timer_update(wm.sel_timer, 16);
	return 0;
}

static int repeat_tick(void* data)
{
	(void)data;
	if (!wm.repeat_fn) {
		if (wm.repeat_timer) {
			wl_event_source_remove(wm.repeat_timer);
			wm.repeat_timer = NULL;
		}
		return 0;
	}
	union arg a = { .i = wm.repeat_arg_i };
	wm.repeat_fn(&a, 0, 0, WL_KEYBOARD_KEY_STATE_PRESSED);
	wl_event_source_timer_update(wm.repeat_timer, wm.repeat_interval);
	return 0;
}

static void repeat_start(void (*fn)(void*, uint32_t, uint32_t, uint32_t), int32_t arg_i, int interval_ms)
{
	if (wm.repeat_fn == fn && wm.repeat_arg_i == arg_i)
		return;
	wm.repeat_fn       = fn;
	wm.repeat_arg_i    = arg_i;
	wm.repeat_interval = interval_ms;
	if (!wm.repeat_timer)
		wm.repeat_timer = wl_event_loop_add_timer(wm.ev_loop, repeat_tick, NULL);
	if (wm.repeat_timer)
		wl_event_source_timer_update(wm.repeat_timer, repeat_delay_ms);
}

static void repeat_stop(void (*fn)(void*, uint32_t, uint32_t, uint32_t), int32_t arg_i)
{
	if (wm.repeat_fn != fn || wm.repeat_arg_i != arg_i)
		return;
	wm.repeat_fn = NULL;
	if (wm.repeat_timer) {
		wl_event_source_remove(wm.repeat_timer);
		wm.repeat_timer = NULL;
	}
}

void focus_next(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	struct client* c;
	struct screen* s;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_screen)
		return;

	s = wm.sel_screen;

	if (!wm.sel_client) {
		c = first_float(s);
		if (!c)
			c = first_tiled(s);
		focus(c, false);
		return;
	}

	if (wm.sel_client->floating) {
		for (c = wl_container_of(wm.sel_client->float_link.next, c, float_link);
			&c->float_link != &wm.floating;
			c = wl_container_of(c->float_link.next, c, float_link)) {
			if (is_float(c, s)) {
				focus(c, false);
				return;
			}
		}

		c = first_tiled(s);
		if (!c)
			c = first_float(s);
		if (c)
			focus(c, false);
		return;
	}

	for (c = wl_container_of(wm.sel_client->tiled_link.next, c, tiled_link);
		&c->tiled_link != &wm.tiled;
		c = wl_container_of(c->tiled_link.next, c, tiled_link)) {
		if (is_tiled(c, s)) {
			focus(c, false);
			return;
		}
	}

	c = first_float(s);
	if (!c)
		c = first_tiled(s);
	focus(c, false);
}

void focus_prev(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	struct client* c;
	struct screen* s;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_screen)
		return;

	s = wm.sel_screen;

	if (!wm.sel_client) {
		c = first_float(s);
		if (!c)
			c = first_tiled(s);
		focus(c, false);
		return;
	}

	if (wm.sel_client->floating) {
		for (c = wl_container_of(wm.sel_client->float_link.prev, c, float_link);
			&c->float_link != &wm.floating;
			c = wl_container_of(c->float_link.prev, c, float_link)) {
			if (is_float(c, s)) {
				focus(c, false);
				return;
			}
		}

		c = last_tiled(s);
		if (!c)
			c = last_float(s);
		if (c)
			focus(c, false);
		return;
	}

	for (c = wl_container_of(wm.sel_client->tiled_link.prev, c, tiled_link);
		&c->tiled_link != &wm.tiled;
		c = wl_container_of(c->tiled_link.prev, c, tiled_link)) {
		if (is_tiled(c, s)) {
			focus(c, false);
			return;
		}
	}

	c = last_float(s);
	if (!c)
		c = last_tiled(s);
	focus(c, false);
}

void master_next(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	struct client* first;
	struct client* last;
	struct screen* s;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_screen)
		return;

	s = wm.sel_screen;
	first = first_tiled(s);
	last = last_tiled(s);

	if (!first || !last || first == last)
		return;

	wl_list_remove(&last->tiled_link);
	wl_list_insert(first->tiled_link.prev, &last->tiled_link);
	focus(first_tiled(s), true);
	tile(s);
}

void master_prev(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	struct client* first;
	struct client* last;
	struct screen* s;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_screen)
		return;

	s = wm.sel_screen;
	first = first_tiled(s);
	last = last_tiled(s);

	if (!first || !last || first == last)
		return;

	wl_list_remove(&first->tiled_link);
	wl_list_insert(&last->tiled_link, &first->tiled_link);
	focus(first_tiled(s), true);
	tile(s);
}

void kill_sel(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_client)
		return;

	swc_window_close(wm.sel_client->win);
}

void master_resize(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(master_resize, a->i);
		return;
	}

	if (!wm.sel_screen)
		return;

	{
		struct swc_rectangle* g = &wm.sel_screen->scr->usable_geometry;

		uint32_t out_gaps = cfg.gaps + cfg.border_width + cfg.outer_border_width;
		uint32_t in_gaps  = cfg.gaps + (cfg.border_width + cfg.outer_border_width) * 2;

		int32_t total = (int32_t)g->width - (int32_t)(out_gaps * 2);

		int32_t min_master = 20;
		int32_t max_master = total - (int32_t)in_gaps - 20;

		if (max_master < min_master)
			max_master = min_master;

		int32_t mw = (int32_t)master_width + a->i;

		if (mw < min_master)
			mw = max_master;
		else if (mw > max_master)
			mw = min_master;

		master_width = (uint32_t)mw;
	}

	tile(wm.sel_screen);
	repeat_start(master_resize, a->i, repeat_tile_rate_ms);
}

void stack_resize(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(stack_resize, a->i);
		return;
	}

	if (!wm.sel_screen || !wm.sel_client || wm.sel_client->floating)
		return;

	struct client* first = first_tiled(wm.sel_screen);
	if (!first || wm.sel_client == first)
		return;

	wm.sel_client->height_bias += a->i;
	tile(wm.sel_screen);
	repeat_start(stack_resize, a->i, repeat_tile_rate_ms);
}

void draw_mode_toggle(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;
	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;
	wm.draw_mode = !wm.draw_mode;
}

void mouse_click(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)value;

	int32_t fx, fy;
	if (!swc_cursor_position(&fx, &fy)) {
		swc_pointer_send_button(time, BTN_LEFT, state);
		return;
	}

	int32_t x = wl_fixed_to_int(fx);
	int32_t y = wl_fixed_to_int(fy);

	if (wm.draw_mode) {
		if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
			wm.sel_start_x = x;
			wm.sel_start_y = y;
			wm.selecting = true;
			if (!wm.sel_timer)
				wm.sel_timer = wl_event_loop_add_timer(wm.ev_loop, sel_tick, NULL);
			if (wm.sel_timer)
				wl_event_source_timer_update(wm.sel_timer, 1);
		} else {
			if (!wm.selecting) return;
			wm.selecting = false;
			wm.draw_mode = false;
			if (wm.sel_timer) {
				wl_event_source_remove(wm.sel_timer);
				wm.sel_timer = NULL;
			}
			swc_overlay_clear();

			int32_t x1 = wm.sel_start_x < x ? wm.sel_start_x : x;
			int32_t y1 = wm.sel_start_y < y ? wm.sel_start_y : y;
			int32_t x2 = wm.sel_start_x < x ? x : wm.sel_start_x;
			int32_t y2 = wm.sel_start_y < y ? y : wm.sel_start_y;

			uint32_t w = (uint32_t)(x2 - x1);
			uint32_t h = (uint32_t)(y2 - y1);
			if (w < 100) w = 100;
			if (h < 50)  h = 50;

			wm.spawn_geometry.x      = x1;
			wm.spawn_geometry.y      = y1;
			wm.spawn_geometry.width  = w;
			wm.spawn_geometry.height = h;
			wm.spawn_pending = true;

			union arg a = { .v = termcmd };
			spawn(&a, 0, 0, WL_KEYBOARD_KEY_STATE_PRESSED);
		}
		return;
	}

	struct swc_window* target = swc_window_at(x, y);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED && target) {
		struct client* c;
		wl_list_for_each(c, &wm.floating, float_link) {
			if (c->win == target) {
				focus(c, true);
				break;
			}
		}
		wl_list_for_each(c, &wm.tiled, tiled_link) {
			if (c->win == target) {
				focus(c, true);
				break;
			}
		}
	}

	swc_pointer_send_button(time, BTN_LEFT, state);
}

void mouse_move(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		int32_t fx, fy;
		if (swc_cursor_position(&fx, &fy)) {
			int32_t x = wl_fixed_to_int(fx);
			int32_t y = wl_fixed_to_int(fy);
			struct swc_window* target = swc_window_at(x, y);

			if (!target) {
				wm.pan_active = true;
				wm.pan_last_x = x;
				wm.pan_last_y = y;
				if (!wm.pan_timer)
					wm.pan_timer = wl_event_loop_add_timer(wm.ev_loop, pan_drag_tick, NULL);
				if (wm.pan_timer)
					wl_event_source_timer_update(wm.pan_timer, 1);
				return;
			}

			struct client* c;
			bool found = false;
			wl_list_for_each(c, &wm.floating, float_link) {
				if (c->win == target) { found = true; break; }
			}
			if (!found) {
				wl_list_for_each(c, &wm.tiled, tiled_link) {
					if (c->win == target) { found = true; break; }
				}
			}
			if (found) {
				if (swc_get_zoom() != 1.0f) {
					if (wm.sel_client)
						swc_window_set_border(wm.sel_client->win,
							cfg.border_col_normal, cfg.border_width,
							cfg.outer_border_col_normal, cfg.outer_border_width);
					swc_window_set_border(c->win,
						cfg.border_col_active, cfg.border_width,
						cfg.outer_border_col_active, cfg.outer_border_width);
					swc_window_focus(c->win);
					wm.sel_client = c;
				} else {
					focus(c, true);
				}
			}
		}
		if (wm.sel_client)
			swc_window_begin_move(wm.sel_client->win);
	} else {
		if (wm.pan_active) {
			wm.pan_active = false;
			if (wm.pan_timer) {
				wl_event_source_remove(wm.pan_timer);
				wm.pan_timer = NULL;
			}
			return;
		}
		if (wm.sel_client)
			swc_window_end_move(wm.sel_client->win);
	}
}

void mouse_resize(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (!wm.sel_client)
			return;

		if (!wm.sel_client->floating) {
			set_floating(wm.sel_client, true, false);
			tile(wm.sel_client->scr);
		}

		wm.grab.active = true;
		wm.grab.resize = true;
		wm.grab.c = wm.sel_client;

		swc_window_begin_resize(
			wm.grab.c->win,
			SWC_WINDOW_EDGE_RIGHT | SWC_WINDOW_EDGE_BOTTOM
		);
	} else {
		if (!wm.grab.active || !wm.grab.resize || !wm.grab.c)
			return;

		swc_window_end_resize(wm.grab.c->win);

		wm.grab.active = false;
		wm.grab.c = NULL;
	}
}

void float_move_x(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_move_x, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	swc_window_get_geometry(wm.sel_client->win, &geom);
	geom.x += a->i;
	swc_window_set_geometry(wm.sel_client->win, &geom);
	repeat_start(float_move_x, a->i, repeat_rate_ms);
}

void float_move_y(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_move_y, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	swc_window_get_geometry(wm.sel_client->win, &geom);
	geom.y += a->i;
	swc_window_set_geometry(wm.sel_client->win, &geom);
	repeat_start(float_move_y, a->i, repeat_rate_ms);
}

void float_resize_w(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_resize_w, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	if (wm.repeat_fn == float_resize_w && wm.repeat_arg_i == a->i) {
		geom = wm.repeat_geom;
	} else {
		swc_window_get_geometry(wm.sel_client->win, &geom);
		wm.repeat_anchor_x = geom.x + (int32_t)geom.width;
	}

	geom.x -= a->i;
	geom.width = (uint32_t)(wm.repeat_anchor_x - geom.x);
	swc_window_set_geometry(wm.sel_client->win, &geom);
	wm.repeat_geom = geom;
	repeat_start(float_resize_w, a->i, repeat_rate_ms);
}

void float_resize_h(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_resize_h, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	if (wm.repeat_fn == float_resize_h && wm.repeat_arg_i == a->i) {
		geom = wm.repeat_geom;
	} else {
		swc_window_get_geometry(wm.sel_client->win, &geom);
		wm.repeat_anchor_y = geom.y + (int32_t)geom.height;
	}

	geom.y -= a->i;
	geom.height = (uint32_t)(wm.repeat_anchor_y - geom.y);
	swc_window_set_geometry(wm.sel_client->win, &geom);
	wm.repeat_geom = geom;
	repeat_start(float_resize_h, a->i, repeat_rate_ms);
}

void float_resize_w_right(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_resize_w_right, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	swc_window_get_geometry(wm.sel_client->win, &geom);
	geom.width = (uint32_t)((int32_t)geom.width + a->i);
	swc_window_set_geometry(wm.sel_client->win, &geom);
	repeat_start(float_resize_w_right, a->i, repeat_rate_ms);
}

void float_resize_h_bottom(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_resize_h_bottom, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	swc_window_get_geometry(wm.sel_client->win, &geom);
	geom.height = (uint32_t)((int32_t)geom.height + a->i);
	swc_window_set_geometry(wm.sel_client->win, &geom);
	repeat_start(float_resize_h_bottom, a->i, repeat_rate_ms);
}

void float_scale(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		repeat_stop(float_scale, a->i);
		return;
	}

	if (!wm.sel_client || !wm.sel_client->floating)
		return;

	struct swc_rectangle geom;
	if (wm.repeat_fn == float_scale && wm.repeat_arg_i == a->i) {
		geom = wm.repeat_geom;
	} else {
		swc_window_get_geometry(wm.sel_client->win, &geom);
		wm.repeat_anchor_x = geom.x + (int32_t)geom.width  / 2;
		wm.repeat_anchor_y = geom.y + (int32_t)geom.height / 2;
	}

	geom.width  = (uint32_t)((int32_t)geom.width  + a->i);
	geom.height = (uint32_t)((int32_t)geom.height + a->i);
	geom.x = wm.repeat_anchor_x - (int32_t)geom.width  / 2;
	geom.y = wm.repeat_anchor_y - (int32_t)geom.height / 2;
	swc_window_set_geometry(wm.sel_client->win, &geom);
	wm.repeat_geom = geom;
	repeat_start(float_scale, a->i, repeat_rate_ms);
}

void new_screen(struct swc_screen* scr)
{
	struct screen* s;

	s = malloc(sizeof(*s));
	if (!s)
		die(EXIT_FAILURE, "new screen malloc failed");

	s->scr = scr;
	s->x = 0;
	s->y = 0;
	s->w = 0;
	s->h = 0;

	wl_list_insert(&wm.screens, &s->link);

	if (!wm.sel_screen)
		wm.sel_screen = s;

	swc_screen_set_handler(scr, &screen_handler, s);

	_log(stderr, "new_screen=%p\n", (void*)scr);
}

static int next_cascade_slot(void)
{
	struct client* c;
	for (int n = 0; n < 50; n++) {
		int32_t sx = cfg.float_x + n * 100;
		int32_t sy = cfg.float_y + n * 100;
		bool occupied = false;
		wl_list_for_each(c, &wm.floating, float_link) {
			struct swc_rectangle geom;
			swc_window_get_geometry(c->win, &geom);
			if (abs(geom.x - sx) < 30 && abs(geom.y - sy) < 30) {
				occupied = true;
				break;
			}
		}
		if (!occupied)
			return n;
	}
	return 0;
}

void new_window(struct swc_window* win)
{
	struct client* c;

	c = malloc(sizeof(*c));
	if (!c)
		die(EXIT_FAILURE, "malloc client failed");

	win->motion_throttle_ms = 1000 / cfg.motion_throttle_hz;
	win->min_width		= 20;
	win->min_height		= 20;
	win->max_width		= 0;
	win->max_height		= 0;

	c->win			= win;
	c->scr			= wm.sel_screen;
	c->mapped		= false;
	c->floating		= wm.global_floating;
	c->fullscreen		= false;
	c->ws			= wm.ws;
	c->x			= 0;
	c->y			= 0;
	c->w			= 0;
	c->h			= 0;
	c->height_bias		= 0;
	c->saved_float_x	= 0;
	c->saved_float_y	= 0;
	c->saved_float_w	= 0;
	c->saved_float_h	= 0;

	if (c->floating) {
		wl_list_insert(&wm.floating, &c->float_link);
		wl_list_init(&c->tiled_link);
		swc_window_set_handler(win, &window_handler, c);
		swc_window_set_stacked(win);
		if (wm.spawn_pending) {
			struct swc_rectangle geom = wm.spawn_geometry;
			wm.spawn_pending = false;
			swc_window_set_geometry(win, &geom);
		} else {
			int slot = next_cascade_slot();
			swc_window_set_position(win,
				cfg.float_x + slot * 100,
				cfg.float_y + slot * 100);
		}
	} else {
		wl_list_insert(&wm.tiled, &c->tiled_link);
		wl_list_init(&c->float_link);
		swc_window_set_handler(win, &window_handler, c);
		swc_window_set_tiled(win);
	}

	swc_window_show(win);
	focus(c, true);
	tile(wm.sel_screen);

	_log(stderr, "new_window=%p\n", (void*)win);
}

void new_device(struct libinput_device* dev)
{
	if (!libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER))
		return;

	/* tapping */
	if (libinput_device_config_tap_get_finger_count(dev) > 0) {
		libinput_device_config_tap_set_enabled(dev, LIBINPUT_CONFIG_TAP_ENABLED);
		libinput_device_config_tap_set_button_map(dev, LIBINPUT_CONFIG_TAP_MAP_LRM);
	}

	/* natural scrolling */
	if (libinput_device_config_scroll_has_natural_scroll(dev))
		libinput_device_config_scroll_set_natural_scroll_enabled(dev, 1);

	/* clickfinger */
	uint32_t methods = libinput_device_config_click_get_methods(dev);
	if (methods & LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER)
		libinput_device_config_click_set_method(dev, LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER);

	/* disable while typing */
	if (libinput_device_config_dwt_is_available(dev))
		libinput_device_config_dwt_set_enabled(dev, LIBINPUT_CONFIG_DWT_ENABLED);
}

void quit(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;
	(void)state;

	wl_display_terminate(wm.dpy);
}

void spawn(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	union arg* a = data;
	char* const* cmd = (char* const*)a->v;

	(void)time;
	(void)value;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (fork() == 0) {
		execvp(cmd[0], cmd);
		_exit(127);
	}
}

void toggle_float(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_client)
		return;

	set_floating(wm.sel_client, !wm.sel_client->floating, true);
	tile(wm.sel_client->scr);
}

void toggle_float_global(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	wm.global_floating = !wm.global_floating;

	FILE* f = fopen("/tmp/neuwm_mode", "w");
	if (f) {
		fprintf(f, "%s\n", wm.global_floating ? "float" : "tile");
		fclose(f);
	}

	struct client* c;
	struct client* tmp;

	if (wm.global_floating) {
		wl_list_for_each_safe(c, tmp, &wm.tiled, tiled_link) {
			set_floating(c, true, false);
		}
	} else {
		wl_list_for_each_safe(c, tmp, &wm.floating, float_link) {
			set_floating(c, false, false);
		}
	}

	tile(wm.sel_screen);
}

void fullscreen(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)data;
	(void)time;
	(void)value;

	struct swc_rectangle geom;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_client)
		return;

	if (wm.sel_client->fullscreen) {
		wm.sel_client->fullscreen = false;
		swc_window_set_stacked(wm.sel_client->win);
		if (wm.sel_client->w > 0 && wm.sel_client->h > 0) {
			geom.x = wm.sel_client->x;
			geom.y = wm.sel_client->y;
			geom.width  = wm.sel_client->w;
			geom.height = wm.sel_client->h;
			swc_window_set_geometry(wm.sel_client->win, &geom);
		}
		return;
	}

	wm.sel_client->fullscreen = true;
	struct swc_rectangle saved;
	swc_window_get_geometry(wm.sel_client->win, &saved);
	wm.sel_client->x = saved.x;
	wm.sel_client->y = saved.y;
	wm.sel_client->w = saved.width;
	wm.sel_client->h = saved.height;
	swc_window_set_fullscreen(wm.sel_client->win, wm.sel_client->scr->scr);
}

void workspace_goto(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;
	struct client* c;
	struct screen* s;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (a->u == wm.ws)
		return;

	wm.ws = a->u;
	sync_window_visibility();

	if (!wm.sel_screen)
		return;

	s = wm.sel_screen;

	c = first_float(s);
	if (!c)
		c = first_tiled(s);
	focus(c, true);
	tile(NULL);
}

void workspace_moveto(void* data, uint32_t time, uint32_t value, uint32_t state)
{
	(void)time;
	(void)value;

	union arg* a = data;
	struct client* c;
	struct client* next;

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (!wm.sel_client)
		return;

	c = wm.sel_client;

	if (c->ws == a->u)
		return;

	c->ws = a->u;
	if (c->ws == wm.ws)
		swc_window_show(c->win);
	else
		swc_window_hide(c->win);

	next = NULL;
	if (wm.sel_screen) {
		next = first_float(wm.sel_screen);
		if (!next)
			next = first_tiled(wm.sel_screen);
	}

	focus(next, true);
	tile(NULL);
}

int main(void)
{
	setup();
	wl_display_run(wm.dpy);
	swc_finalize();
	wl_display_destroy(wm.dpy);
	return EXIT_SUCCESS;
}
