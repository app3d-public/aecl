#include <ecl/scene/obj/import.hpp>

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
    {
        ecl::scene::obj::Importer importer(dataDir / "cube.obj");
        auto state = importer.load();
        importer.clear();
        return state == io::file::ReadState::Success;
    }
} // namespace tests