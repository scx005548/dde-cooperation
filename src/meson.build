protocol = protoc.process(
  [
    'protocol/message.proto',
    'protocol/pair.proto',
    'protocol/device_sharing.proto',
    'protocol/fs.proto',
    'protocol/clipboard.proto',
    'protocol/remote_display.proto',
    'protocol/service_status.proto',
    'protocol/screencast.proto',
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
  utils/message_helper.h
  utils/net.h
  utils/ptr.h
'''.split())

includes = include_directories('./')

subdir('input-emitter')
subdir('input-grabber')
subdir('scrcpy-core')
subdir('dde-cooperation-daemon')
