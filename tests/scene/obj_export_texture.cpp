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
        exporter.objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
        exporter.materialFlags = MaterialExportFlags::texture_copyToLocal;

        createObjects(exporter.objects);
        auto mat = acul::make_shared<umbf::MatRangeAssignAtrr>();
        mat->matID = 0;
        exporter.objects.front().meta.push_back(mat);

        createMaterials(exporter.materials);

        acul::string texture;
        createDefaultTexture(texture, dataDir);
        exporter.textures.push_back(texture);

        auto state = exporter.save();
        exporter.clear();
        return state;
    }
} // namespace tests