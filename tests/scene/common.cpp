#include "common.hpp"
#include <aecl/image/export.hpp>
#include <umbf/version.h>

void create_cube_verticles(acul::vector<umbf::mesh::Vertex> &vertices)
{
    vertices.resize(24);
    // Front face
    vertices[0] = {{100, -100, 100}, {1, 0}, {0, 0, 1}};
    vertices[1] = {{100, 100, 100}, {1, -1}, {0, 0, 1}};
    vertices[2] = {{-100, 100, 100}, {0, -1}, {0, 0, 1}};
    vertices[3] = {{-100, -100, 100}, {0, 0}, {0, 0, 1}};
    // Right face
    vertices[4] = {{100, -100, -100}, {1, 0}, {1, 0, 0}};
    vertices[5] = {{100, 100, -100}, {1, -1}, {1, 0, 0}};
    vertices[6] = {{100, 100, 100}, {0, -1}, {1, 0, 0}};
    vertices[7] = {{100, -100, 100}, {0, 0}, {1, 0, 0}};
    // Back face
    vertices[8] = {{-100, -100, -100}, {1, 0}, {0, 0, -1}};
    vertices[9] = {{-100, 100, -100}, {1, -1}, {0, 0, -1}};
    vertices[10] = {{100, 100, -100}, {0, -1}, {0, 0, -1}};
    vertices[11] = {{100, -100, -100}, {0, 0}, {0, 0, -1}};
    // Left face
    vertices[12] = {{-100, -100, 100}, {1, 0}, {-1, 0, 0}};
    vertices[13] = {{-100, 100, 100}, {1, -1}, {-1, 0, 0}};
    vertices[14] = {{-100, 100, -100}, {0, -1}, {-1, 0, 0}};
    vertices[15] = {{-100, -100, -100}, {0, 0}, {-1, 0, 0}};
    // Top face
    vertices[16] = {{100, 100, 100}, {1, 0}, {0, 1, 0}};
    vertices[17] = {{100, 100, -100}, {1, -1}, {0, 1, 0}};
    vertices[18] = {{-100, 100, -100}, {0, -1}, {0, 1, 0}};
    vertices[19] = {{-100, 100, 100}, {0, 0}, {0, 1, 0}};
    // Bottom face
    vertices[20] = {{100, -100, -100}, {1, 0}, {0, -1, 0}};
    vertices[21] = {{100, -100, 100}, {1, -1}, {0, -1, 0}};
    vertices[22] = {{-100, -100, 100}, {0, -1}, {0, -1, 0}};
    vertices[23] = {{-100, -100, -100}, {0, 0}, {0, -1, 0}};
}

void create_cube_faces(acul::vector<umbf::mesh::Face> &faces)
{
    faces.resize(6);
    faces[0] = {{
                    {2, 0},
                    {3, 1},
                    {1, 2},
                    {0, 3},
                },
                {0, 0, 1},
                0,
                6};
    faces[1] = {{
                    {4, 4},
                    {5, 5},
                    {3, 6},
                    {2, 7},
                },
                {1, 0, 0},
                6,
                6};
    faces[2] = {{
                    {6, 8},
                    {7, 9},
                    {5, 10},
                    {4, 11},
                },
                {0, 0, -1},
                12,
                6};
    faces[3] = {{
                    {0, 12},
                    {1, 13},
                    {7, 14},
                    {6, 15},
                },
                {-1, 0, 0},
                18,
                6};
    faces[4] = {{
                    {3, 16},
                    {5, 17},
                    {7, 18},
                    {1, 19},
                },
                {0, 1, 0},
                24,
                6};
    faces[5] = {{
                    {4, 20},
                    {2, 21},
                    {0, 22},
                    {6, 23},
                },
                {0, -1, 0},
                30,
                6};
}

void create_objects(acul::vector<umbf::Object> &objects)
{
    objects.emplace_back();
    auto &cube = objects.front();
    cube.name = "cube";
    auto mesh = acul::make_shared<umbf::mesh::Mesh>();
    auto &model = mesh->model;
    create_cube_verticles(model.vertices);
    model.indices = {2,  3,  0,  0,  1,  2,  6,  7,  4,  4,  5,  6,  10, 11, 8,  8,  9,  10,
                     14, 15, 12, 12, 13, 14, 18, 19, 16, 16, 17, 18, 22, 23, 20, 20, 21, 22};
    create_cube_faces(model.faces);
    model.group_count = 8;
    model.aabb = {glm::vec3{-100, -100, -100}, {100, 100, 100}};
    cube.meta.push_back(mesh);
}

void create_materials(acul::vector<umbf::File> &materials)
{
    materials.emplace_back();
    auto mat = acul::make_shared<umbf::Material>();
    mat->albedo.textured = true;
    mat->albedo.texture_id = 0;
    auto meta = acul::make_shared<umbf::MaterialInfo>();
    meta->name = "ecl:test:mat_e";
    meta->assignments.push_back(0);
    materials.emplace_back();
    auto &asset = materials.back();
    asset.header.vendor_sign = UMBF_VENDOR_ID;
    asset.header.vendor_version = UMBF_VERSION;
    asset.header.spec_version = UMBF_VERSION;
    asset.header.type_sign = umbf::sign_block::format::Material;
    asset.blocks.push_back(mat);
    asset.blocks.push_back(meta);
    materials.push_back(asset);
}

void create_generated_texture(acul::string &tex, const acul::io::path &texDir)
{
    umbf::Image2D image;
    image.format = vk::Format::eR8G8B8A8Srgb;
    image.bytes_per_channel = 1;
    image.width = 4;
    image.height = 4;
    image.channel_count = 4;
    image.channel_names = {"red", "green", "blue", "alpha"};
    char *pixels = acul::alloc_n<char>(image.size());
    u8 color[4] = {255, 120, 80, 255};
    for (int w = 0; w < 4; w++)
    {
        for (int h = 0; h < 4; h++)
        {
            int index = w + h * 4;
            pixels[index * 4 + 0] = color[0];
            pixels[index * 4 + 1] = color[1];
            pixels[index * 4 + 2] = color[2];
            pixels[index * 4 + 3] = color[3];
        }
    }
    image.pixels = pixels;
    tex = texDir / "generated.png";
    aecl::image::png::Params pp(image);
    bool success = aecl::image::png::save(tex, pp, 1);
    acul::release(pixels);
    if (!success) throw acul::runtime_error("Failed to save image");
}