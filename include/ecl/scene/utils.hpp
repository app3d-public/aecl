#pragma once

#include <umbf/umbf.hpp>

namespace ecl
{
    namespace utils
    {
        using Vertex2D = std::array<f32, 2>;

        // Determines whether a given polygon is oriented in counter-clockwise (CCW) direction.
        bool isPolygonCCW(const acul::vector<Vertex2D> &polygon);

        // Projects a 3D point to a 2D point
        acul::vector<Vertex2D> projectToVertex2D(const umbf::mesh::Face &face,
                                                 const acul::vector<umbf::mesh::Vertex> &vertices,
                                                 acul::vector<u32> &vertexIndices);

        acul::vector<u32> triangulate(const umbf::mesh::Face &face, const acul::vector<umbf::mesh::Vertex> &vertices);

        inline glm::vec3 averageVertexNormal(const umbf::mesh::Face &face,
                                             const acul::vector<umbf::mesh::Vertex> &vertices)
        {
            glm::vec3 normal{0.0f, 0.0f, 0.0f};
            for (const auto &vertexRef : face.vertices) normal += vertices[vertexRef.vertex].normal;
            return glm::normalize(normal);
        }
    } // namespace utils
} // namespace ecl