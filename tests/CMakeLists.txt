cmake_minimum_required(VERSION 3.17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

set(ECL_TEST_INCLUDES
    $<TARGET_PROPERTY:app3d-core,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:app3d-assets,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:app3d-ecl,INCLUDE_DIRECTORIES>)
set(ECL_TEST_LIBRARIES
    app3d-core
    app3d-assets
    app3d-ecl
    ${OPENIMAGEIO_LIBRARIES}
)

set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(TEST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/tests/artifacts")
set(ENV_VAR "TEST_DATA_DIR=${TEST_DATA_DIR};TEST_OUTPUT_DIR=${TEST_OUTPUT_DIR}")

set(ECL_OBJ_IMPORT_SRC main.cpp "scene/obj_import.cpp")
add_test_files(
    "ecl"
    "obj_import"
    "${ECL_OBJ_IMPORT_SRC}"
    "${ECL_TEST_INCLUDES}"
    "${ECL_TEST_LIBRARIES}"
    "${ENV_VAR}")

set(ECL_OBJ_EXPORT_TRIANGLES_SRC
    main.cpp
    "scene/common.cpp"
    "scene/obj_export_triangles.cpp")
add_test_files(
    "ecl"
    "obj_export_triangles"
    "${ECL_OBJ_EXPORT_TRIANGLES_SRC}"
    "${ECL_TEST_INCLUDES}"
    "${ECL_TEST_LIBRARIES}"
    "${ENV_VAR}"
)

set(ECL_OBJ_EXPORT_TEXTURE_SRC
    main.cpp
    "scene/common.cpp"
    "scene/obj_export_texture.cpp")
add_test_files(
    "ecl"
    "obj_export_texture"
    "${ECL_OBJ_EXPORT_TEXTURE_SRC}"
    "${ECL_TEST_INCLUDES}"
    "${ECL_TEST_LIBRARIES}"
    "${ENV_VAR}"
)

set(ECL_OBJ_EXPORT_TEXGEN_SRC
    main.cpp
    "scene/common.cpp"
    "scene/obj_export_texgen.cpp")
add_test_files(
    "ecl"
    "obj_export_texgen"
    "${ECL_OBJ_EXPORT_TEXGEN_SRC}"
    "${ECL_TEST_INCLUDES}"
    "${ECL_TEST_LIBRARIES}"
    "${ENV_VAR}"
)