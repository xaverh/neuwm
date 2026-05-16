-- neuwm configuration
-- Loaded at startup; falls back to built-in defaults if missing or invalid.

-- Colors (ARGB, e.g. 0xffRRGGBB)
border_col_active        = 0xffc678dd
border_col_normal        = 0xff444444
border_width             = 1
outer_border_col_active  = 0xff222222
outer_border_col_normal  = 0xff222222
outer_border_width       = 4

-- Layout
master_width = 60   -- percentage of screen width for master column
gaps         = 10   -- outer and inner gap size in pixels

-- Default floating window geometry
float_x = 100
float_y = 100
float_w = 802
float_h = 500

-- Initial zoom level (1.0 = no zoom)
zoom_level = 0.5

-- Pointer motion throttle (events per second)
motion_throttle_hz = 85

-- Terminal command; also used by draw-to-spawn mode (mod+d)
termcmd = { "st-wl" }

-- Commands executed via sh -c on startup (double-forked, non-blocking)
autostart = {
    'pgrep -x pipewire || pipewire',
    'sleep 0.5; pgrep -x wireplumber || wireplumber',
    'sleep 0.5; pgrep -x pipewire-pulse || pipewire-pulse',
    'brightnessctl set 40%',
    'sh $HOME/.config/mojito/status.sh | mojito -g "1900x20+10" &',
    'swall -m scaled "$HOME/Pictures/walls/wallhaven-mpl1x8.jpg" &',
}

-- Key / button / axis bindings
-- Format:
--   key    binding: { "key",    "mods", "KeyName",  "action" [, arg] }
--   button binding: { "button", "mods", "BTN_NAME", "action" }
--   axis   binding: { "axis",   "mods", "action" }
--
-- Mod strings: combine with "+": "mod4", "mod4+shift", "mod4+ctrl", "any"
-- spawn action: arg is a table of argv strings { "cmd", "arg", ... }
-- integer actions: arg is a positive or negative number

