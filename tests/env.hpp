#include <acul/log.hpp>
#include <umbf/umbf.hpp>

struct test_environment
{
    umbf::registry::HashResolver resolver;
    acul::string data_dir;
    acul::string output_dir;

    test_environment() = default;
    ~test_environment();
    test_environment(const test_environment &) = delete;
    test_environment &operator=(const test_environment &) = delete;
};

void create_test_environment(test_environment &env);
