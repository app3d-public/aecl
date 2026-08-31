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
    assert(object.header.type_sign == umbf::sign_block::format::scene_object);
    assert(object.blocks.size() == 2u);
    bool has_object = false;
    bool has_mesh = false;
    for (const auto &block : object.blocks)
    {
        assert(block);
        has_object |= block->signature() == umbf::sign_block::object_info;
        has_mesh |= block->signature() == umbf::sign_block::mesh;
    }
    assert(has_object && has_mesh);

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

    // Resource copies own their block list but share the actual blocks.
    auto retained = object;
    assert(retained.blocks.front().get() == object.blocks.front().get());
    auto independent = retained;
    independent.blocks.clear();
    assert(!retained.blocks.empty());

    importer.clear();
    assert(importer.objects().size() == 0u);
    assert(importer.images().size() == 0u);
    assert(importer.materials().size() == 0u);
    assert(retained.header.type_sign == umbf::sign_block::format::scene_object);
    assert(retained.blocks.size() == 2u);
    for (const auto &block : retained.blocks) assert(block && block->signature());
}
