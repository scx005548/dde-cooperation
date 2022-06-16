dde_cooperation_daemon_sources = files('''
  main.cc
  Cooperation.h
  Cooperation.cc
  KeyPair.h
  KeyPair.cc
  Machine.h
  Machine.cc
  InputDevice.h
  InputDevice.cc
  InputEvent.h
  InputEvent.cc
'''.split())

dde_cooperation_daemon_sources += common_sources + protocol

executable('dde-cooperation-daemon',
  dde_cooperation_daemon_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    uuid,
    fmt,
    spdlog,
    openssl,
    libevdev,
    glibmm,
    giomm,
    protobuf,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
