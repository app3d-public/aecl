#include <core/log.hpp>
#include <cstdlib>

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir);
}

int main(int argc, char *argv[])
{
    task::ServiceDispatch sd;
    logging::g_LogService = astl::alloc<logging::LogService>();
    sd.registerService(logging::g_LogService);
    logging::g_DefaultLogger = logging::g_LogService->addLogger<logging::ConsoleLogger>("app");
    logging::g_LogService->level(logging::Level::Trace);
    logging::g_DefaultLogger->setPattern("%(color_auto)%(level_name)\t%(message)%(color_off)\n");
    const char *dataDir = getenv("TEST_DATA_DIR");
    const char *outputDir = getenv("TEST_OUTPUT_DIR");
    if (!dataDir || !outputDir) return 1;
    bool result = tests::runTest(dataDir, outputDir);
    logging::g_LogService->await();
    return result ? 0 : 1;
}