protocol = protoc.process(
  [
    'protocol/message.proto',
    'protocol/pair.proto',
    'protocol/device_sharing.proto',
    'protocol/fs.proto',
    'protocol/clipboard.proto',
    'protocol/remote_display.proto',
  ],
  preserve_path_from: meson.current_source_dir(),
)

ipc_protocol = protoc.process(
  [
    'protocol/ipc_message.proto',
  ],
  preserve_path_from: meson.current_source_dir(),
)

common_sources = files('''
  dbus/dbus.h
  dbus/Method.h
  dbus/Method.cc
  dbus/Property.h
  dbus/Property.cc
  dbus/Signal.h
  dbus/Signal.cc
  dbus/Interface.h
  dbus/Interface.cc
  dbus/Service.h
  dbus/Service.cc
  dbus/Object.h
  dbus/Object.cc
  utils/message_helper.h
  utils/net.h
  utils/ptr.h
'''.split())

includes = include_directories('./')

subdir('uvxx')
subdir('input-emittor')
subdir('input-grabber')
subdir('dde-cooperation-daemon')
subdir('dde-cooperation-dialog')
subdir('dde-scrcpy')
subdir('scrcpy-core')
