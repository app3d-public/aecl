cmake_minimum_required(VERSION 3.17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(TEST_LIBRARIES
    acul
    umbf
    aecl
    ${OPENIMAGEIO_LIBRARIES}
)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tests/artifacts")
set(TEST_ENV "TEST_DATA_DIR=${TEST_DATA_DIR};TEST_OUTPUT_DIR=${TEST_OUTPUT_DIR}")
set(TEST_SOURCES env.cpp scene/common.cpp)

# Image
add_test_files(aecl image_import image/import.cpp)
add_test_files(aecl image_export image/export.cpp)

# Scene
add_test_files(aecl parse_exception scene/parse_exception.cpp)
add_test_files(aecl obj_import scene/obj_import.cpp)
add_test_files(aecl obj_export_triangles scene/obj_export_triangles.cpp)
add_test_files(aecl obj_export_texture scene/obj_export_texture.cpp)
add_test_files(aecl obj_export_texgen scene/obj_export_texgen.cpp)
add_test_files(aecl obj_export_multimat scene/obj_export_multimat.cpp)

if(ENABLE_COVERAGE)
    add_test_coverage(ecl)
endif()