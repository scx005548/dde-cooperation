dde_scrcpy_sources = files('''
  main.cc
  ui/MainWindow.cc
  ui/VideoForm.cc
  ui/ToolForm.cc
  render/QYUVOpenGLWidget.cc
  uibase/MagneticWidget.cc
  uibase/KeepRatioWidget.cc
  util/mousetap/MouseTap.cc
  util/mousetap/XMouseTap.cc
'''.split())

dde_scrcpy_moc_headers = files('''
  ui/MainWindow.h
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
    includes,
    '.',
  ],
  dependencies: [
    qt5dep,
    dtk_widget,
    xcb,
    scrcpy_core_dep,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)

dde_scrcpy_qrcode_sources = files('''
  qrcode.cc
  ui/NewDeviceDialog.cc
  uibase/QrCodeWidget.cc
'''.split())

dde_scrcpy_qrcode_moc_headers = files('''
  ui/NewDeviceDialog.h
  uibase/QrCodeWidget.h
'''.split())

dde_scrcpy_qrcode_sources += qt5.preprocess(
  qresources: [],
  moc_headers: dde_scrcpy_qrcode_moc_headers,
)

executable('dde-scrcpy-qrcode',
  sources: dde_scrcpy_qrcode_sources,
  include_directories: [
    includes,
    '.',
  ],
  dependencies: [
    qt5dep,
    dtk_widget,
    qrcodegencpp,
  ],
  install: true,
  install_dir: join_paths(libexecdir, 'dde-cooperation'),
)
