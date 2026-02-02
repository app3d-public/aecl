#include <aecl/scene/obj/export.hpp>
#include "../env.hpp"
#include "common.hpp"

void test_obj_export_texture()
{
    test_environment env;
    create_test_environment(env);
    using namespace aecl::scene;
    obj::Exporter exporter(acul::path(env.output_dir) / "export_origin.obj");
    exporter.mesh_flags =
        MeshExportFlagBits::export_normals | MeshExportFlagBits::export_uv | MeshExportFlagBits::transform_reverse_y;
    exporter.obj_flags = obj::ObjExportFlagBits::object_policy_groups | obj::ObjExportFlagBits::materials_pbr;
    exporter.material_flags = MaterialExportFlags::texture_copy;

    create_objects(exporter.objects);
    auto mat = acul::make_shared<umbf::MaterialRange>();
    mat->mat_id = 0;
    exporter.objects.front().meta.push_back(mat);

    create_materials(exporter.materials);

    acul::string texture;
    create_default_texture(texture, env.data_dir);
    exporter.textures.push_back(texture);

    auto state = exporter.save();
    exporter.clear();
    assert(state.success());
}
