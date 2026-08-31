#include <aecl/image/import.hpp>
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
    exporter.objects.front().blocks.push_back(acul::static_pointer_cast<umbf::Block>(mat));

    create_materials(exporter.materials);

    acul::string texture;
    create_generated_texture(texture, path / "tex");
    exporter.textures.push_back(target_texture(texture));

    assert(exporter.save().success());

    // Embedded textures are exported directly from prepared pixels.
    unsigned char pixels[] = {17, 42, 230, 255, 100, 80, 0, 127};
    auto image = acul::make_shared<umbf::Image2D>();
    image->width = 2u;
    image->height = 1u;
    image->channels = {"R", "G", "B", "A"};
    image->format = {umbf::ImageFormat::Type::uint, 1u};
    image->pixels = pixels;
    aecl::Asset embedded;
    embedded.header.type_sign = umbf::sign_block::format::image;
    embedded.blocks.push_back(image);
    exporter.textures[0] = embedded;
    assert(exporter.save().success());
    auto *loader = aecl::image::get_importer_by_path(path / "tex" / "export_origin_texture_0.png");
    assert(loader);
    acul::vector<umbf::Image2D> restored;
    assert(loader->load(path / "tex" / "export_origin_texture_0.png", restored));
    assert(restored.size() == 1u && restored[0].size() == sizeof(pixels));
    assert(std::memcmp(restored[0].pixels, pixels, sizeof(pixels)) == 0);
    for (auto &layer : restored) acul::release(static_cast<char *>(layer.pixels));
    acul::release(loader);
}
