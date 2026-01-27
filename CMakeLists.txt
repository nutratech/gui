cmake_minimum_required(VERSION 3.16)

project(nutra LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- SECURITY FIX: Bypass compromised system symlink ---
if(EXISTS "/lib/x86_64-linux-gnu/libkeyutils.so.1.9")
    message(STATUS "Security Bypass: Forcing linkage to explicit libkeyutils.so.1.9")
    add_library(SystemKeyUtils SHARED IMPORTED)
    set_target_properties(SystemKeyUtils PROPERTIES IMPORTED_LOCATION "/lib/x86_64-linux-gnu/libkeyutils.so.1.9")
endif()
# -------------------------------------------------------

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network)

if(Qt${QT_VERSION_MAJOR}Network_FOUND)
    message(STATUS "Qt Network module found. Enabling NLP interactions.")
    add_compile_definitions(NUTRA_HAS_NETWORK)
else()
    message(WARNING "Qt Network module NOT found. NLP interactions will be disabled.")
endif()

# --- AUTOMATIC SOURCE DISCOVERY ---
# 'CONFIGURE_DEPENDS' ensures CMake re-runs if you add new files
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/db/*.cpp" "src/utils/*.cpp" "include/db/*.h" "include/utils/*.h")
file(GLOB_RECURSE UI_SOURCES CONFIGURE_DEPENDS "src/widgets/*.cpp" "src/mainwindow.cpp" "src/main.cpp" "include/widgets/*.h" "include/mainwindow.h")

# Conditionally remove/add PythonServiceManager based on Network module
if(NOT Qt${QT_VERSION_MAJOR}Network_FOUND)
    # Filter out the python service manager if we don't have network
    list(FILTER CORE_SOURCES EXCLUDE REGEX "pythonservicemanager.cpp")
endif()

# Versioning Logic
if(NOT NUTRA_VERSION)
    execute_process(
        COMMAND git describe --tags --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(NUTRA_VERSION "${GIT_VERSION}")
    if(NOT NUTRA_VERSION)
        set(NUTRA_VERSION "v0.0.0-unknown")
    endif()
endif()
add_compile_definitions(NUTRA_VERSION_STRING="${NUTRA_VERSION}")

# --- ASSETS: Copy SQL/SQLite files from libraries ---

# 1. ntsqlite: Copy tables.sql
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib/ntsqlite/sql")
file(COPY "${CMAKE_SOURCE_DIR}/lib/ntsqlite/sql/" 
     DESTINATION "${CMAKE_BINARY_DIR}/lib/ntsqlite/sql"
     FILES_MATCHING PATTERN "*.sql" PATTERN "*.sqlite3")

# 2. usdasqlite: Copy usda.sqlite3 and schema files
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lib/usdasqlite/sql")
file(COPY "${CMAKE_SOURCE_DIR}/lib/usdasqlite/sql/" 
     DESTINATION "${CMAKE_BINARY_DIR}/lib/usdasqlite/sql"
     FILES_MATCHING PATTERN "*.sql" PATTERN "*.sqlite3")
# ----------------------------------------------------

# Main Executable
add_executable(nutra ${CORE_SOURCES} ${UI_SOURCES} "resources.qrc")
target_include_directories(nutra PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nutra PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

if(Qt${QT_VERSION_MAJOR}Network_FOUND)
    target_link_libraries(nutra PRIVATE Qt${QT_VERSION_MAJOR}::Network)
endif()

# --- SECURITY FIX LINKING ---
if(TARGET SystemKeyUtils)
    target_link_libraries(nutra PRIVATE SystemKeyUtils)
endif()

# Testing
enable_testing()
find_package(Qt${QT_VERSION_MAJOR}Test REQUIRED)

# Dynamic Test Discovery
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/test_*.cpp")

add_custom_target(build_tests)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCE} ${CORE_SOURCES})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)
    if(Qt${QT_VERSION_MAJOR}Network_FOUND)
        target_link_libraries(${TEST_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
    endif()

    if(TARGET SystemKeyUtils)
        target_link_libraries(${TEST_NAME} PRIVATE SystemKeyUtils)
    endif()

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    add_dependencies(build_tests ${TEST_NAME})
endforeach()

# Installation
include(GNUInstallDirs)
set(NUTRA_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/nutra")
configure_file(nutra.desktop.in ${CMAKE_BINARY_DIR}/nutra.desktop @ONLY)

install(TARGETS nutra DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_BINARY_DIR}/nutra.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES resources/nutrition_icon-no_bg.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps RENAME nutra.png)

if(NUTRA_DB_FILE AND EXISTS "${NUTRA_DB_FILE}")
    install(FILES "${NUTRA_DB_FILE}" DESTINATION share/nutra RENAME usda.sqlite3)
endif()

# Install Python NLP service
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/pylang_serv/pylang_serv")
    install(DIRECTORY lib/pylang_serv/pylang_serv
            DESTINATION share/nutra/pylang_serv
            FILES_MATCHING PATTERN "*.py")
    if(EXISTS "${CMAKE_SOURCE_DIR}/lib/pylang_serv/pyproject.toml")
        install(FILES lib/pylang_serv/pyproject.toml
                DESTINATION share/nutra/pylang_serv)
    endif()
endif()

# AppImage generation
find_program(LINUXDEPLOY linuxdeploy)

if(LINUXDEPLOY)
    add_custom_target(appimage
        COMMAND ${CMAKE_COMMAND} --install . --prefix AppDir/usr
        COMMAND sed -i "s|^Exec=.*|Exec=nutra|" AppDir/usr/share/applications/nutra.desktop
        COMMAND ${LINUXDEPLOY} --appdir=AppDir --plugin=qt --output=appimage
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating AppImage..."
        VERBATIM
    )
    add_dependencies(appimage nutra)
endif()
