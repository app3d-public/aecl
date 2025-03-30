#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir)
    {
        using namespace ecl::scene;
        obj::ObjExportFlags objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
        obj::Exporter exporter(outputDir / "export_origin.obj");
        exporter.materialFlags = MaterialExportFlags::none;
        exporter.meshFlags = MeshExportFlagBits::export_normals | MeshExportFlagBits::export_triangulated;
        exporter.objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
        createObjects(exporter.objects);
        auto state = exporter.save();
        exporter.clear();
        return state;
    }
} // namespace tests