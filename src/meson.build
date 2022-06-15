protocol = protoc.process(
  [
    'protocol/fs.proto',
    'protocol/clipboard.proto',
    'protocol/base_message.proto',
    'protocol/pair.proto',
    'protocol/remote_display.proto',
    'protocol/input_event.proto',
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
  utils/sha256.h
  utils/sha256.cc
  utils/idxubyte.h
  utils/message.h
  utils/net.h
'''.split())

dde_cooperation_sources = files('''
  dde-cooperation/main.cc
  dde-cooperation/Cooperation.h
  dde-cooperation/Cooperation.cc
  dde-cooperation/KeyPair.h
  dde-cooperation/KeyPair.cc
  dde-cooperation/Device.h
  dde-cooperation/Device.cc
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
    libnm,
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
'''.split())

dde_cooperation_agent_sources += common_sources

executable('dde-cooperation-agent',
  sources: dde_cooperation_agent_sources,
  dependencies: [
    fmt,
    glibmm,
    giomm,
  ],
  install: true,
)

