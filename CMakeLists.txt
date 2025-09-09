cmake_minimum_required(VERSION 3.17)

if(NOT DEFINED ACBT_PROJECT_LOADED)
    include(cmake/project.cmake)
endif()

project(aecl
    VERSION 0.0.1
    LANGUAGES CXX C
    DESCRIPTION "App3d Extended Content Loader")

if(NOT DEFINED ACBT_LOADED)
    include(cmake/utils.cmake)
endif()

add_files(${PROJECT_NAME} RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if (NOT TARGET acul)
    add_subdirectory(modules/acul)
endif()
if (NOT TARGET umbf)
    add_subdirectory(modules/umbf)
endif()

add_library(${PROJECT_NAME} SHARED ${AECL_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DAPPLIB_BUILD)
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/3rd-party/"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/3rd-party/earcut/include/"
)

find_package(PkgConfig)
pkg_search_module(OPENIMAGEIO REQUIRED OpenImageIO)

target_link_libraries(${PROJECT_NAME} PRIVATE
    acul
    umbf
    ${OPENIMAGEIO_LIBRARIES}
)

target_compile_options(${PROJECT_NAME} PRIVATE -march=native)

if(USE_ASAN)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
endif(USE_ASAN)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(WIN32)
    gen_manifest_lib(${PROJECT_NAME} ${PROJECT_VERSION})
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    CXX_EXTENSIONS YES
)