# neuwm
Based on [wsxwm](https://git.sr.ht/~uint/wsxwm) and borrowed from [hevel](https://git.sr.ht/~dlm/hevel).

## Why

I wanted a compositor that combined the best parts of the projects I love — double borders, nein cursor, and smooth zoom/pan from [hevel](https://git.sr.ht/~dlm/hevel), and the keyboard-driven tiling/floating layout from [tohu](https://git.sr.ht/~shrub900/tohu) (itself a fork of [wsxwm](https://git.sr.ht/~unit/wsxwm)). Since `tohu` and `wsxwm` share the same base, `wsxwm` made the most sense as a starting point. `neuwm` is the result of pulling those features together into a single compositor.

## Inherited from wsxwm
- Tiling and floating window management
- Vanity gaps in tiling mode
- 9 virtual workspaces (see known issues)
- Per-window and global float/tile toggle
- Mouse move and resize for floating windows

## Borrowed from hevel
- Plan 9 nein cursor theme (from [hevel](https://git.sr.ht/~dlm/hevel))
- Double window borders (from [hevel](https://git.sr.ht/~dlm/hevel))
- Compositor zoom in/out via scroll wheel with smooth easing (from [hevel](https://git.sr.ht/~dlm/hevel))
- Draw-to-spawn terminals — press `mod+d`, drag to define window size (inspired by [hevel](https://git.sr.ht/~dlm/hevel))

## Added in neuwm
- Resize height of stacked windows `mod+j/k`
- Cascading floating window placement with screen boundary awareness
- Global float toggle now remembers window positions and geometry
- Keyboard move and resize for floating windows
- Click to focus and reset zoom; hover focus suppressed while zoomed
- Pan canvas by dragging empty desktop with `mod+left-click`
- IPC support via [neuipc](https://codeberg.org/binkd/neuipc) — Unix socket at `/tmp/neuwm.sock`, `neuwmctl` CLI for querying and subscribing

## Known issues
- Zoom and workspaces are incompatible due to a swc compositor limitation;
  hidden windows from other workspaces become visible when zoomed out.
  Workspace switching resets zoom but the underlying issue remains.

  #### An infinite canvas layout is planned to replace workspaces entirely in future releases

## Dependencies
Install these first:
- [neuwld](https://git.sr.ht/~shrub900/neuwld) — patched wld rendering library
- [neuswc](https://git.sr.ht/~shrub900/neuswc) — patched swc compositor
- [neuipc](https://codeberg.org/binkd/neuipc) — lightweight Unix IPC library

Then the standard build dependencies:
- wayland-server
- libinput
- xkbcommon
- pixman
- libdrm
- libudev
- xcb
- xcb-composite
- xcb-ewmh
- xcb-icccm
- luajit (for Lua config support)

## Build & Install

### meson
```sh
meson setup build
ninja -C build
doas ninja -C build install
```

## Launch
```sh
#!/bin/sh -e
export WLD_DRM_DUMB=1
export XDG_SESSION_DESKTOP=neuwm
swc-launch neuwm
```

## Configuration
Edit `~/.config/neuwm/config.lua`. Falls back to built-in defaults if missing.

## Decoration themes
Window decorations are provided by neuswc. To use them, clone the [decor](https://codeberg.org/shrub900/decor) collection and follow the per-theme `info` file for correct widths. Set the theme path in `~/.config/neuwm/config.lua`.

## IPC
neuwm exposes a Unix socket at `/tmp/neuwm.sock`. Use `neuwmctl` to query or subscribe:

```sh
neuwmctl get_workspace    # current workspace and max: "1 9"
neuwmctl get_workspaces   # current + workspaces with windows: "1 4 5"
neuwmctl get_mode         # tile or float
neuwmctl sub ws           # subscribe to workspace change events
neuwmctl sub mode         # subscribe to mode change events
```

## Keybinds
| Keybind | Action |
|---|---|
| `mod+return` | Terminal |
| `mod+space` | Menu |
| `mod+shift+e` | Quit |
| `mod+tab` | Focus next |
| `mod+shift+tab` | Focus prev |
| `mod+shift+j/k` | Rotate master |
| `mod+h/l` | Resize master |
| `mod+j/k` | Resize stack |
| `mod+t` | Toggle float (current window) |
| `mod+shift+t` | Toggle global float |
| `mod+f` | Fullscreen |
| `mod+shift+q` | Close window |
| `mod+1-9` | Switch workspace |
| `mod+shift+1-9` | Move window to workspace |
| `mod+arrows` | Move floating window |
| `mod+shift+arrows` | Resize floating window (left/top anchor) |
| `mod+ctrl+arrows` | Resize floating window (right/bottom anchor) |
| `mod+equal` | Scale floating window up |
| `mod+minus` | Scale floating window down |
| `mod+left-click` | Move window / pan canvas on empty space |
| `mod+right-click` | Resize window |
| `mod+scroll` | Zoom in/out |
| `mod+d` | Draw-to-spawn terminal mode |
| `mod+click` | Focus window / reset zoom |

## Extras
### swcsnap
Screenshots using neuswc's `swc_snap` protocol. Outputs PPM to stdout.
```sh
swcsnap image screenshot.ppm
```

Interactive region selection via `chum`
```sh
swcsnap region screenshot.ppm
```

### swcrecord
Screen recorder wrapping swcsnap with ffmpeg. Requires ffmpeg.
```sh
swcsnap record output.mp4
```

## Recommended
To make neuwm usable, the following programs are highly recomended
- [hst](https://git.sr.ht/~dlm/hst) — a pre-patched and [freeway](https://git.sr.ht/~shrub900/freeway)-compatible version of [st-wl](https://github.com/ianbeyst/st-wl) with better font rendering using neuwld
- [neumenu](https://git.sr.ht/~uint/neumenu) — updated [dmenu](https://github.com/michaelforney/dmenu) fork for wayland using [neuwld](https://git.sr.ht/~shrub900/neuwld)
- [mojito](https://git.sr.ht/~dlm/mojito) — featherweight, lemonbar-compatible bar for neuswc-based compositors
- [neubar](https://git.sr.ht/~pfr/neubar) — event-driven status bar feeder for mojito; uses neuwm IPC for workspace/mode updates and inotify for package count, replacing polling shell scripts
- [wawa](https://codeberg.org/sewn/wawa) — minimal layer shell wallpaper setter
- [chum](https://git.sr.ht/~dlm/chum) — interactive region picker, required for `swcsnap region`
- [decor](https://codeberg.org/shrub900/decor) — window decoration theme collection for neuswc; includes a `png2swc` conversion script and per-theme width info

## Screenshot
![neuwm screenshot](assets/screenshot.png)

## Credits
- [~shrub900](https://git.sr.ht/~shrub900/)
  - [neuswc](https://git.sr.ht/~shrub900/neuswc)
  - [neuwld](https://git.sr.ht/~shrub900/neuwld)
- [~uint](https://git.sr.ht/~uint)
  - [wsxwm](https://git.sr.ht/~uint/wsxwm)
  - [neumenu](https://git.sr.ht/~uint/neumenu)
- [~dlm](https://git.sr.ht/~dlm)
  - [hevel](https://git.sr.ht/~dlm/hevel)
  - [mojito](https://git.sr.ht/~dlm/mojito)
  - [hst](https://git.sr.ht/~dlm/hst)
  - [chum](https://git.sr.ht/~dlm/chum)
- [sewn](https://codeberg.org/sewn)
  - [wawa](https://codeberg.org/sewn/wawa)
- [Michael Forney](https://github.com/michaelforney) for all of their amazing work, but specifically [swc](https://github.com/michaelforney/swc) & [wld](https://github.com/michaelforney/wld)

## License
neuwm is licensed with [unlicense](https://unlicense.org/), effectively putting it in the public domain. You are free to do whatever you please with this code.
