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

includes = include_directories('./')

subdir('dde-cooperation-daemon')
subdir('dde-cooperation-user')
