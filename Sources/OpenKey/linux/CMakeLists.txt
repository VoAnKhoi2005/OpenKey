cmake_minimum_required(VERSION 3.16)
project(OpenKeyLinux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(IBUS REQUIRED IMPORTED_TARGET ibus-1.0)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(INDICATOR REQUIRED IMPORTED_TARGET ayatana-appindicator3-0.1)

set(ENGINE_SOURCES
    ../engine/Engine.cpp
    ../engine/Vietnamese.cpp
    ../engine/Macro.cpp
    ../engine/SmartSwitchKey.cpp
    ../engine/ConvertTool.cpp)

add_library(openkey-config STATIC src/config.cpp)
target_link_libraries(openkey-config PUBLIC PkgConfig::IBUS)
target_compile_definitions(openkey-config PUBLIC LINUX)
target_include_directories(openkey-config PUBLIC src ../engine)

add_executable(openkey-ibus src/ibus_engine.cpp ${ENGINE_SOURCES})
target_compile_definitions(openkey-ibus PRIVATE LINUX)
target_link_libraries(openkey-ibus PRIVATE openkey-config PkgConfig::IBUS)
target_include_directories(openkey-ibus PRIVATE ../engine src)

add_executable(openkey-control src/control.cpp)
target_sources(openkey-control PRIVATE ${ENGINE_SOURCES})
target_compile_definitions(openkey-control PRIVATE LINUX)
target_link_libraries(openkey-control PRIVATE openkey-config PkgConfig::GTK PkgConfig::INDICATOR)

configure_file(data/openkey.xml.in openkey.xml @ONLY)

install(TARGETS openkey-ibus openkey-control RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openkey.xml DESTINATION share/ibus/component)
install(FILES data/org.openkey.Linux.gschema.xml DESTINATION share/glib-2.0/schemas)
install(FILES data/openkey-control.desktop DESTINATION share/applications)

set(CPACK_PACKAGE_NAME "openkey-linux")
set(CPACK_PACKAGE_VENDOR "OpenKey contributors")
set(CPACK_PACKAGE_CONTACT "OpenKey contributors")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Vietnamese IBus input method for Ubuntu")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "ibus (>= 1.5), libgtk-3-0, libayatana-appindicator3-1")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_GENERATOR "DEB")
include(CPack)
