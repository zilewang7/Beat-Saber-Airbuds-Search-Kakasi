cmake_minimum_required(VERSION 3.22)

option(QUEST "Build for quest" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_compile_options(-frtti -fexceptions -fvisibility=hidden -fPIE -fPIC)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/qpm.cmake)

if (QUEST)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/targets/quest.cmake)
endif ()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/strip.cmake)

project(airbuds-search-kakasi VERSION ${PACKAGE_VERSION})

add_compile_definitions(MOD_ID="${CMAKE_PROJECT_NAME}")
add_compile_definitions(VERSION="${CMAKE_PROJECT_VERSION}")

set(COMPILE_ID ${CMAKE_PROJECT_NAME})

RECURSE_FILES(cpp_file_list ${SOURCE_DIR}/*.cpp)
RECURSE_FILES(c_file_list ${SOURCE_DIR}/*.c)

add_library(
    ${CMAKE_PROJECT_NAME}
    SHARED
    ${cpp_file_list}
    ${c_file_list}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${SHARED_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE log)

set(KAKASI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/kakasi")
set(KAKASI_LIB "${KAKASI_ROOT}/lib/libkakasi.a")
set(KAKASI_ICONV_LIB "${KAKASI_ROOT}/lib/libiconv.a")
set(KAKASI_CHARSET_LIB "${KAKASI_ROOT}/lib/libcharset.a")
set(KAKASI_INCLUDE_DIR "${KAKASI_ROOT}/include")

if (EXISTS "${KAKASI_LIB}" AND EXISTS "${KAKASI_INCLUDE_DIR}/libkakasi.h")
    add_library(kakasi STATIC IMPORTED GLOBAL)
    set_target_properties(kakasi PROPERTIES
        IMPORTED_LOCATION "${KAKASI_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${KAKASI_INCLUDE_DIR}"
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE kakasi)
    if (EXISTS "${KAKASI_ICONV_LIB}" AND EXISTS "${KAKASI_CHARSET_LIB}")
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${KAKASI_ICONV_LIB}" "${KAKASI_CHARSET_LIB}")
    else ()
        message(FATAL_ERROR "Kakasi libiconv/libcharset not found under ${KAKASI_ROOT}")
    endif ()
else ()
    message(FATAL_ERROR "Kakasi not found at ${KAKASI_ROOT}")
endif ()
