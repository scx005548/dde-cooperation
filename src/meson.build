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

uvxx_sources = files('''
  uvxx/UvType.h
  uvxx/Loop.h
  uvxx/Loop.cc
  uvxx/noncopyable.h
  uvxx/Addr.h
  uvxx/Addr.cc
  uvxx/Handle.h
  uvxx/Handle.cc
  uvxx/Stream.h
  uvxx/Stream.cc
  uvxx/Pipe.h
  uvxx/Pipe.cc
  uvxx/TCP.h
  uvxx/TCP.cc
  uvxx/UDP.h
  uvxx/UDP.cc
  uvxx/Buffer.h
  uvxx/Buffer.cc
  uvxx/Poll.h
  uvxx/Poll.cc
  uvxx/Timer.h
  uvxx/Timer.cc
  uvxx/Async.h
  uvxx/Async.cc
  uvxx/Process.h
  uvxx/Process.cc
  uvxx/Request.h
  uvxx/Request.cc
  uvxx/Signal.h
  uvxx/Signal.cc
  uvxx/utils.h
'''.split())

includes = include_directories('./')

subdir('input-emittor')
subdir('input-grabber')
subdir('dde-cooperation-daemon')
subdir('dde-cooperation')
subdir('dde-cooperation-dialog')
