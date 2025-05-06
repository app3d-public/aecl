#include <ecl/scene/obj/export.hpp>
#include "../env.hpp"
#include "common.hpp"

void test_obj_export_triangles()
{
    test_environment env;
    create_test_environment(env);
    using namespace ecl::scene;
    obj::ObjExportFlags obj_flags = obj::ObjExportFlagBits::ObjectPolicyGroups | obj::ObjExportFlagBits::MaterialsPBR;
    obj::Exporter exporter(acul::io::path(env.output_dir) / "export_origin.obj");
    exporter.material_flags = MaterialExportFlags::None;
    exporter.mesh_flags = MeshExportFlagBits::ExportNormals | MeshExportFlagBits::ExportTriangulated;
    exporter.obj_flags = obj::ObjExportFlagBits::ObjectPolicyGroups | obj::ObjExportFlagBits::MaterialsPBR;
    create_objects(exporter.objects);
    auto state = exporter.save();
    exporter.clear();
    assert(state);
}