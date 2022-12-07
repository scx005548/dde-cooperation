input_grabber_sources = files('''
  main.cc
  InputGrabber.cc
'''.split())

input_grabber_moc_headers = files('''
  InputGrabber.h
'''.split())

input_grabber_sources += qt5.preprocess(
  qresources: [],
  moc_headers: input_grabber_moc_headers,
)

input_grabber_sources += [ipc_protocol]

input_grabber = executable('input-grabber',
  input_grabber_sources,
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
