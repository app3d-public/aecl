#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir)
    {
        using namespace ecl::scene;
        obj::Exporter exporter(outputDir / "export_origin.obj");
        exporter.meshFlags =
            MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverseY;
        exporter.materialFlags = MaterialExportFlags::texture_origin;
        exporter.objFlags = obj::ObjExportFlagBits::mgp_objects | obj::ObjExportFlagBits::mat_PBR;

        createObjects(exporter.objects);
        auto mat = acul::make_shared<umbf::MatRangeAssignAtrr>();
        mat->matID = 0;
        exporter.objects.front().meta.push_back(mat);

        createMaterials(exporter.materials);

        acul::string texture;
        createGeneratedTexture(texture, outputDir / "tex");
        exporter.textures.push_back(texture);

        return exporter.save();
    }
} // namespace tests