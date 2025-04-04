#include <acul/log.hpp>

namespace tests
{
    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir);
}

int main(int argc, char *argv[])
{
    acul::task::service_dispatch sd;
    acul::log::log_service *log_service = acul::alloc<acul::log::log_service>();
    sd.register_service(log_service);
    auto *app_logger = log_service->add_logger<acul::log::console_logger>("app");
    log_service->level = acul::log::level::trace;
    app_logger->set_pattern("%(color_auto)%(level_name)\t%(message)%(color_off)\n");
    log_service->default_logger = app_logger;
    const char *dataDir = getenv("TEST_DATA_DIR");
    const char *outputDir = getenv("TEST_OUTPUT_DIR");
    if (!dataDir || !outputDir) return 1;
    bool result = tests::runTest(dataDir, outputDir);
    log_service->await();
    return result ? 0 : 1;
}