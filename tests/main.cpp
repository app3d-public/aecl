#include <core/log.hpp>
#include <core/task.hpp>
#include <cstdlib>

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir);
}

int main(int argc, char *argv[])
{
    logging::LogManager::init();
    auto logger = logging::mng->addLogger<logging::ConsoleLogger>("backend");
    logging::mng->level(logging::Level::Trace);
    logger->setPattern("%(color_auto)%(level_name)\t%(message)%(color_off)\n");
    logging::mng->defaultLogger(logger);
    const char *dataDir = getenv("TEST_DATA_DIR");
    const char *outputDir = getenv("TEST_OUTPUT_DIR");
    if (!dataDir || !outputDir) return 1;
    bool result = tests::runTest(dataDir, outputDir);
    TaskManager::global().await(true);
    logging::mng->await();
    logging::LogManager::destroy();
    return result ? 0 : 1;
}