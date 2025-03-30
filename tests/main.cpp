#include <acul/log.hpp>

namespace tests
{
    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir);
}

int main(int argc, char *argv[])
{
    task::ServiceDispatch sd;
    acul::log::g_LogService = acul::alloc<acul::log::LogService>();
    sd.registerService(acul::log::g_LogService);
    acul::log::g_DefaultLogger = acul::log::g_LogService->addLogger<acul::log::ConsoleLogger>("app");
    acul::log::g_LogService->level(acul::log::Level::Trace);
    acul::log::g_DefaultLogger->setPattern("%(color_auto)%(level_name)\t%(message)%(color_off)\n");
    const char *dataDir = getenv("TEST_DATA_DIR");
    const char *outputDir = getenv("TEST_OUTPUT_DIR");
    if (!dataDir || !outputDir) return 1;
    bool result = tests::runTest(dataDir, outputDir);
    acul::log::g_LogService->await();
    return result ? 0 : 1;
}