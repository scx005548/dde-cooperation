dde_scrcpy_sources = files('''
  main.cc
  MainWindow.cc
  ui/VideoForm.cc
  ui/ToolForm.cc
  render/QYUVOpenGLWidget.cc
  uibase/MagneticWidget.cc
  uibase/KeepRatioWidget.cc
  util/mousetap/MouseTap.cc
  util/mousetap/XMouseTap.cc
'''.split())

dde_scrcpy_moc_headers = files('''
  MainWindow.h
  ui/VideoForm.h
  ui/ToolForm.h
  render/QYUVOpenGLWidget.h
  uibase/MagneticWidget.h
  uibase/KeepRatioWidget.h
'''.split())

dde_scrcpy_ui_files = files('''
  ui/VideoForm.ui
  ui/ToolForm.ui
'''.split())

dde_scrcpy_sources += qt5.preprocess(
  qresources: [],
  moc_headers: dde_scrcpy_moc_headers,
  ui_files: dde_scrcpy_ui_files,
)

executable('dde-scrcpy',
  sources: dde_scrcpy_sources,
  include_directories: [
    '.',
  ],
  dependencies: [
    qt5dep,
    xcb,
    scrcpy_core_dep,
  ],
  install: true,
)
