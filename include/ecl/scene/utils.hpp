#pragma once

#include <assets/asset.hpp>

namespace ecl
{
    namespace utils
    {
        using Vertex2D = SArray<f32, 2>;

        // Determines whether a given polygon is oriented in counter-clockwise (CCW) direction.
        bool isPolygonCCW(const DArray<Vertex2D> &polygon);

        // Projects a 3D point to a 2D point
        DArray<Vertex2D> projectToVertex2D(const assets::mesh::Face &face,
                                           const DArray<assets::mesh::Vertex> &vertices,
                                           DArray<u32> &vertexIndices);

        DArray<u32> triangulate(const assets::mesh::Face &face,
                                const DArray<assets::mesh::Vertex> &vertices);

        inline glm::vec3 averageVertexNormal(const assets::mesh::Face &face,
                                             const DArray<assets::mesh::Vertex> &vertices)
        {
            glm::vec3 normal{0.0f, 0.0f, 0.0f};
            for (const auto &vertexRef : face.vertices) normal += vertices[vertexRef.vertex].normal;
            return glm::normalize(normal);
        }

        /**
         * Builds the barycentric coordinates for the given vertices and indices.
         *
         * @param barycentric reference to the array to store the barycentric vertices
         * @param face the face for which barycentric coordinates are being constructed
         * @param vertices the array of vertices
         * @param indices the array of indices
         *
         * @throws None
         */
        void buildBarycentric(DArray<assets::mesh::bary::Vertex> &barycentric,
                              const assets::mesh::Face &face, const DArray<assets::mesh::Vertex> &vertices,
                              const DArray<u32> &indices);
    } // namespace utils
} // namespace ecl