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
    exporter.objects.front().blocks.push_back(acul::static_pointer_cast<umbf::Block>(mat));

    create_materials(exporter.materials);

    acul::string texture;
    create_default_texture(texture, env.data_dir);
    auto resource = target_texture(texture);
    exporter.textures.push_back(resource);

    auto state = exporter.save();
    assert(state.success());
    const auto copied = acul::path(env.output_dir) / "tex" / "export_origin_texture_0.jpg";
    acul::vector<char> original_bytes, copied_bytes;
    assert(acul::fs::read_binary(texture, original_bytes));
    assert(acul::fs::read_binary(copied, copied_bytes));
    assert(original_bytes == copied_bytes);
    exporter.textures[0] = target_texture("assets://unresolved/origin.png");
    assert(!exporter.save().success());
    assert(exporter.error().find("Unresolved texture target") != acul::string::npos);
    exporter.textures[0] = resource;
    exporter.textures[0].blocks.push_back(target_texture(texture).blocks.front());
    assert(!exporter.save().success());
    assert(exporter.error().find("Multiple texture targets") != acul::string::npos);
    exporter.textures.clear();
    assert(!exporter.save().success());
    assert(exporter.error().find("Missing texture") != acul::string::npos);
    exporter.textures.resize(1u);
    assert(!exporter.save().success());
    assert(exporter.error().find("Missing texture") != acul::string::npos);
    exporter.material_flags = MaterialExportFlags::texture_none;
    assert(exporter.save().success());
    exporter.clear();
}
