#pragma once

#include <umbf/umbf.hpp>

namespace aecl
{
    namespace utils
    {
        using Vertex2D = std::array<f32, 2>;

        acul::vector<u32> triangulate(const umbf::mesh::Face &face, const acul::vector<umbf::mesh::Vertex> &vertices);

        inline glm::vec3 average_vertex_normal(const umbf::mesh::Face &face,
                                               const acul::vector<umbf::mesh::Vertex> &vertices)
        {
            glm::vec3 normal{0.0f, 0.0f, 0.0f};
            for (const auto &ref : face.vertices) normal += vertices[ref.vertex].normal;
            return glm::normalize(normal);
        }
    } // namespace utils
} // namespace aecl