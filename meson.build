project('neuwm', 'c',
  version : '0.1',
  default_options : [
    'c_std=c99',
    'warning_level=2',
    'optimization=2',
  ],
)

pkg_deps = [
  dependency('swc'),
  dependency('wayland-server'),
  dependency('xkbcommon'),
  dependency('libinput'),
  dependency('pixman-1'),
  dependency('libdrm'),
  dependency('wld'),
  dependency('libudev'),
  dependency('xcb'),
  dependency('xcb-composite'),
  dependency('xcb-ewmh'),
  dependency('xcb-icccm'),
]

cc = meson.get_compiler('c')
libm = cc.find_library('m', required : true)

executable('neuwm',
  sources : [
    'source/neuwm.c',
    'source/util.c',
  ],
  include_directories : include_directories('include'),
  c_args : ['-D_POSIX_C_SOURCE=200809L', '-Wall', '-Wextra'],
  dependencies : pkg_deps + [libm],
  install : true,
  install_dir : '/usr/local/bin',
)
