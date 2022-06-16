dde_cooperation_sources = files('''
  main.cc
  Backend.cc
  MachineModel.cc
'''.split())

moc_headers = files('''
  Backend.h
  MachineModel.h
'''.split())

dde_cooperation_sources += qt5.preprocess(
  qresources: [],
  moc_headers: moc_headers,
)

executable('dde-cooperation',
  sources: dde_cooperation_sources,
  dependencies: [
    qt5dep,
  ],
  install: true,
)