binds = {

    -- spawners
    { "key",    "",            "XF86MonBrightnessDown", "spawn", { "brightnessctl", "set", "5%-" } },
    { "key",    "",            "XF86MonBrightnessUp",   "spawn", { "brightnessctl", "set", "5%+" } },
    { "key",    "",            "XF86AudioLowerVolume",  "spawn", { "wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-" } },
    { "key",    "",            "XF86AudioRaiseVolume",  "spawn", { "wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+" } },
    { "key",    "",            "XF86AudioMute",         "spawn", { "wpctl", "set-mute",   "@DEFAULT_AUDIO_SINK@", "toggle" } },
    { "key",    "mod4",        "Return", "spawn", { "st-wl" } },
    { "key",    "mod4",        "space",  "spawn", { "neumenu_run",
                                                     "-fn", "tadepe",
                                                     "-nb", "#000000",
                                                     "-sb", "#c678dd",
                                                     "-sf", "#000000" } },
    { "key",    "mod4",        "b",      "spawn", { "pkill", "-USR1", "mojito" } },

    -- window management
    { "key",    "mod4+shift",  "e",      "quit"                },
    { "key",    "mod4",        "Tab",    "focus_next"          },
    { "key",    "mod4+shift",  "Tab",    "focus_prev"          },
    { "key",    "mod4",        "q",      "kill_sel"            },
    { "key",    "mod4",        "t",      "toggle_float"        },
    { "key",    "mod4+shift",  "t",      "toggle_float_global" },
    { "key",    "mod4",        "f",      "fullscreen"          },
    { "key",    "mod4",        "d",      "draw_mode_toggle"    },

    -- tiling resize
    { "key",    "mod4",        "l",      "master_resize",   10  },
    { "key",    "mod4",        "h",      "master_resize",  -10  },
    { "key",    "mod4",        "k",      "stack_resize",    10  },
    { "key",    "mod4",        "j",      "stack_resize",   -10  },
    { "key",    "mod4+shift",  "k",      "master_next"          },
    { "key",    "mod4+shift",  "j",      "master_prev"          },

    -- floating move
    { "key",    "mod4",        "Left",   "float_move_x",  -10  },
    { "key",    "mod4",        "Right",  "float_move_x",   10  },
    { "key",    "mod4",        "Up",     "float_move_y",  -10  },
    { "key",    "mod4",        "Down",   "float_move_y",   10  },

    -- floating resize — left/top anchored (expands leftward/upward)
    { "key",    "mod4+shift",  "Left",   "float_resize_w",   6  },
    { "key",    "mod4+shift",  "Right",  "float_resize_w",  -6  },
    { "key",    "mod4+shift",  "Up",     "float_resize_h",   6  },
    { "key",    "mod4+shift",  "Down",   "float_resize_h",  -6  },

    -- floating resize — right/bottom anchored (expands rightward/downward)
    { "key",    "mod4+ctrl",   "Left",   "float_resize_w_right",    -6  },
    { "key",    "mod4+ctrl",   "Right",  "float_resize_w_right",     6  },
    { "key",    "mod4+ctrl",   "Up",     "float_resize_h_bottom",   -6  },
    { "key",    "mod4+ctrl",   "Down",   "float_resize_h_bottom",    6  },

    -- floating scale (centered)
    { "key",    "mod4",        "equal",  "float_scale",   6  },
    { "key",    "mod4",        "minus",  "float_scale",  -6  },

    -- mouse
    { "button", "mod4",        "BTN_LEFT",  "mouse_move"   },
    { "button", "mod4",        "BTN_RIGHT", "mouse_resize" },
    { "button", "any",         "BTN_LEFT",  "mouse_click"  },

    -- zoom (scroll wheel)
    { "axis",   "mod4",        "zoom_axis" },

    -- workspaces — switch
    { "key",    "mod4",        "1",      "workspace_goto",    1 },
    { "key",    "mod4",        "2",      "workspace_goto",    2 },
    { "key",    "mod4",        "3",      "workspace_goto",    3 },
    { "key",    "mod4",        "4",      "workspace_goto",    4 },
    { "key",    "mod4",        "5",      "workspace_goto",    5 },
    { "key",    "mod4",        "6",      "workspace_goto",    6 },
    { "key",    "mod4",        "7",      "workspace_goto",    7 },
    { "key",    "mod4",        "8",      "workspace_goto",    8 },
    { "key",    "mod4",        "9",      "workspace_goto",    9 },

    -- workspaces — move window
    { "key",    "mod4+shift",  "1",      "workspace_moveto",  1 },
    { "key",    "mod4+shift",  "2",      "workspace_moveto",  2 },
    { "key",    "mod4+shift",  "3",      "workspace_moveto",  3 },
    { "key",    "mod4+shift",  "4",      "workspace_moveto",  4 },
    { "key",    "mod4+shift",  "5",      "workspace_moveto",  5 },
    { "key",    "mod4+shift",  "6",      "workspace_moveto",  6 },
    { "key",    "mod4+shift",  "7",      "workspace_moveto",  7 },
    { "key",    "mod4+shift",  "8",      "workspace_moveto",  8 },
    { "key",    "mod4+shift",  "9",      "workspace_moveto",  9 },
}

-- Window decorations (disabled by default)
-- Remove comment markers below to enable.
--
decor_on = true
--
-- Theme selection:
--   "motif"        	Motif theme (compiled-in PNG theme)
--   "motif-notitle"    Motif theme without titlebar (compiled-in PNG theme)
--   unset        	solid color only
--
decor_theme = "motif"
--
decor = {
    color  = 0xff222222,
    top    = 23, -- motif = 23 motif-notitle = 6 
    right  = 6,
    bottom = 6,
    left   = 6,
    title = {
        enabled = true,
        edge    = "top",
        align   = "center",
        padding = 4,
        font    = "Tadepe:style=Bold:pixelsize=16:antialias=false",
    },
}
