cmake_minimum_required(VERSION 3.16)

project(nutra LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 or Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)


# Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h")

# Filter out main.cpp for the library
list(FILTER SOURCES EXCLUDE REGEX ".*src/main\\.cpp$")

# Versioning
if(NOT NUTRA_VERSION)
    execute_process(
        COMMAND git describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_VERSION)
        set(NUTRA_VERSION "${GIT_VERSION}")
    else()
        set(NUTRA_VERSION "v0.0.0-unknown")
    endif()
endif()
add_compile_definitions(NUTRA_VERSION_STRING="${NUTRA_VERSION}")

# Core Library
add_library(nutra_lib STATIC ${SOURCES} ${HEADERS} "resources.qrc")
target_include_directories(nutra_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nutra_lib PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

# Main Executable
add_executable(nutra src/main.cpp)
target_link_libraries(nutra PRIVATE nutra_lib)

# Testing
enable_testing()
find_package(Qt${QT_VERSION_MAJOR}Test REQUIRED)

# Dynamic Test Discovery
file(GLOB TEST_SOURCES "tests/test_*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE nutra_lib Qt${QT_VERSION_MAJOR}::Test)
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()


include(GNUInstallDirs)
set(NUTRA_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/nutra")
configure_file(nutra.desktop.in ${CMAKE_BINARY_DIR}/nutra.desktop @ONLY)

install(TARGETS nutra DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_BINARY_DIR}/nutra.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES resources/nutrition_icon-no_bg.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps RENAME nutra.png)

if(NUTRA_DB_FILE AND EXISTS "${NUTRA_DB_FILE}")
    install(FILES "${NUTRA_DB_FILE}" DESTINATION share/nutra RENAME usda.sqlite3)
endif()

# AppImage generation (requires linuxdeploy and linuxdeploy-plugin-qt in PATH)
find_program(LINUXDEPLOY linuxdeploy)

if(LINUXDEPLOY)
    add_custom_target(appimage
        COMMAND ${CMAKE_COMMAND} --install . --prefix AppDir/usr
        COMMAND ${LINUXDEPLOY} --appdir=AppDir --plugin=qt --output=appimage
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating AppImage..."
        VERBATIM
    )
    add_dependencies(appimage nutra)
endif()
