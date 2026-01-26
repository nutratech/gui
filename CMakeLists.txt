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
file(GLOB_RECURSE CORE_SOURCES_CPP CONFIGURE_DEPENDS "src/db/*.cpp" "src/utils/*.cpp")
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS "include/db/*.h" "include/utils/*.h")
set(CORE_SOURCES ${CORE_SOURCES_CPP} ${CORE_HEADERS})

file(GLOB_RECURSE UI_SOURCES_CPP CONFIGURE_DEPENDS "src/widgets/*.cpp" "src/mainwindow.cpp")
file(GLOB_RECURSE UI_HEADERS CONFIGURE_DEPENDS "include/widgets/*.h" "include/mainwindow.h")
set(UI_SOURCES ${UI_SOURCES_CPP} ${UI_HEADERS})

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

# Main Executable
add_executable(nutra src/main.cpp ${CORE_SOURCES} ${UI_SOURCES} "resources.qrc")
target_include_directories(nutra PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nutra PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

# Testing
enable_testing()
find_package(Qt${QT_VERSION_MAJOR}Test REQUIRED)

# Dynamic Test Discovery
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/test_*.cpp")

add_custom_target(build_tests)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # Create independent test executable with only CORE sources
    add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCE} ${CORE_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    add_dependencies(build_tests ${TEST_NAME})
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
