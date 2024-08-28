#include <ecl/scene/obj/export.hpp>
#include "common.hpp"

namespace tests
{
    void createMultimatMaterials(DArray<std::shared_ptr<assets::Asset>> &materials)
    {
        materials.resize(2);
        {
            auto mat = std::make_shared<assets::Material>();
            mat->albedo.rgb = glm::vec3(0.5, 1.0, 0.0f);
            mat->albedo.textured = false;
            mat->albedo.textureID = -1;
            auto meta = std::make_shared<assets::MaterialInfo>();
            meta->name = "ecl:test:mat_first_e";
            meta->assignments.push_back(0);
            materials[0] = std::make_shared<assets::Asset>();
            materials[0]->header.type = assets::Type::Material;
            materials[0]->blocks.push_back(mat);
            materials[0]->blocks.push_back(meta);
        }
        {
            auto mat = std::make_shared<assets::Material>();
            mat->albedo.rgb = glm::vec3(1.0, 0.5, 0.0f);
            mat->albedo.textured = false;
            mat->albedo.textureID = -1;
            auto meta = std::make_shared<assets::MaterialInfo>();
            meta->name = "ecl:test:mat_second_e";
            meta->assignments.push_back(0);
            materials[1] = std::make_shared<assets::Asset>();
            materials[1]->header.type = assets::Type::Material;
            materials[1]->blocks.push_back(mat);
            materials[1]->blocks.push_back(meta);
        }
    }

    bool runTest(const std::filesystem::path &dataDir, const std::filesystem::path &outputDir)
    {
        using namespace ecl::scene;
        MeshExportFlags meshFlags =
            MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverseY;
        MaterialExportFlags materialFlags = MaterialExportFlagBits::texture_copyToLocal;
        obj::ObjExportFlags objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;
        obj::Exporter exporter(outputDir / "export_origin.obj", meshFlags, materialFlags, objFlags);

        DArray<std::shared_ptr<assets::Object>> objects;
        createObjects(objects);
        // Materials assignments
        auto mat0 = std::make_shared<assets::MatRangeAssignAtrr>();
        mat0->matID = 0;
        mat0->faces.resize(2);
        mat0->faces[0] = 2;
        mat0->faces[1] = 3;
        objects.front()->meta.push_back(mat0);

        auto mat1 = std::make_shared<assets::MatRangeAssignAtrr>();
        mat1->matID = 1;
        mat1->faces.resize(2);
        mat1->faces[0] = 4;
        mat1->faces[1] = 5;
        objects.front()->meta.push_back(mat1);
        exporter.objects(objects);

        DArray<std::shared_ptr<assets::Asset>> materials;
        createMultimatMaterials(materials);
        exporter.materials(materials);

        auto state = exporter.save();
        exporter.clear();
        return state;
    }
} // namespace tests