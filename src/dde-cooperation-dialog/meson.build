dde_cooperation_dialog_sources = files('''
  confirmdialog.h
  confirmdialog.cpp
  main.cpp
'''.split())

qtprocessed = qt5.preprocess(moc_headers: 'confirmdialog.h')

dde_cooperation_dialog = executable('dde-cooperation-dialog',
  qtprocessed,
  dde_cooperation_dialog_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    qt5dep,
    dtk_widget
  ],
  install: true,
  install_mode: ['rwsr-xr-x'],
  install_dir: executable_install_dir
)
