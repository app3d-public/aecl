#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
    {
        using namespace ecl::scene;
        MeshExportFlags meshFlags =
            MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverseY;
        MaterialExportFlags materialFlags = MaterialExportFlagBits::texture_origin;
        obj::ObjExportFlags objFlags = obj::ObjExportFlagBits::mgp_objects | obj::ObjExportFlagBits::mat_PBR;
        obj::Exporter exporter(outputDir / "export_origin.obj", meshFlags, materialFlags, objFlags);

        DArray<std::shared_ptr<assets::Object>> objects;
        createObjects(objects);
        auto mat = std::make_shared<assets::MatRangeAssignAtrr>();
        mat->matID = 0;
        objects.front()->meta.push_back(mat);
        exporter.objects(objects);
        exporter.objects(objects);

        DArray<std::shared_ptr<assets::Asset>> materials;
        createMaterials(materials);
        exporter.materials(materials);

        DArray<assets::Target::Addr> textures(1);
        createGeneratedTexture(textures.front(), outputDir / "tex");
        exporter.textures(textures);

        return exporter.save();
    }
} // namespace tests