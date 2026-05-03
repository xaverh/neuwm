project('neuwm', 'c',
  version : '0.1',
  default_options : [
    'c_std=c99',
    'warning_level=2',
    'optimization=2',
  ],
)

os = host_machine.system()

luajit_dep = dependency('luajit')
neuipc_dep = dependency('neuipc')

libinput_dep = os == 'linux' ? dependency('libinput') : dependency('', required: false)
libudev_dep = os == 'linux' ? dependency('libudev') : dependency('', required: false)

pkg_deps = [
  dependency('swc'),
  dependency('wayland-server'),
  dependency('xkbcommon'),
  libinput_dep,
  dependency('pixman-1'),
  dependency('libdrm'),
  dependency('wld'),
  libudev_dep,
  dependency('xcb'),
  dependency('xcb-composite'),
  dependency('xcb-ewmh', required: false),
  dependency('xcb-icccm', required: false),
  ]

cc = meson.get_compiler('c')
libm = cc.find_library('m', required : true)

executable('neuwm',
  sources : [
    'source/neuwm.c',
    'source/util.c',
    'source/lua-config.c',
    'source/neuwm-ipc.c',
  ],
  include_directories : include_directories('include'),
  c_args : ['-D_POSIX_C_SOURCE=200809L', '-Wall', '-Wextra'],
  dependencies : pkg_deps + [libm, luajit_dep, neuipc_dep],
  install : true,
  install_dir : '/usr/local/bin',
)

executable('neuwmctl',
  sources : ['source/neuwmctl.c'],
  c_args : ['-D_POSIX_C_SOURCE=200809L'],
  install : true,
  install_dir : '/usr/local/bin',
)
