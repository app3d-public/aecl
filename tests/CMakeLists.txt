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
file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}/tex")
set(TEST_ENV "TEST_DATA_DIR=${TEST_DATA_DIR};TEST_OUTPUT_DIR=${TEST_OUTPUT_DIR}")
set(TEST_SOURCES env.cpp image/import/common.cpp scene/common.cpp)

# Image
# Import
add_test_files(aecl image_import_bmp image/import/bmp.cpp)
add_test_files(aecl image_import_gif image/import/gif.cpp)
add_test_files(aecl image_import_hdr image/import/hdr.cpp)
add_test_files(aecl image_import_heif image/import/heif.cpp)
add_test_files(aecl image_import_jpeg image/import/jpeg.cpp)
add_test_files(aecl image_import_exr image/import/exr.cpp)
add_test_files(aecl image_import_png image/import/png.cpp)
add_test_files(aecl image_import_ppm image/import/ppm.cpp)
add_test_files(aecl image_import_tga image/import/tga.cpp)
add_test_files(aecl image_import_tiff image/import/tiff.cpp)
add_test_files(aecl image_import_webp image/import/webp.cpp)
add_test_files(aecl image_import_umbf image/import/umbf.cpp)

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