#include <aecl/scene/obj/export.hpp>
#include "../env.hpp"
#include "common.hpp"

void test_obj_export_texgen()
{
    test_environment env;
    create_test_environment(env);
    using namespace aecl::scene;
    auto path = acul::path(env.output_dir);
    obj::Exporter exporter(path / "export_origin.obj");
    exporter.mesh_flags =
        MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverse_y;
    exporter.material_flags = MaterialExportFlags::texture_origin;
    exporter.obj_flags = obj::ObjExportFlagBits::object_policy_objects | obj::ObjExportFlagBits::materials_pbr;

    create_objects(exporter.objects);
    auto mat = acul::make_shared<umbf::MaterialRange>();
    mat->mat_id = 0;
    exporter.objects.front().meta.push_back(mat);

    create_materials(exporter.materials);

    acul::string texture;
    create_generated_texture(texture, path / "tex");
    exporter.textures.push_back(texture);

    assert(exporter.save().success());
}
