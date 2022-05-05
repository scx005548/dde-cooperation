# cmake version
cmake_minimum_required ( VERSION   3.11 )

project ( deepin-coopertation )

set(TARGERT_NAME )

set(CMAKE_CXX_STANDARD 11)

find_package (PkgConfig REQUIRED)

pkg_check_modules (GLIBMM REQUIRED glibmm-2.4)
include_directories (${GLIBMM_INCLUDE_DIRS})
link_directories (${GLIBMM_LIBRARY_DIRS})

pkg_check_modules (GIOMM REQUIRED giomm-2.4)
include_directories (${GIOMM_INCLUDE_DIRS})
link_directories (${GIOMM_LIBRARY_DIRS})

pkg_check_modules (LIBNM REQUIRED libnm)
include_directories (${LIBNM_INCLUDE_DIRS})
link_directories (${LIBNM_LIBRARY_DIRS})

pkg_check_modules (PROTOBUF REQUIRED protobuf)
include_directories (${PROTOBUF_INCLUDE_DIRS})
link_directories (${PROTOBUF_LIBRARY_DIRS})

include_directories (./src/dbus)
aux_source_directory( ./src/dbus DBUS_SRC)

include_directories (./src/protocol)
aux_source_directory( ./src/protocol PROTOCOL_SRC)

include_directories (./src/utils)
aux_source_directory( ./src/utils UTILS_SRC)

include_directories (./src/modules)
aux_source_directory( ./src/modules MODULES_SRC)

include_directories (./src)
aux_source_directory( ./src ROOT_SRC)

# 编译目标
add_executable( ${PROJECT_NAME}
                ${DBUS_SRC}
                ${PROTOCOL_SRC}
                ${UTILS_SRC}
                ${MODULES_SRC}
                ${ROOT_SRC}
                )

# 链接参数
target_link_libraries (${PROJECT_NAME}
                        ${GLIBMM_LIBRARIES}
                        ${GIOMM_LIBRARIES}
                        ${LIBNM_LIBRARIES}
                        ${PROTOBUF_LIBRARIES}
                        )

# 安装
install(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/misc/conf/com.deepin.system.Cooperation.conf 
        DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/share/dbus-1/system.d/
        )