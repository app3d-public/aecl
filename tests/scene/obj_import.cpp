#include <aecl/scene/obj/import.hpp>
#include <umbf/ext/material/material.hpp>
#include <umbf/ext/scene/scene.hpp>
#include "../env.hpp"

void test_obj_import()
{
    test_environment env;
    create_test_environment(env);
    aecl::scene::obj::Importer importer(acul::path(env.data_dir) / "cube.obj");
    assert(!importer.path().empty());
    auto state = importer.load();
    assert(state.success());

    assert(importer.objects().size() == 1u);
    assert(importer.images().size() == 0u);
    assert(importer.materials().size() == 1u);

    const auto &object = importer.objects().at(0u);
    assert(!importer.objects().at(0u).blocks.empty());
    assert(object.header.type_sign == umbf::sign_block::format::none);
    assert(object.blocks.size() == 2u);
    bool has_object = false;
    bool has_mesh = false;
    const umbf::mesh::Mesh *mesh = nullptr;
    for (const auto &block : object.blocks)
    {
        assert(block);
        has_object |= block->signature() == umbf::sign_block::object_info;
        has_mesh |= block->signature() == umbf::sign_block::mesh;
        if (block->signature() == umbf::sign_block::mesh) mesh = static_cast<const umbf::mesh::Mesh *>(block.get());
    }
    assert(has_object && has_mesh);
    // All six quads must produce two triangles. Front and back have normals
    // parallel to Z and guard the projection-basis regression.
    assert(mesh && mesh->geometry.indices.size() == 36u);

    auto &material_rc = importer.materials().at(0u);
    assert(material_rc.header.type_sign == umbf::sign_block::format::material);
    assert(material_rc.blocks.size() == 2u);
    bool has_material = false;
    bool has_binding = false;
    for (const auto &block : material_rc.blocks)
    {
        assert(block);
        has_material |= block->signature() == umbf::sign_block::material;
        has_binding |= block->signature() == umbf::sign_block::material_info;
    }
    assert(has_material && has_binding);

    assert(importer.images().empty());

    // Imported resources have one owner and can be transferred without cloning their blocks.
    auto retained = std::move(importer.objects().at(0u));
    assert(!retained.blocks.empty());

    importer.clear();
    assert(importer.objects().size() == 0u);
    assert(importer.images().size() == 0u);
    assert(importer.materials().size() == 0u);
    assert(retained.header.type_sign == umbf::sign_block::format::none);
    assert(retained.blocks.size() == 2u);
    for (const auto &block : retained.blocks) assert(block && block->signature());
}
