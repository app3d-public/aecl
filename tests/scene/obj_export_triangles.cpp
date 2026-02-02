#include <aecl/scene/obj/export.hpp>
#include "../env.hpp"
#include "common.hpp"

void test_obj_export_triangles()
{
    test_environment env;
    create_test_environment(env);
    using namespace aecl::scene;
    obj::Exporter exporter(acul::path(env.output_dir) / "export_origin.obj");
    exporter.material_flags = MaterialExportFlags::none;
    exporter.mesh_flags = MeshExportFlagBits::export_normals | MeshExportFlagBits::export_triangulated;
    exporter.obj_flags = obj::ObjExportFlagBits::object_policy_groups | obj::ObjExportFlagBits::materials_pbr;
    create_objects(exporter.objects);
    auto state = exporter.save();
    exporter.clear();
    assert(state.success());
}