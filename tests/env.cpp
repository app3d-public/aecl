#include "env.hpp"

void create_test_environment(test_environment &env)
{
    env.sd.run();
    env.log_service = acul::alloc<acul::log::log_service>();
    env.sd.register_service(env.log_service);
    env.log_service->level = acul::log::level::Trace;
    auto *app_logger = env.log_service->add_logger<acul::log::console_logger>("app");
    env.log_service->level = acul::log::level::Trace;
    app_logger->set_pattern("%(message)\n");
    env.log_service->default_logger = app_logger;
    
    const char *data_dir = getenv("TEST_DATA_DIR");
    const char *output_dir = getenv("TEST_OUTPUT_DIR");
    assert(data_dir && output_dir);

    env.data_dir = data_dir;
    env.output_dir = output_dir;
}