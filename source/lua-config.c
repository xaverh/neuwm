#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <luajit-2.1/lua.h>
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lualib.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "types.h"
#include "neuwm.h"
#include "util.h"
#include "lua-config.h"

/* ------------------------------------------------------------------ globals */

struct config     cfg;
struct bind      *binds      = NULL;
size_t            nbinds     = 0;
struct axis_bind *axis_binds = NULL;
size_t            naxis_binds = 0;
char            **termcmd    = NULL;
char            **autostart_cmds  = NULL;
size_t            autostart_count = 0;

/* ----------------------------------------------------------- static defaults */

static char *s_termcmd[]  = { "st-wl", NULL };
static char *s_menucmd[]  = { "neumenu_run",
	"-fn", "tadepe",
	"-nb", "#000000",
	"-sb", "#c678dd",
	"-sf", "#000000",
	NULL };
static char *s_btoggle[]  = { "pkill", "-USR1", "mojito", NULL };

static char *s_autostart[] = {
	"sh $HOME/.config/mojito/status.sh"
	" | mojito -f \"Tadepe:style=Bold:pixelsize=16:antialias=false\""
	" -g 1900x20+10 &",
	"swall -m scaled $HOME/Pictures/wallpapers/rgb.png &",
};

static void set_defaults(void)
{
	cfg.motion_throttle_hz      = 85;
	cfg.border_col_active       = 0xffc678dd;
	cfg.border_col_normal       = 0xff444444;
	cfg.border_width            = 1;
	cfg.outer_border_col_active = 0xff222222;
	cfg.outer_border_col_normal = 0xff222222;
	cfg.outer_border_width      = 4;
	cfg.master_width            = 60;
	cfg.gaps                    = 10;
	cfg.float_x                 = 100;
	cfg.float_y                 = 100;
	cfg.float_w                 = 802;
	cfg.float_h                 = 500;
	cfg.zoom_level              = 0.5f;

	termcmd         = s_termcmd;
	autostart_cmds  = s_autostart;
	autostart_count = LENGTH(s_autostart);
}

