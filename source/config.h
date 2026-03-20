#ifndef CONFIG_H
#define CONFIG_H

#include <xkbcommon/xkbcommon-keysyms.h>

#include "types.h"
#include "neuwm.h"
#include "nein_cursor.h"

static const struct config cfg = {
	.motion_throttle_hz        = 85,
	.border_col_active         = 0xffc678dd,
	.border_col_normal         = 0xff444444,
	.border_width              = 1,
	.outer_border_col_active   = 0xff222222,
	.outer_border_col_normal   = 0xff222222,
	.outer_border_width        = 4,
	.master_width              = 60,
	.gaps                      = 10,
	.float_x                   = 100,
	.float_y                   = 100,
	.float_w                   = 800,
	.float_h                   = 500,
	.zoom_level                = 0.5f,
};

static const char* termcmd[] = { "st-wl", NULL };
static const char* menucmd[] = { "neumenu_run", "-fn", "tadepe", "-nb", "#000000", "-sb", "#c678dd", NULL };
static const char* btoggle[] = { "pkill", "-USR1", "mojito", NULL };
static struct bind binds[] = {

	/* keyboard */
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Return, { .v = termcmd }, spawn },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_space,  { .v = menucmd }, spawn },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_b,      { .v = btoggle }, spawn },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_e,      { .v = NULL },    quit },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Tab,    { .v = NULL },    focus_next },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_Tab,    { .v = NULL },    focus_prev },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_q,      { .v = NULL },    kill_sel },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_t,      { .v = NULL },    toggle_float },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_t,      { .v = NULL },    toggle_float_global },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_f,      { .v = NULL },    fullscreen },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_l,      { .i = 50 },      master_resize },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_h,      { .i = -50 },     master_resize },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_k,      { .i =  50 },     stack_resize },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_j,      { .i = -50 },     stack_resize },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_k,      { .v = NULL },    master_next },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_j,      { .v = NULL },    master_prev },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_d,      { .v = NULL },    draw_mode_toggle },

	/* floating move */
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Left,   { .i = -100 },    float_move_x },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Right,  { .i =  100 },    float_move_x },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Up,     { .i = -100 },    float_move_y },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_Down,   { .i =  100 },    float_move_y },

	/* floating resize - left/top anchored (expands leftward/upward) */
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_Left,   { .i =  50 },     float_resize_w },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_Right,  { .i = -50 },     float_resize_w },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_Up,     { .i =  50 },     float_resize_h },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_Down,   { .i = -50 },     float_resize_h },

	/* floating resize - right/bottom anchored (expands rightward/downward) */
	{ SWC_BINDING_KEY,    MOD4|CTRL,  XKB_KEY_Left,   { .i = -50 },     float_resize_w_right },
	{ SWC_BINDING_KEY,    MOD4|CTRL,  XKB_KEY_Right,  { .i =  50 },     float_resize_w_right },
	{ SWC_BINDING_KEY,    MOD4|CTRL,  XKB_KEY_Up,     { .i = -50 },     float_resize_h_bottom },
	{ SWC_BINDING_KEY,    MOD4|CTRL,  XKB_KEY_Down,   { .i =  50 },     float_resize_h_bottom },

	/* floating scale - centered */
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_equal,  { .i =  50 },     float_scale },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_minus,  { .i = -50 },     float_scale },

	/* mouse */
	{ SWC_BINDING_BUTTON, MOD4,       BTN_LEFT,       { .v = NULL },    mouse_move },
	{ SWC_BINDING_BUTTON, MOD4,       BTN_RIGHT,      { .v = NULL },    mouse_resize },

	/* workspaces */
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_1,      { .u = 1 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_2,      { .u = 2 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_3,      { .u = 3 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_4,      { .u = 4 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_5,      { .u = 5 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_6,      { .u = 6 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_7,      { .u = 7 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_8,      { .u = 8 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4,       XKB_KEY_9,      { .u = 9 },       workspace_goto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_1,      { .u = 1 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_2,      { .u = 2 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_3,      { .u = 3 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_4,      { .u = 4 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_5,      { .u = 5 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_6,      { .u = 6 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_7,      { .u = 7 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_8,      { .u = 8 },       workspace_moveto },
	{ SWC_BINDING_KEY,    MOD4|SHFT,  XKB_KEY_9,      { .u = 9 },       workspace_moveto },
};

#endif /* CONFIG_H */
