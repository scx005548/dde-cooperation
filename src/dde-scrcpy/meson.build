dde_scrcpy_sources = files('''
  main.cc
  MainWindow.cc
'''.split())

moc_headers = files('''
  MainWindow.h
'''.split())

dde_scrcpy_sources += qt5.preprocess(
  qresources: [],
  moc_headers: moc_headers,
)

executable('dde-scrcpy',
  sources: dde_scrcpy_sources,
  dependencies: [
    qt5dep,
  ],
  install: true,
)
