#include "env.hpp"
#include <cassert>
#include <umbf/ext/image/image.hpp>
#include <umbf/ext/material/material.hpp>
#include <umbf/ext/scene/scene.hpp>

void create_test_environment(test_environment &env)
{
    umbf::insert_default_block_streams(env.resolver);
    umbf::insert_image_streams(env.resolver);
    umbf::insert_material_streams(env.resolver);
    umbf::insert_scene_streams(env.resolver);
    umbf::insert_default_segment_codecs(env.resolver);
    umbf::insert_image_codecs(env.resolver);
    umbf::registry::resolver = &env.resolver;

    const char *data_dir = getenv("TEST_DATA_DIR");
    const char *output_dir = getenv("TEST_OUTPUT_DIR");
    assert(data_dir && output_dir);

    env.data_dir = data_dir;
    env.output_dir = output_dir;
}

test_environment::~test_environment()
{
    if (umbf::registry::resolver == &resolver) umbf::registry::resolver = nullptr;
}
