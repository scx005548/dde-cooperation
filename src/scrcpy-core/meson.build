scrcpy_public_headers = files('''
  include/QtScrcpyCoreDef.h
'''.split())

scrcpy_public_headers += qt5.preprocess(
  qresources: [],
  moc_headers: files('''
  include/adbprocess.h
  include/QtScrcpyCore.h
'''.split()),
)

scrcpy_core_sources = files('''
  src/device/device.cpp
  src/device/compat.h
  src/device/android/input.h
  src/device/android/keycodes.h
  src/device/controller/controller.cpp
  src/device/controller/bufferutil.h
  src/device/controller/bufferutil.cpp
  src/device/controller/inputconvert/inputconvertbase.cpp
  src/device/controller/inputconvert/inputconvertnormal.cpp
  src/device/controller/inputconvert/inputconvertgame.cpp
  src/device/controller/inputconvert/controlmsg.h
  src/device/controller/inputconvert/controlmsg.cpp
  src/device/controller/inputconvert/keymap/keymap.cpp
  src/device/controller/receiver/devicemsg.cpp
  src/device/controller/receiver/receiver.cpp
  src/device/decoder/avframeconvert.h
  src/device/decoder/avframeconvert.cpp
  src/device/decoder/decoder.cpp
  src/device/decoder/fpscounter.cpp
  src/device/decoder/videobuffer.cpp
  src/device/filehandler/filehandler.cpp
  src/device/recorder/recorder.cpp
  src/device/server/server.cpp
  src/device/server/tcpserver.cpp
  src/device/server/videosocket.cpp
  src/device/stream/stream.cpp
'''.split())

scrcpy_core_headers = files('''
  src/device/device.h
  src/device/controller/controller.h
  src/device/controller/inputconvert/inputconvertbase.h
  src/device/controller/inputconvert/inputconvertnormal.h
  src/device/controller/inputconvert/inputconvertgame.h
  src/device/controller/inputconvert/keymap/keymap.h
  src/device/controller/receiver/devicemsg.h
  src/device/controller/receiver/receiver.h
  src/device/decoder/decoder.h
  src/device/decoder/fpscounter.h
  src/device/decoder/videobuffer.h
  src/device/filehandler/filehandler.h
  src/device/recorder/recorder.h
  src/device/server/server.h
  src/device/server/tcpserver.h
  src/device/server/videosocket.h
  src/device/stream/stream.h
'''.split())

scrcpy_core_sources += qt5.preprocess(
  qresources: [],
  moc_headers: scrcpy_core_headers,
)

scrcpy_adb_sources = files('''
  src/adb/adbprocessimpl.cpp
  src/adb/adbprocess.cpp
'''.split())

scrcpy_adb_headers = files('''
  src/adb/adbprocessimpl.h
'''.split())

scrcpy_adb_sources += qt5.preprocess(
  qresources: [],
  moc_headers: scrcpy_adb_headers,
)

scrcpy_common_sources = files('''
  src/common/qscrcpyevent.h
'''.split())

static_library('scrcpy_core',
  sources: [
    scrcpy_public_headers,
    scrcpy_core_sources,
    scrcpy_adb_sources,
    scrcpy_common_sources,
  ],
  dependencies: [
    qt5dep,
    libavcodec,
    libavformat,
    libavutil,
    libswscale,
  ],
  include_directories: [
    'include',
    'src/common',
    'src/device',
    'src/device/android',
    'src/device/controller',
    'src/device/controller/inputconvert',
    'src/device/controller/inputconvert/keymap',
    'src/device/controller/receiver',
    'src/device/decoder',
    'src/device/stream',
    'src/device/server',
    'src/device/filehandler',
    'src/device/recorder',
  ]
)
