#include <ecl/scene/obj/import.hpp>

namespace tests
{
    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir)
    {
        acul::events::dispatcher ed;
        ecl::scene::obj::Importer importer(dataDir / "cube.obj");
        auto state = importer.load(ed);
        importer.clear();
        return state == acul::io::file::op_state::success;
    }
} // namespace tests