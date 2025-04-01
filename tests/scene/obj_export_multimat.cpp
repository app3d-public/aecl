#include <ecl/scene/obj/export.hpp>
#include "common.hpp"
#include "umbf/version.h"

namespace tests
{
    void createMultimatMaterials(acul::vector<umbf::File> &materials, u64 object_id, u64 *materials_ids)
    {
        acul::id_gen generator;
        materials.resize(2);
        {
            auto mat = acul::make_shared<umbf::Material>();
            mat->albedo.rgb = glm::vec3(0.5, 1.0, 0.0f);
            mat->albedo.textured = false;
            mat->albedo.texture_id = -1;
            auto meta = acul::make_shared<umbf::MaterialInfo>();
            meta->name = "ecl:test:mat_first_e";
            meta->id = generator();
            materials_ids[0] = meta->id;
            meta->assignments.push_back(object_id);
            materials[0].header.vendor_sign = UMBF_VENDOR_ID;
            materials[0].header.vendor_version = UMBF_VERSION;
            materials[0].header.spec_version = UMBF_VERSION;
            materials[0].header.type_sign = umbf::sign_block::format::material;
            materials[0].blocks.push_back(mat);
            materials[0].blocks.push_back(meta);
        }
        {
            auto mat = acul::make_shared<umbf::Material>();
            mat->albedo.rgb = glm::vec3(1.0, 0.5, 0.0f);
            mat->albedo.textured = false;
            mat->albedo.texture_id = -1;
            auto meta = acul::make_shared<umbf::MaterialInfo>();
            meta->name = "ecl:test:mat_second_e";
            meta->id = generator();
            materials_ids[1] = meta->id;
            meta->assignments.push_back(object_id);
            materials[1].header.vendor_sign = UMBF_VENDOR_ID;
            materials[1].header.vendor_version = UMBF_VERSION;
            materials[1].header.spec_version = UMBF_VERSION;
            materials[1].header.type_sign = umbf::sign_block::format::material;
            materials[1].blocks.push_back(mat);
            materials[1].blocks.push_back(meta);
        }
    }

    bool runTest(const acul::io::path &dataDir, const acul::io::path &outputDir)
    {
        using namespace ecl::scene;
        obj::Exporter exporter(outputDir / "export_origin.obj");
        exporter.meshFlags =
            MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverseY;
        exporter.materialFlags = MaterialExportFlags::texture_copyToLocal;
        exporter.objFlags = obj::ObjExportFlagBits::mgp_groups | obj::ObjExportFlagBits::mat_PBR;

        createObjects(exporter.objects);
        u64 materials_ids[2];
        createMultimatMaterials(exporter.materials, exporter.objects.front().id, materials_ids);
        // Materials assignments
        auto mat0 = acul::make_shared<umbf::MatRangeAssignAtrr>();
        mat0->matID = materials_ids[0];
        mat0->faces.resize(2);
        mat0->faces[0] = 2;
        mat0->faces[1] = 3;
        exporter.objects.front().meta.push_back(mat0);

        auto mat1 = acul::make_shared<umbf::MatRangeAssignAtrr>();
        mat1->matID = materials_ids[1];
        mat1->faces.resize(2);
        mat1->faces[0] = 4;
        mat1->faces[1] = 5;
        exporter.objects.front().meta.push_back(mat1);

        auto state = exporter.save();
        exporter.clear();
        return state;
    }

} // namespace tests