project(
    'dde-cooperation',
    'cpp',
    version: '0.0.1',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
    ]
)

fmt = dependency('fmt', required: true)
spdlog = dependency('spdlog', required: true)
libnm = dependency('libnm', required: true)
glibmm = dependency('glibmm-2.4', required: true)
giomm = dependency('giomm-2.4', required: true)

protobuf = dependency('protobuf', required: true)
protoc_exec = find_program('protoc')
protoc = generator(protoc_exec,
  output: ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments: ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@']
)

includes = include_directories('src')

subdir('src')

install_data('misc/conf/com.deepin.Cooperation.conf',
             install_dir: 'share/dbus-1/system.d/')
