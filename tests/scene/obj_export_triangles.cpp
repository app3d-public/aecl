#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
    {
        using namespace ecl::scene;
        MeshExportFlags meshFlags = MeshExportFlagBits::export_normals | MeshExportFlagBits::export_triangulated;
        MaterialExportFlags materialFlags = MaterialExportFlagBits::none;
        obj::ObjExportFlags objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
        obj::Exporter exporter(outputDir / "export_origin.obj", meshFlags, materialFlags, objFlags);
        DArray<std::shared_ptr<assets::Object>> objects;
        createObjects(objects);
        exporter.objects(objects);
        auto state = exporter.save();
        exporter.clear();
        return state;
    }
} // namespace tests