#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
    {
        using namespace ecl::scene;
        obj::Exporter exporter(outputDir / "export_origin.obj");
        exporter.meshFlags =
            MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverseY;
        exporter.materialFlags = MaterialExportFlagBits::texture_origin;
        exporter.objFlags = obj::ObjExportFlagBits::mgp_objects | obj::ObjExportFlagBits::mat_PBR;

        createObjects(exporter.objects);
        auto mat = astl::make_shared<assets::MatRangeAssignAtrr>();
        mat->matID = 0;
        exporter.objects.front().meta.push_back(mat);

        createMaterials(exporter.materials);

        assets::Target::Addr texture;
        createGeneratedTexture(texture, outputDir / "tex");
        exporter.textures.push_back(texture);

        return exporter.save();
    }
} // namespace tests