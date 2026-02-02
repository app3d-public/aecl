#include <acul/log.hpp>

struct test_environment
{
    acul::string data_dir;
    acul::string output_dir;
};

void create_test_environment(test_environment &env);