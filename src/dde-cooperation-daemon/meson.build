dde_cooperation_daemon_sources = files('''
  main.cc
  Manager.h
  Manager.cc
  KeyPair.h
  KeyPair.cc
  Machine.h
  Machine.cc
  Request.h
  Request.cc
  InputEmittorWrapper.h
  InputEmittorWrapper.cc
  InputGrabberWrapper.h
  InputGrabberWrapper.cc
  FuseClient.h
  FuseClient.cc
  FuseServer.h
  FuseServer.cc
  DisplayBase.h
  DisplayBase.cc
  ClipboardBase.h
  ClipboardBase.cc
  X11/X11.h
  X11/X11.cc
  X11/Display.h
  X11/Display.cc
  X11/Clipboard.h
  X11/Clipboard.cc
'''.split())

dde_cooperation_daemon_sources += common_sources + uvxx_sources + protocol + ipc_protocol

conf_data = configuration_data()
conf_data.set('libexecdir', libexecdir)
configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf_data
)

executable('dde-cooperation-daemon',
  dde_cooperation_daemon_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    thread,
    uuid,
    fmt,
    spdlog,
    openssl,
    libevdev,
    uv,
    fuse3,
    xcb,
    xcb_randr,
    xcb_xinput,
    xcb_xfixes,
    glibmm,
    giomm,
    protobuf,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
