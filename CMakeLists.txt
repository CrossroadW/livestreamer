cmake_minimum_required(VERSION 3.30)
project(livestreamer CXX)


find_package(Qt5 CONFIG REQUIRED COMPONENTS Widgets Gui Multimedia MultimediaWidgets)
find_package(spdlog CONFIG REQUIRED)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/*.h)
file(GLOB UI ui/*.ui)
message(STATUS "${SOURCES} ${HEADERS} ${UI}")
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
        PRIVATE ${SOURCES} ${HEADERS} ${UI}
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets spdlog::spdlog
        Qt5::Multimedia Qt5::MultimediaWidgets
        Qt5::Gui )
set_target_properties(${PROJECT_NAME} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
        CMAKE_CURRENT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFmpeg MODULE REQUIRED COMPONENTS avformat avcodec avutil swscale swresample
        avdevice avfilter)

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS} src)
target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})