static void build_default_binds(void)
{
#define K SWC_BINDING_KEY
#define B SWC_BINDING_BUTTON
	static struct bind db[] = {
		/* spawners */
		{ K, MOD4,      XKB_KEY_Return, { .v = s_termcmd  }, spawn },
		{ K, MOD4,      XKB_KEY_space,  { .v = s_menucmd  }, spawn },
		{ K, MOD4,      XKB_KEY_b,      { .v = s_btoggle  }, spawn },
		/* window management */
		{ K, MOD4|SHFT, XKB_KEY_e,      { .v = NULL }, quit              },
		{ K, MOD4,      XKB_KEY_Tab,    { .v = NULL }, focus_next        },
		{ K, MOD4|SHFT, XKB_KEY_Tab,    { .v = NULL }, focus_prev        },
		{ K, MOD4,      XKB_KEY_q,      { .v = NULL }, kill_sel          },
		{ K, MOD4,      XKB_KEY_t,      { .v = NULL }, toggle_float      },
		{ K, MOD4|SHFT, XKB_KEY_t,      { .v = NULL }, toggle_float_global },
		{ K, MOD4,      XKB_KEY_f,      { .v = NULL }, fullscreen        },
		{ K, MOD4,      XKB_KEY_d,      { .v = NULL }, draw_mode_toggle  },
		/* tiling */
		{ K, MOD4,      XKB_KEY_l,      { .i =  10 }, master_resize },
		{ K, MOD4,      XKB_KEY_h,      { .i = -10 }, master_resize },
		{ K, MOD4,      XKB_KEY_k,      { .i =  10 }, stack_resize  },
		{ K, MOD4,      XKB_KEY_j,      { .i = -10 }, stack_resize  },
		{ K, MOD4|SHFT, XKB_KEY_k,      { .v = NULL }, master_next  },
		{ K, MOD4|SHFT, XKB_KEY_j,      { .v = NULL }, master_prev  },
		/* floating move */
		{ K, MOD4,      XKB_KEY_Left,   { .i = -10 }, float_move_x },
		{ K, MOD4,      XKB_KEY_Right,  { .i =  10 }, float_move_x },
		{ K, MOD4,      XKB_KEY_Up,     { .i = -10 }, float_move_y },
		{ K, MOD4,      XKB_KEY_Down,   { .i =  10 }, float_move_y },
		/* floating resize left/top anchored */
		{ K, MOD4|SHFT, XKB_KEY_Left,   { .i =  6 }, float_resize_w },
		{ K, MOD4|SHFT, XKB_KEY_Right,  { .i = -6 }, float_resize_w },
		{ K, MOD4|SHFT, XKB_KEY_Up,     { .i =  6 }, float_resize_h },
		{ K, MOD4|SHFT, XKB_KEY_Down,   { .i = -6 }, float_resize_h },
		/* floating resize right/bottom anchored */
		{ K, MOD4|CTRL, XKB_KEY_Left,   { .i = -6 }, float_resize_w_right   },
		{ K, MOD4|CTRL, XKB_KEY_Right,  { .i =  6 }, float_resize_w_right   },
		{ K, MOD4|CTRL, XKB_KEY_Up,     { .i = -6 }, float_resize_h_bottom  },
		{ K, MOD4|CTRL, XKB_KEY_Down,   { .i =  6 }, float_resize_h_bottom  },
		/* floating scale */
		{ K, MOD4,      XKB_KEY_equal,  { .i =  6 }, float_scale },
		{ K, MOD4,      XKB_KEY_minus,  { .i = -6 }, float_scale },
		/* mouse */
		{ B, MOD4,        BTN_LEFT,  { .v = NULL }, mouse_move   },
		{ B, MOD4,        BTN_RIGHT, { .v = NULL }, mouse_resize },
		{ B, SWC_MOD_ANY, BTN_LEFT,  { .v = NULL }, mouse_click  },
		/* workspaces goto */
		{ K, MOD4,      XKB_KEY_1, { .u = 1 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_2, { .u = 2 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_3, { .u = 3 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_4, { .u = 4 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_5, { .u = 5 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_6, { .u = 6 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_7, { .u = 7 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_8, { .u = 8 }, workspace_goto },
		{ K, MOD4,      XKB_KEY_9, { .u = 9 }, workspace_goto },
		/* workspaces moveto */
		{ K, MOD4|SHFT, XKB_KEY_1, { .u = 1 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_2, { .u = 2 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_3, { .u = 3 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_4, { .u = 4 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_5, { .u = 5 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_6, { .u = 6 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_7, { .u = 7 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_8, { .u = 8 }, workspace_moveto },
		{ K, MOD4|SHFT, XKB_KEY_9, { .u = 9 }, workspace_moveto },
	};
#undef K
#undef B

	static struct axis_bind dab[] = {
		{ MOD4, zoom_axis },
	};

	binds      = db;
	nbinds     = LENGTH(db);
	axis_binds  = dab;
	naxis_binds = LENGTH(dab);
}

/* ---------------------------------------------------------------- Lua helpers */

static uint32_t parse_mods(const char *s)
{
	if (!s || s[0] == '\0') return 0;
	if (strcmp(s, "any") == 0) return SWC_MOD_ANY;
	uint32_t mods = 0;
	if (strstr(s, "mod4"))  mods |= MOD4;
	if (strstr(s, "mod1"))  mods |= MOD1;
	if (strstr(s, "shift")) mods |= SHFT;
	if (strstr(s, "ctrl"))  mods |= CTRL;
	return mods;
}

static uint32_t parse_key(const char *name)
{
	if (!name || name[0] == '\0') return 0;
	if (strcmp(name, "BTN_LEFT")   == 0) return BTN_LEFT;
	if (strcmp(name, "BTN_RIGHT")  == 0) return BTN_RIGHT;
	if (strcmp(name, "BTN_MIDDLE") == 0) return BTN_MIDDLE;
	uint32_t sym = xkb_keysym_from_name(name, XKB_KEYSYM_NO_FLAGS);
	if (sym != XKB_KEY_NoSymbol) return sym;
	return 0;
}

typedef void (*bind_fn_t)(void *, uint32_t, uint32_t, uint32_t);

static bind_fn_t parse_action(const char *name)
{
	if (!name) return NULL;
	if (strcmp(name, "spawn")                == 0) return spawn;
	if (strcmp(name, "quit")                 == 0) return quit;
	if (strcmp(name, "kill_sel")             == 0) return kill_sel;
	if (strcmp(name, "focus_next")           == 0) return focus_next;
	if (strcmp(name, "focus_prev")           == 0) return focus_prev;
	if (strcmp(name, "toggle_float")         == 0) return toggle_float;
	if (strcmp(name, "toggle_float_global")  == 0) return toggle_float_global;
	if (strcmp(name, "fullscreen")           == 0) return fullscreen;
	if (strcmp(name, "master_next")          == 0) return master_next;
	if (strcmp(name, "master_prev")          == 0) return master_prev;
	if (strcmp(name, "master_resize")        == 0) return master_resize;
	if (strcmp(name, "stack_resize")         == 0) return stack_resize;
	if (strcmp(name, "workspace_goto")       == 0) return workspace_goto;
	if (strcmp(name, "workspace_moveto")     == 0) return workspace_moveto;
	if (strcmp(name, "draw_mode_toggle")     == 0) return draw_mode_toggle;
	if (strcmp(name, "mouse_click")          == 0) return mouse_click;
	if (strcmp(name, "mouse_move")           == 0) return mouse_move;
	if (strcmp(name, "mouse_resize")         == 0) return mouse_resize;
	if (strcmp(name, "float_move_x")         == 0) return float_move_x;
	if (strcmp(name, "float_move_y")         == 0) return float_move_y;
	if (strcmp(name, "float_resize_w")       == 0) return float_resize_w;
	if (strcmp(name, "float_resize_h")       == 0) return float_resize_h;
	if (strcmp(name, "float_resize_w_right") == 0) return float_resize_w_right;
	if (strcmp(name, "float_resize_h_bottom")== 0) return float_resize_h_bottom;
	if (strcmp(name, "float_scale")          == 0) return float_scale;
	return NULL;
}

/* Parse a Lua array of strings at stack index idx into a NULL-terminated
   heap-allocated char** array.  Returns NULL on failure. */
static char **parse_string_array(lua_State *L, int idx)
{
	if (!lua_istable(L, idx)) return NULL;
	size_t n = lua_objlen(L, idx);
	char **arr = malloc((n + 1) * sizeof(char *));
	if (!arr) return NULL;
	for (size_t i = 1; i <= n; i++) {
		lua_rawgeti(L, idx, (int)i);
		const char *s = lua_tostring(L, -1);
		arr[i - 1] = strdup(s ? s : "");
		lua_pop(L, 1);
	}
	arr[n] = NULL;
	return arr;
}

static void parse_autostart(lua_State *L)
{
	/* expects the autostart table at stack top */
	size_t n = lua_objlen(L, -1);
	char **cmds = malloc((n + 1) * sizeof(char *));
	if (!cmds) return;
	size_t count = 0;
	for (size_t i = 1; i <= n; i++) {
		lua_rawgeti(L, -1, (int)i);
		if (lua_isstring(L, -1)) {
			const char *s = lua_tostring(L, -1);
			cmds[count++] = strdup(s);
		}
		lua_pop(L, 1);
	}
	autostart_cmds  = cmds;
	autostart_count = count;
}

static void parse_binds(lua_State *L)
{
	/* expects the binds table at stack top */
	size_t n = lua_objlen(L, -1);
	if (n == 0) {
		binds = NULL; nbinds = 0;
		axis_binds = NULL; naxis_binds = 0;
		return;
	}

	struct bind      *nb  = malloc(n * sizeof(struct bind));
	struct axis_bind *nab = malloc(n * sizeof(struct axis_bind));
	if (!nb || !nab) { free(nb); free(nab); return; }

	size_t bc = 0, ac = 0;

	for (size_t i = 1; i <= n; i++) {
		lua_rawgeti(L, -1, (int)i);
		/* entry table is now at -1 */

		/* [1] type */
		lua_rawgeti(L, -1, 1);
		char type[16] = "";
		if (lua_isstring(L, -1)) strncpy(type, lua_tostring(L, -1), sizeof(type) - 1);
		lua_pop(L, 1);

		/* [2] mods */
		lua_rawgeti(L, -1, 2);
		uint32_t mods = 0;
		if (lua_isstring(L, -1)) mods = parse_mods(lua_tostring(L, -1));
		lua_pop(L, 1);

		if (strcmp(type, "axis") == 0) {
			/* [3] action name */
			lua_rawgeti(L, -1, 3);
			char action[64] = "";
			if (lua_isstring(L, -1)) strncpy(action, lua_tostring(L, -1), sizeof(action) - 1);
			lua_pop(L, 1);

			if (strcmp(action, "zoom_axis") == 0) {
				nab[ac].mods = mods;
				nab[ac].fn   = zoom_axis;
				ac++;
			}

		} else if (strcmp(type, "key") == 0 || strcmp(type, "button") == 0) {
			uint32_t btype = (strcmp(type, "key") == 0)
				? SWC_BINDING_KEY : SWC_BINDING_BUTTON;

			/* [3] key/button name */
			lua_rawgeti(L, -1, 3);
			char keyname[64] = "";
			if (lua_isstring(L, -1)) strncpy(keyname, lua_tostring(L, -1), sizeof(keyname) - 1);
			lua_pop(L, 1);
			uint32_t ksym = parse_key(keyname);

			/* [4] action */
			lua_rawgeti(L, -1, 4);
			char action[64] = "";
			if (lua_isstring(L, -1)) strncpy(action, lua_tostring(L, -1), sizeof(action) - 1);
			lua_pop(L, 1);

			bind_fn_t fn = parse_action(action);
			if (!fn) { lua_pop(L, 1); continue; }

			struct bind b;
			b.type  = btype;
			b.mods  = mods;
			b.ksym  = ksym;
			b.fn    = fn;
			b.arg.v = NULL;

			/* [5] optional arg */
			lua_rawgeti(L, -1, 5);
			if (strcmp(action, "spawn") == 0) {
				if (lua_istable(L, -1)) {
					char **argv = parse_string_array(L, -1);
					if (argv) b.arg.v = argv;
				}
			} else if (lua_isnumber(L, -1)) {
				lua_Number num = lua_tonumber(L, -1);
				if (strcmp(action, "workspace_goto")   == 0 ||
				    strcmp(action, "workspace_moveto") == 0)
					b.arg.u = (uint32_t)num;
				else
					b.arg.i = (int)num;
			}
			lua_pop(L, 1); /* pop arg */

			nb[bc++] = b;
		}

		lua_pop(L, 1); /* pop entry */
	}

	binds      = nb;
	nbinds     = bc;
	axis_binds  = nab;
	naxis_binds = ac;
}

/* -- scalar readers -------------------------------------------------------- */

static void get_uint(lua_State *L, const char *name, uint32_t *dst)
{
	lua_getglobal(L, name);
	if (lua_isnumber(L, -1)) *dst = (uint32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

static void get_int(lua_State *L, const char *name, int32_t *dst)
{
	lua_getglobal(L, name);
	if (lua_isnumber(L, -1)) *dst = (int32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

static void get_float(lua_State *L, const char *name, float *dst)
{
	lua_getglobal(L, name);
	if (lua_isnumber(L, -1)) *dst = (float)lua_tonumber(L, -1);
	lua_pop(L, 1);
}

/* Colors can be Lua numbers (0xffc678dd) or hex strings ("0xffc678dd"). */
static void get_color(lua_State *L, const char *name, uint32_t *dst)
{
	lua_getglobal(L, name);
	if (lua_isnumber(L, -1)) {
		*dst = (uint32_t)(lua_Integer)lua_tointeger(L, -1);
	} else if (lua_isstring(L, -1)) {
		*dst = (uint32_t)strtoul(lua_tostring(L, -1), NULL, 0);
	}
	lua_pop(L, 1);
}

/* ---------------------------------------------------------------- public API */

void load_config(void)
{
	set_defaults();
	build_default_binds();

	const char *home = getenv("HOME");
	if (!home) {
		fprintf(stderr, "neuwm: WARNING: HOME not set, using built-in config\n");
		return;
	}

	char path[512];
	snprintf(path, sizeof(path), "%s/.config/neuwm/config.lua", home);

	lua_State *L = luaL_newstate();
	if (!L) {
		fprintf(stderr, "neuwm: WARNING: luaL_newstate failed, using built-in config\n");
		return;
	}
	luaL_openlibs(L);

	if (luaL_dofile(L, path) != 0) {
		fprintf(stderr, "neuwm: WARNING: %s\n", lua_tostring(L, -1));
		fprintf(stderr, "neuwm: using built-in config\n");
		lua_close(L);
		return;
	}

	/* --- colours & scalars --- */
	get_color(L, "border_col_active",        &cfg.border_col_active);
	get_color(L, "border_col_normal",        &cfg.border_col_normal);
	get_uint (L, "border_width",             &cfg.border_width);
	get_color(L, "outer_border_col_active",  &cfg.outer_border_col_active);
	get_color(L, "outer_border_col_normal",  &cfg.outer_border_col_normal);
	get_uint (L, "outer_border_width",       &cfg.outer_border_width);
	get_uint (L, "gaps",                     &cfg.gaps);
	get_uint (L, "master_width",             &cfg.master_width);
	get_int  (L, "float_x",                 &cfg.float_x);
	get_int  (L, "float_y",                 &cfg.float_y);
	get_uint (L, "float_w",                 &cfg.float_w);
	get_uint (L, "float_h",                 &cfg.float_h);
	get_float(L, "zoom_level",              &cfg.zoom_level);
	get_uint (L, "motion_throttle_hz",      &cfg.motion_throttle_hz);

	/* --- termcmd --- */
	lua_getglobal(L, "termcmd");
	if (lua_istable(L, -1)) {
		char **tc = parse_string_array(L, -1);
		if (tc) termcmd = tc;
	}
	lua_pop(L, 1);

	/* --- autostart --- */
	lua_getglobal(L, "autostart");
	if (lua_istable(L, -1))
		parse_autostart(L);
	lua_pop(L, 1);

	/* --- binds --- */
	lua_getglobal(L, "binds");
	if (lua_istable(L, -1))
		parse_binds(L);
	lua_pop(L, 1);

	lua_close(L);
}
