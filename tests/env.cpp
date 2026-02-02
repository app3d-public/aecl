#include "env.hpp"
#include <cassert>

void create_test_environment(test_environment &env)
{
    const char *data_dir = getenv("TEST_DATA_DIR");
    const char *output_dir = getenv("TEST_OUTPUT_DIR");
    assert(data_dir && output_dir);

    env.data_dir = data_dir;
    env.output_dir = output_dir;
}