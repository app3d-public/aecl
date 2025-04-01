#pragma once

#include <ecl/scene/export.hpp>
#include <umbf/umbf.hpp>

namespace tests
{
    void createCubeVerticles(acul::vector<umbf::mesh::Vertex> &vertices);

    void createCubeVGroups(acul::vector<umbf::mesh::VertexGroup> &vertexGroups);

    void createCubeFaces(acul::vector<umbf::mesh::IndexedFace> &faces);

    void createObjects(acul::vector<umbf::Object> &objects);

    void createMaterials(acul::vector<umbf::File> &materials);

    inline void createDefaultTexture(acul::string &tex, const acul::io::path &dataDir)
    {
        tex = (dataDir / "devCheck.jpg").str();
    }

    void createGeneratedTexture(acul::string &tex, const acul::io::path &texDir);
} // namespace tests