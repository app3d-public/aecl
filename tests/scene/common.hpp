#pragma once

#include <assets/asset.hpp>
#include <ecl/scene/export.hpp>

namespace tests
{
    void createCubeVerticles(DArray<assets::mesh::Vertex> &vertices);

    void createCubeVGroups(DArray<assets::mesh::VertexGroup> &vertexGroups);

    void createCubeFaces(DArray<assets::mesh::Face> &faces);

    void createObjects(DArray<std::shared_ptr<assets::Object>> &objects);

    void createMaterials(DArray<std::shared_ptr<assets::Asset>> &materials);

    inline void createDefaultTexture(assets::Target::Addr &tex, const std::filesystem::path &dataDir)
    {
        tex.url = (dataDir / "devCheck.jpg").string();
    }

    void createGeneratedTexture(assets::Target::Addr &tex, const std::filesystem::path& texDir);
} // namespace tests