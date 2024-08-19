#include "common.hpp"
#include <filesystem>
#include <oneapi/tbb/scalable_allocator.h>

namespace tests
{
    void createCubeVerticles(DArray<assets::meta::mesh::Vertex> &vertices)
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

    void createCubeVGroups(DArray<assets::meta::mesh::VertexGroup> &vertexGroups)
    {
        vertexGroups.resize(8);
        vertexGroups[0] = {{3, 12, 22}, {0, 3, 5}};
        vertexGroups[1] = {{2, 13, 19}, {0, 3, 4}};
        vertexGroups[2] = {{0, 7, 21}, {0, 1, 5}};
        vertexGroups[3] = {{1, 6, 16}, {0, 1, 4}};
        vertexGroups[4] = {{4, 11, 20}, {1, 2, 5}};
        vertexGroups[5] = {{5, 10, 17}, {1, 2, 4}};
        vertexGroups[6] = {{8, 15, 23}, {2, 3, 5}};
        vertexGroups[7] = {{9, 14, 18}, {2, 3, 4}};
    }

    void createCubeFaces(DArray<assets::meta::mesh::Face> &faces)
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

    void createObjects(DArray<std::shared_ptr<assets::Object>> &objects)
    {
        objects.push_back(std::make_shared<assets::Object>());
        auto cube = objects.front();
        cube->name = "cube";
        auto meshBlock = std::make_shared<assets::meta::mesh::MeshBlock>();
        auto &model = meshBlock->model;
        createCubeVerticles(model.vertices);
        model.indices = {2,  3,  0,  0,  1,  2,  6,  7,  4,  4,  5,  6,  10, 11, 8,  8,  9,  10,
                         14, 15, 12, 12, 13, 14, 18, 19, 16, 16, 17, 18, 22, 23, 20, 20, 21, 22};
        createCubeVGroups(model.groups);
        createCubeFaces(model.faces);
        model.aabb = {{-100, -100, -100}, {100, 100, 100}};
        cube->meta.push_front(meshBlock);
    }

    void createMaterials(DArray<std::shared_ptr<assets::Material>> &materials)
    {
        auto mat = std::make_shared<assets::Material>();
        mat->info.albedo.textured = true;
        mat->info.albedo.textureID = 0;
        auto meta = std::make_shared<assets::meta::MaterialBlock>();
        meta->name = "ecl:test:mat_e";
        meta->assignments.push_back(0);
        mat->meta.push_front(meta);
        materials.push_back(mat);
    }

    void createDefaultTexture(ecl::scene::TextureNode &tex, const std::filesystem::path &dataDir)
    {
        tex.path = dataDir / "devCheck.jpg";
        tex.flags = assets::ImageTypeFlagBits::image2D | assets::ImageTypeFlagBits::external;
    }

    void createGeneratedTexture(ecl::scene::TextureNode &tex)
    {
        tex.flags = assets::ImageTypeFlagBits::image2D;
        tex.image.imageFormat = vk::Format::eR8G8B8A8Srgb;
        tex.image.bytesPerChannel = 1;
        tex.image.width = 4;
        tex.image.height = 4;
        tex.image.channelCount = 4;
        tex.image.channelNames = {"red", "green", "blue", "alpha"};
        char *pixels = (char *)scalable_malloc(64);
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
        tex.image.pixels = pixels;
    }
} // namespace tests