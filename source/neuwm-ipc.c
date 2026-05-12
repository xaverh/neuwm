#include <stdio.h>
#include <stdlib.h>
#include <wayland-server.h>
#include <swc.h>
#include <neuipc/neuipc.h>

#include "neuwm-ipc.h"
#include "neuwm.h"
#include "lua-config.h"
#include "types.h"

#define IPC_SOCK "/tmp/neuwm.sock"

static uint32_t
workspace_max(void)
{
	struct client *c;
	uint32_t max = wm.ws;

	wl_list_for_each(c, &wm.tiled, tiled_link)
		if (c->ws > max) max = c->ws;

	wl_list_for_each(c, &wm.floating, float_link)
		if (c->ws > max) max = c->ws;

	for (size_t i = 0; i < nbinds; i++)
		if ((binds[i].fn == workspace_goto || binds[i].fn == workspace_moveto)
				&& binds[i].arg.u > max)
			max = binds[i].arg.u;

	return max;
}

static swc_ipc_status
q_get_workspace(char **args, void *userdata)
{
	(void)args; (void)userdata;
	swc_ipc_status s = { .ok = true };
	snprintf(s.msg, sizeof(s.msg), "%u %u", wm.ws, workspace_max());
	return s;
}

static swc_ipc_status
q_get_workspaces(char **args, void *userdata)
{
	(void)args; (void)userdata;
	swc_ipc_status s = { .ok = true };

	uint32_t max = workspace_max();
	uint8_t *has_window = calloc(max + 1, 1);
	if (!has_window) return s;

	struct client *c;
	wl_list_for_each(c, &wm.tiled, tiled_link)
		if (c->ws <= max) has_window[c->ws] = 1;
	wl_list_for_each(c, &wm.floating, float_link)
		if (c->ws <= max) has_window[c->ws] = 1;

	int len = snprintf(s.msg, sizeof(s.msg), "%u", wm.ws);
	for (uint32_t i = 1; i <= max && len < (int)sizeof(s.msg) - 4; i++) {
		if (i != wm.ws && has_window[i])
			len += snprintf(s.msg + len, sizeof(s.msg) - len, " %u", i);
	}

	free(has_window);
	return s;
}

static swc_ipc_status
q_get_mode(char **args, void *userdata)
{
	(void)args; (void)userdata;
	swc_ipc_status s = { .ok = true };
	snprintf(s.msg, sizeof(s.msg), "%s", wm.global_floating ? "float" : "tile");
	return s;
}

static swc_ipc_status
q_get_cursor(char **args, void *userdata)
{
	(void)args; (void)userdata;
	swc_ipc_status s = { .ok = true };
	int32_t fx, fy;
	if (swc_cursor_position(&fx, &fy)) {
		snprintf(s.msg, sizeof(s.msg), "%d %d",
			wl_fixed_to_int(fx), wl_fixed_to_int(fy));
	} else {
		s.ok = false;
		snprintf(s.msg, sizeof(s.msg), "no cursor");
	}
	return s;
}

static swc_ipc_status
q_get_focused(char **args, void *userdata)
{
	(void)args; (void)userdata;
	swc_ipc_status s = { .ok = true };
	if (!wm.sel_client) {
		s.ok = false;
		snprintf(s.msg, sizeof(s.msg), "no focused window");
		return s;
	}
	struct swc_rectangle geom;
	swc_window_get_geometry(wm.sel_client->win, &geom);
	snprintf(s.msg, sizeof(s.msg), "%d,%d %ux%u",
		geom.x, geom.y, geom.width, geom.height);
	return s;
}

static int
ipc_callback(int fd, uint32_t mask, void *data)
{
	(void)fd; (void)mask; (void)data;
	swc_ipc_dispatch();
	return 0;
}

void
ipc_init(void)
{
	if (swc_ipc_init(IPC_SOCK) < 0)
		return;

	swc_ipc_register("get_workspace",  q_get_workspace,  NULL);
	swc_ipc_register("get_workspaces", q_get_workspaces, NULL);
	swc_ipc_register("get_mode",       q_get_mode,       NULL);
	swc_ipc_register("get_cursor",     q_get_cursor,     NULL);
	swc_ipc_register("get_focused",   q_get_focused,    NULL);

	wl_event_loop_add_fd(wm.ev_loop, swc_ipc_get_fd(),
		WL_EVENT_READABLE, ipc_callback, NULL);
}

void
ipc_finish(void)
{
	swc_ipc_finish();
}

static void
_emit_ws_cb(void *data)
{
	(void)data;
	swc_ipc_emit("ws", "ws_changed %u %u", wm.ws, workspace_max());
}

void
ipc_emit_ws(void)
{
	wl_event_loop_add_idle(wm.ev_loop, _emit_ws_cb, NULL);
}

static void
_emit_mode_cb(void *data)
{
	(void)data;
	swc_ipc_emit("mode", "mode_changed %s", wm.global_floating ? "float" : "tile");
}

void
ipc_emit_mode(void)
{
	wl_event_loop_add_idle(wm.ev_loop, _emit_mode_cb, NULL);
}
