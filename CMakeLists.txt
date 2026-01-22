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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "include/*.h")
set(PROJECT_SOURCES ${SOURCES} ${HEADERS} "resources.qrc")

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






add_executable(nutra
    ${PROJECT_SOURCES}
)

target_include_directories(nutra PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(nutra PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

enable_testing()
find_package(Qt${QT_VERSION_MAJOR}Test REQUIRED)

add_executable(test_nutra EXCLUDE_FROM_ALL tests/test_foodrepository.cpp src/db/databasemanager.cpp src/db/foodrepository.cpp src/utils/string_utils.cpp)
target_include_directories(test_nutra PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_nutra PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)

add_test(NAME FoodRepoTest COMMAND test_nutra)

add_executable(test_databasemanager tests/test_databasemanager.cpp src/db/databasemanager.cpp src/db/foodrepository.cpp src/utils/string_utils.cpp)
target_include_directories(test_databasemanager PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_databasemanager PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Sql)
add_test(NAME DatabaseManagerTest COMMAND test_databasemanager)

add_executable(test_calculations tests/test_calculations.cpp)
target_include_directories(test_calculations PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_calculations PRIVATE Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME CalculationsTest COMMAND test_calculations)


include(GNUInstallDirs)
set(NUTRA_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/nutra")
configure_file(nutra.desktop.in ${CMAKE_BINARY_DIR}/nutra.desktop @ONLY)

install(TARGETS nutra DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_BINARY_DIR}/nutra.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(FILES resources/nutrition_icon-no_bg.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps RENAME nutra.png)

if(NUTRA_DB_FILE AND EXISTS "${NUTRA_DB_FILE}")
    install(FILES "${NUTRA_DB_FILE}" DESTINATION share/nutra RENAME usda.sqlite3)
endif()
