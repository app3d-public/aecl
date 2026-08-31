#pragma once

#include <aecl/scene/export.hpp>
#include <umbf/ext/image/image.hpp>
#include <umbf/ext/material/material.hpp>

void create_cube_verticles(acul::vector<umbf::mesh::Vertex> &vertices);
void create_cube_faces(acul::vector<umbf::mesh::Face> &faces);
void create_objects(acul::vector<aecl::Asset> &objects);
void create_materials(acul::vector<aecl::Asset> &materials);

inline void create_default_texture(acul::string &tex, const acul::path &data_dir)
{
    tex = (data_dir / "devCheck.jpg").str();
}

void create_generated_texture(acul::string &tex, const acul::path &tex_folder);

inline aecl::Asset target_texture(const acul::string &url)
{
    aecl::Asset resource;
    resource.header.type_sign = umbf::sign_block::format::target;
    auto target = acul::make_shared<umbf::Target>();
    target->header.type_sign = umbf::sign_block::format::image;
    target->url = url;
    resource.blocks.push_back(target);
    return resource;
}
