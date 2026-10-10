#pragma once

#include <amal/geometric.hpp>
#include <amal/vector.hpp>
#include <umbf/ext/scene/scene.hpp>

namespace aecl::utils
{
    using Vertex2D = std::array<f32, 2>;
    acul::vector<u32> triangulate(const umbf::mesh::Face &face, const acul::vector<umbf::mesh::Vertex> &vertices);
} // namespace aecl::utils
