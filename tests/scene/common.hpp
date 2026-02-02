#pragma once

#include <aecl/scene/export.hpp>
#include <umbf/umbf.hpp>

void create_cube_verticles(acul::vector<umbf::mesh::Vertex> &vertices);

void create_cube_faces(acul::vector<umbf::mesh::Face> &faces);

void create_objects(acul::vector<umbf::Object> &objects);

void create_materials(acul::vector<umbf::File> &materials);

inline void create_default_texture(acul::string &tex, const acul::path &data_dir)
{
    tex = (data_dir / "devCheck.jpg").str();
}

void create_generated_texture(acul::string &tex, const acul::path &tex_folder);