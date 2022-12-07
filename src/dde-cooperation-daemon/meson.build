dde_cooperation_daemon_sources = files('''
  main.cc
  Manager.cc
  ManagerDBusAdaptor.cc
  KeyPair.h
  KeyPair.cc
  Machine/Machine.cc
  Machine/MachineDBusAdaptor.cc
  Machine/PCMachine.h
  Machine/PCMachine.cc
  Machine/AndroidMachine.h
  Machine/AndroidMachine.cc
  Wrappers/InputEmittorWrapper.h
  Wrappers/InputEmittorWrapper.cc
  Wrappers/InputGrabberWrapper.h
  Wrappers/InputGrabberWrapper.cc
  Wrappers/ConfirmDialogWrapper.h
  Wrappers/ConfirmDialogWrapper.cc
  Fuse/FuseClient.cc
  Fuse/FuseServer.cc
  DisplayBase.h
  DisplayBase.cc
  ClipboardBase.h
  ClipboardBase.cc
  X11/X11.cc
  X11/Display.h
  X11/Display.cc
  X11/Clipboard.h
  X11/Clipboard.cc
'''.split())

dde_cooperation_daemon_moc_headers = files('''
  Manager.h
  ManagerDBusAdaptor.h
  Machine/Machine.h
  Machine/MachineDBusAdaptor.h
  Fuse/FuseClient.h
  Fuse/FuseServer.h
  X11/X11.h
'''.split())

dde_cooperation_daemon_sources += qt5.preprocess(
  qresources: [],
  moc_headers: dde_cooperation_daemon_moc_headers,
)

dde_cooperation_daemon_sources += common_sources + [protocol] + [ipc_protocol]

conf_data = configuration_data()
conf_data.set('executable_install_dir', executable_install_dir)
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
    tl_expected,
    fuse3,
    xcb,
    xcb_randr,
    xcb_xinput,
    xcb_xfixes,
    protobuf,
    qt5dep,
    dtk_core,
  ],
  link_with: [
    uvxx,
  ],
  install: true,
  install_dir: executable_install_dir
)
