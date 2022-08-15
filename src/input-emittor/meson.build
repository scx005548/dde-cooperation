input_emittor_sources = files('''
  main.cc
  InputEmittor.h
  InputEmittor.cc
'''.split())

input_emittor_sources += uvxx_sources + ipc_protocol

input_emittor = executable('input-emittor',
  input_emittor_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    thread,
    fmt,
    spdlog,
    libevdev,
    uv,
    protobuf,
  ],
  install: true,
  install_mode: ['rwsr-xr-x'],
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
