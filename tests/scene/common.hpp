#pragma once

#include <assets/scene.hpp>
#include <ecl/scene/export.hpp>

namespace tests
{
    void createCubeVerticles(DArray<assets::meta::mesh::Vertex> &vertices);

    void createCubeVGroups(DArray<assets::meta::mesh::VertexGroup> &vertexGroups);

    void createCubeFaces(DArray<assets::meta::mesh::Face> &faces);

    void createMeshes(DArray<ecl::scene::MeshNode> &meshes, int matID = -1);

    void createMaterials(DArray<ecl::scene::MaterialNode> &materials);

    void createDefaultTexture(ecl::scene::TextureNode& tex, const std::filesystem::path& dataDir);

    void createGeneratedTexture(ecl::scene::TextureNode &tex);
} // namespace tests