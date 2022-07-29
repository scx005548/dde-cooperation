dde_cooperation_user_sources = files('''
  main.cc
  Manager.h
  Manager.cc
  FuseClient.h
  FuseClient.cc
  FuseServer.h
  FuseServer.cc
  DisplayServer.h
  DisplayServer.cc
  DisplayServer/X11.h
  DisplayServer/X11.cc
'''.split())

dde_cooperation_user_sources += common_sources + uvxx_sources + protocol

executable('dde-cooperation-user',
  sources: dde_cooperation_user_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    thread,
    fmt,
    uv,
    glibmm,
    giomm,
    protobuf,
    fuse3,
    xcb,
    xcb_randr,
    xcb_xinput,
    xcb_xfixes,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
