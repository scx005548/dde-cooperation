project(
    'dde-cooperation',
    'cpp',
    version: '0.0.1',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
    ]
)

prefixdir = get_option('prefix')
datadir = prefixdir / get_option('datadir')
libexecdir = prefixdir / get_option('libexecdir')

cxx = meson.get_compiler('cpp')
stdcxxfs = cxx.find_library('stdc++fs', required: false)

thread = dependency('threads')
uuid = dependency('uuid', required: true)
fmt = dependency('fmt', required: true)
spdlog = dependency('spdlog', required: true)
openssl = dependency('openssl', required: true)
uv = dependency('libuv', required: true)
glibmm = dependency('glibmm-2.4', required: true)
giomm = dependency('giomm-2.4', required: true)
libevdev = dependency('libevdev', required: true)
fuse3 = dependency('fuse3', required: true)
xcb = dependency('xcb', required: true)
xcb_randr = dependency('xcb-randr', required: true)
xcb_xinput = dependency('xcb-xinput', required: true)
xcb_xfixes = dependency('xcb-xfixes', required: true)

qt5 = import('qt5')
qt5dep = dependency('qt5', modules: ['Core', 'Gui', 'DBus'])

protobuf = dependency('protobuf', required: true)
protoc_exec = find_program('protoc')
protoc = generator(protoc_exec,
  output: ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments: ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '--experimental_allow_proto3_optional', '@INPUT@']
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')
systemd_user_unit_dir = systemd.get_pkgconfig_variable('systemduserunitdir')


subdir('src')

install_data('misc/dbus/system.d/com.deepin.Cooperation.conf',
             install_dir: join_paths(datadir, 'dbus-1', 'system.d'))
install_data('misc/dbus/system-services/com.deepin.Cooperation.service',
             install_dir: join_paths(datadir, 'dbus-1', 'system-services'))

data = configuration_data()
data.set('libexecdir', libexecdir)

configure_file(
  input: 'misc/systemd-service/dde-cooperation-daemon.service.in',
  output: '@BASENAME@',
  configuration: data,
  install_dir: systemd_system_unit_dir,
)

configure_file(
  input: 'misc/systemd-service/dde-cooperation.service.in',
  output: '@BASENAME@',
  configuration: data,
  install_dir: systemd_user_unit_dir,
)
