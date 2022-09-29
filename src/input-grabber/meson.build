input_grabber_sources = files('''
  main.cc
  InputGrabber.h
  InputGrabber.cc
'''.split())

input_grabber_sources += uvxx_sources + [ipc_protocol]

input_grabber = executable('input-grabber',
  input_grabber_sources,
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
