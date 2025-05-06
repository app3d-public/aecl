#include <acul/log.hpp>

struct test_environment
{
    acul::task::service_dispatch sd;
    acul::log::log_service *log_service;
    acul::string data_dir;
    acul::string output_dir;
};

void create_test_environment(test_environment &env);