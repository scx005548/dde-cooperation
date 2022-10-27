uvxx_sources = files('''
  UvType.h
  Loop.h
  Loop.cc
  noncopyable.h
  Addr.h
  Addr.cc
  Handle.h
  Handle.cc
  Stream.h
  Stream.cc
  Pipe.h
  Pipe.cc
  TCP.h
  TCP.cc
  UDP.h
  UDP.cc
  Buffer.h
  Buffer.cc
  Poll.h
  Poll.cc
  Timer.h
  Timer.cc
  Async.h
  Async.cc
  Process.h
  Process.cc
  Request.h
  Request.cc
  Signal.h
  Signal.cc
  utils.h
'''.split())

uvxx = static_library('uvxx',
  uvxx_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    spdlog,
    uv,
  ],
)
