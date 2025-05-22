# FindFFmpeg.cmake
# Usage: find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat ...)

include(FindPackageHandleStandardArgs)
find_package(PkgConfig REQUIRED)

set(_FFMPEG_MODULES
        avcodec
        avformat
        avutil
        swscale
        swresample
        avdevice
        avfilter
)

foreach(_mod IN LISTS FFmpeg_FIND_COMPONENTS)
    string(TOLOWER ${_mod} _mod_l)
    string(TOUPPER ${_mod} _mod_u)
    pkg_check_modules(FFMPEG_${_mod_u} REQUIRED lib${_mod_l})

    list(APPEND FFMPEG_INCLUDE_DIRS ${FFMPEG_${_mod_u}_INCLUDE_DIRS})
    list(APPEND FFMPEG_LIBRARIES    ${FFMPEG_${_mod_u}_LIBRARIES})

    execute_process(
            COMMAND pkg-config --variable=pcfiledir lib${_mod_l}
            OUTPUT_VARIABLE _pcdir
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    list(APPEND FFMPEG_PC_PATHS ${_pcdir})
endforeach()

# 去重
list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
list(REMOVE_DUPLICATES FFMPEG_LIBRARIES)
list(REMOVE_DUPLICATES FFMPEG_PC_PATHS)

# 检查变量有效性
find_package_handle_standard_args(FFmpeg REQUIRED_VARS FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES)

# 仅一次性输出最终路径
message(STATUS "[FFmpeg] All required components located.")
message(STATUS "  - FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "  - FFMPEG_LIBRARIES:    ${FFMPEG_LIBRARIES}")
message(STATUS "  - FFMPEG_PC_PATHS:    ${FFMPEG_PC_PATHS}")
