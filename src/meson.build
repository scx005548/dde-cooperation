protocol = protoc.process(
  [
    'protocol/fs.proto',
    'protocol/clipboard.proto',
    'protocol/base_message.proto',
    'protocol/pair.proto',
    'protocol/remote_display.proto',
    'protocol/input_event.proto',
    'protocol/cooperation.proto',
  ],
  preserve_path_from: meson.current_source_dir(),
)

common_sources = files('''
  dbus/dbus.h
  dbus/Method.h
  dbus/Method.cc
  dbus/Property.h
  dbus/Property.cc
  dbus/Interface.h
  dbus/Interface.cc
  dbus/Service.h
  dbus/Service.cc
  dbus/Object.h
  dbus/Object.cc
  utils/message.h
  utils/net.h
  utils/ptr.h
'''.split())

dde_cooperation_sources = files('''
  dde-cooperation/main.cc
  dde-cooperation/Cooperation.h
  dde-cooperation/Cooperation.cc
  dde-cooperation/KeyPair.h
  dde-cooperation/KeyPair.cc
  dde-cooperation/Machine.h
  dde-cooperation/Machine.cc
  dde-cooperation/InputDevice.h
  dde-cooperation/InputDevice.cc
  dde-cooperation/InputEvent.h
  dde-cooperation/InputEvent.cc
'''.split())

dde_cooperation_sources += common_sources + protocol

executable('dde-cooperation',
  dde_cooperation_sources,
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
)

dde_cooperation_agent_sources = files('''
  dde-cooperation-agent/main.cc
  dde-cooperation-agent/Agent.h
  dde-cooperation-agent/Agent.cc
  dde-cooperation-agent/EdgeDetector.h
  dde-cooperation-agent/EdgeDetector.cc
  dde-cooperation-agent/EdgeDetector/X11.h
  dde-cooperation-agent/EdgeDetector/X11.cc
'''.split())

dde_cooperation_agent_sources += common_sources

executable('dde-cooperation-agent',
  sources: dde_cooperation_agent_sources,
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
)

