# neuwm
Based on [wsxwm](https://git.sr.ht/~uint/wsxwm) and borrowed from [hevel](https://git.sr.ht/~dlm/hevel).

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
- Mode indicator written to `/tmp/neuwm_mode` for status bar integration

## Known issues
- Zoom and workspaces are incompatible due to a swc compositor limitation;
  hidden windows from other workspaces become visible when zoomed out.
  Workspace switching resets zoom but the underlying issue remains.

  #### A scrolling infinite canvas layout based on [niri](https://github.com/YaLTeR/niri) is planned to replace workspaces entirely in future releases

## Dependencies
Install these first:
- [neuwld](https://git.sr.ht/~shrub900/neuwld) — patched wld rendering library
- [neuswc](https://git.sr.ht/~shrub900/neuswc) — patched swc compositor

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

## Build & Install
```sh
make
doas make install
```

## Launch
```sh
#!/bin/sh -e
export WLD_DRM_DUMB=1
swall -m zoom-fill /path/to/wallpaper.jpg &
~/.config/mojito/status.sh | mojito -f "monospace-10" &
swc-launch neuwm
```

## Configuration
Edit `source/config.h` and recompile.

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
- [swall](https://git.sr.ht/~uint/swall) — wallpaper setter for neuswc

## Credits
- [~shrub900](https://git.sr.ht/~shrub900/)
  - [neuswc](https://git.sr.ht/~shrub900/neuswc)
  - [neuwld](https://git.sr.ht/~shrub900/neuwld)
- [~uint](https://git.sr.ht/~uint)
  - [wsxwm](https://git.sr.ht/~uint/wsxwm)
  - [neumenu](https://git.sr.ht/~uint/neumenu)
  - [swall](https://git.sr.ht/~uint/swall)
- [~dlm](https://git.sr.ht/~dlm)
  - [hevel](https://git.sr.ht/~dlm/hevel)
  - [mojito](https://git.sr.ht/~dlm/mojito)
  - [hst](https://git.sr.ht/~dlm/hst)
- [Michael Forney](https://github.com/michaelforney) for all of their amazing work, but specifically [swc](https://github.com/michaelforney/swc) & [wld](https://github.com/michaelforney/wld) 

## License
neuwm is licensed with [unlicense](https://unlicense.org/), effectively putting it in the public domain. You are free to do whatever you please with this code.