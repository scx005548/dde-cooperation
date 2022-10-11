scrcpy_public_headers = files('''
  include/QtScrcpyCoreDef.h
'''.split())

scrcpy_public_headers += qt5.preprocess(
  qresources: [],
  moc_headers: files('''
  include/AdbProcess.h
  include/QtScrcpyCore.h
'''.split()),
)

scrcpy_core_sources = files('''
  src/device/Device.cc
  src/device/compat.h
  src/device/android/input.h
  src/device/android/keycodes.h
  src/device/controller/Controller.cc
  src/device/controller/BufferUtil.h
  src/device/controller/BufferUtil.cc
  src/device/controller/inputconvert/InputConvertBase.cc
  src/device/controller/inputconvert/InputConvertNormal.cc
  src/device/controller/inputconvert/InputConvertGame.cc
  src/device/controller/inputconvert/ControlMsg.h
  src/device/controller/inputconvert/ControlMsg.cc
  src/device/controller/inputconvert/keymap/Keymap.cc
  src/device/controller/receiver/DeviceMsg.cc
  src/device/controller/receiver/Receiver.cc
  src/device/decoder/AVFrameConvert.h
  src/device/decoder/AVFrameConvert.cc
  src/device/decoder/Decoder.cc
  src/device/decoder/FpsCounter.cc
  src/device/decoder/VideoBuffer.cc
  src/device/filehandler/FileHandler.cc
  src/device/recorder/Recorder.cc
  src/device/server/Server.cc
  src/device/server/TcpServer.cc
  src/device/server/VideoSocket.cc
  src/device/stream/Stream.cc
'''.split())

scrcpy_core_headers = files('''
  src/device/Device.h
  src/device/controller/Controller.h
  src/device/controller/inputconvert/InputConvertBase.h
  src/device/controller/inputconvert/InputConvertNormal.h
  src/device/controller/inputconvert/InputConvertGame.h
  src/device/controller/inputconvert/keymap/Keymap.h
  src/device/controller/receiver/DeviceMsg.h
  src/device/controller/receiver/Receiver.h
  src/device/decoder/Decoder.h
  src/device/decoder/FpsCounter.h
  src/device/decoder/VideoBuffer.h
  src/device/filehandler/FileHandler.h
  src/device/recorder/Recorder.h
  src/device/server/Server.h
  src/device/server/TcpServer.h
  src/device/server/VideoSocket.h
  src/device/stream/Stream.h
'''.split())

scrcpy_core_sources += qt5.preprocess(
  qresources: [],
  moc_headers: scrcpy_core_headers,
)

scrcpy_adb_sources = files('''
  src/adb/AdbProcessImpl.cc
  src/adb/AdbProcess.cc
'''.split())

scrcpy_adb_headers = files('''
  src/adb/AdbProcessImpl.h
'''.split())

scrcpy_adb_sources += qt5.preprocess(
  qresources: [],
  moc_headers: scrcpy_adb_headers,
)

scrcpy_common_sources = files('''
  src/common/QScrcpyEvent.h
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
