cmake_minimum_required(VERSION 3.10)
project(GitGUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
include(GNUInstallDirs)

add_executable(gitgui main.cpp)

target_link_libraries(gitgui ${GTKMM_LIBRARIES})
target_include_directories(gitgui PRIVATE ${GTKMM_INCLUDE_DIRS})
target_compile_options(gitgui PRIVATE ${GTKMM_CFLAGS_OTHER})

install(TARGETS gitgui
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
