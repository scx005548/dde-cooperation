dde_cooperation_user_sources = files('''
  main.cc
  Manager.h
  Manager.cc
  EdgeDetector.h
  EdgeDetector.cc
  EdgeDetector/X11.h
  EdgeDetector/X11.cc
'''.split())

dde_cooperation_user_sources += common_sources

executable('dde-cooperation-user',
  sources: dde_cooperation_user_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    fmt,
    glibmm,
    giomm,
    x11_xcb,
    xi,
    xcb,
    xcb_aux,
    xcb_randr,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
