#include <aecl/scene/obj/export.hpp>
#include <umbf/version.h>
#include "../env.hpp"
#include "common.hpp"

void create_multi_materials(acul::vector<umbf::File> &materials, u64 object_id, u64 *materials_ids)
{
    acul::id_gen generator;
    materials.resize(2);
    {
        auto mat = acul::make_shared<umbf::Material>();
        mat->albedo.rgb = amal::vec3(0.5, 1.0, 0.0f);
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
        mat->albedo.rgb = amal::vec3(1.0, 0.5, 0.0f);
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

void test_obj_export_multimat()
{
    using namespace aecl::scene;

    test_environment env;
    create_test_environment(env);
    obj::Exporter exporter(acul::path(env.output_dir) / "export_origin.obj");
    exporter.mesh_flags =
        MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverse_y;
    exporter.material_flags = MaterialExportFlags::texture_copy;
    exporter.obj_flags = obj::ObjExportFlagBits::object_policy_groups | obj::ObjExportFlagBits::materials_pbr;

    create_objects(exporter.objects);
    u64 materials_ids[2];
    create_multi_materials(exporter.materials, exporter.objects.front().id, materials_ids);
    // Materials assignments
    auto mat0 = acul::make_shared<umbf::MaterialRange>();
    mat0->mat_id = materials_ids[0];
    mat0->faces.resize(2);
    mat0->faces[0] = 2;
    mat0->faces[1] = 3;
    exporter.objects.front().meta.push_back(mat0);

    auto mat1 = acul::make_shared<umbf::MaterialRange>();
    mat1->mat_id = materials_ids[1];
    mat1->faces.resize(2);
    mat1->faces[0] = 4;
    mat1->faces[1] = 5;
    exporter.objects.front().meta.push_back(mat1);

    auto state = exporter.save();
    exporter.clear();
    assert(state.success());
}