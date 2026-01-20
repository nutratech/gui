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

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
    src/db/databasemanager.cpp
    include/db/databasemanager.h
    src/db/foodrepository.cpp
    include/db/foodrepository.h
    src/widgets/searchwidget.cpp
    include/widgets/searchwidget.h
    src/widgets/detailswidget.cpp
    include/widgets/detailswidget.h
    src/widgets/mealwidget.cpp
    include/widgets/mealwidget.h
    src/utils/string_utils.cpp
    include/utils/string_utils.h
    resources.qrc
)






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


install(TARGETS nutra DESTINATION bin)
install(FILES nutra.desktop DESTINATION share/applications)
install(FILES resources/nutrition_icon-no_bg.png DESTINATION share/icons/hicolor/128x128/apps RENAME nutra.png)

if(NUTRA_DB_FILE AND EXISTS "${NUTRA_DB_FILE}")
    install(FILES "${NUTRA_DB_FILE}" DESTINATION share/nutra RENAME usda.sqlite3)
endif()
