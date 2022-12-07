input_emittor_sources = files('''
  main.cc
  InputEmittor.h
  InputEmittor.cc
'''.split())

input_emittor_moc_headers = files('''
'''.split())

input_emittor_sources += qt5.preprocess(
  qresources: [],
  moc_headers: input_emittor_moc_headers,
)

input_emittor_sources += [ipc_protocol]

input_emittor = executable('input-emittor',
  input_emittor_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    thread,
    fmt,
    libevdev,
    protobuf,
    qt5dep,
  ],
  install: true,
  install_mode: ['rwsr-xr-x'],
  install_dir: executable_install_dir
)
