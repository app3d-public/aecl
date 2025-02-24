#pragma once

#include <assets/asset.hpp>
#include <ecl/scene/export.hpp>

namespace tests
{
    void createCubeVerticles(astl::vector<assets::mesh::Vertex> &vertices);

    void createCubeVGroups(astl::vector<assets::mesh::VertexGroup> &vertexGroups);

    void createCubeFaces(astl::vector<assets::mesh::IndexedFace> &faces);

    void createObjects(astl::vector<assets::Object> &objects);

    void createMaterials(astl::vector<assets::Asset> &materials);

    inline void createDefaultTexture(assets::Target::Addr &tex, const std::filesystem::path &dataDir)
    {
        tex.url = (dataDir / "devCheck.jpg").string();
    }

    void createGeneratedTexture(assets::Target::Addr &tex, const std::filesystem::path &texDir);
} // namespace tests