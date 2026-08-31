#include <acul/io/fs/file.hpp>
#include <acul/io/fs/path.hpp>
#include <acul/string/string.hpp>
#include <aecl/image/export.hpp>
#include <aecl/scene/obj/export.hpp>
#include <aecl/status.hpp>
#include <fstream>
#include <inttypes.h>
#include <oneapi/tbb/parallel_for.h>
#include <umbf/ext/material/utils.hpp>
#include "umbf/ext/scene/scene.hpp"

namespace aecl::scene::obj
{
    void transform_vertex(amal::vec3 &pos, MeshExportFlags flags)
    {
        if (flags & MeshExportFlagBits::transform_reverse_x) pos.x = -pos.x;
        if (flags & MeshExportFlagBits::transform_reverse_y) pos.y = -pos.y;
        if (flags & MeshExportFlagBits::transform_reverse_z) pos.z = -pos.z;
        if (flags & MeshExportFlagBits::transform_swap_xy) std::swap(pos.x, pos.y);
        if (flags & MeshExportFlagBits::transform_swap_xz) std::swap(pos.x, pos.z);
        if (flags & MeshExportFlagBits::transform_swap_yz) std::swap(pos.y, pos.z);
    }

    void Exporter::write_vertices(umbf::mesh::Geometry &geometry, const acul::vector<umbf::mesh::VertexGroup> &groups,
                                  acul::stringstream &ss)
    {
        // v
        for (auto &group : groups)
        {
            auto &vertex_id = group.vertices.front();
            amal::vec3 &pos = geometry.vertices[vertex_id].pos;
            transform_vertex(pos, mesh_flags);
            ss << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
        }

        // vt and vn
        for (auto &vertex : geometry.vertices)
        {
            if (mesh_flags & MeshExportFlagBits::export_uv)
            {
                auto [it, inserted] = _vt_map.emplace(vertex.uv, _vt_map.size());
                if (inserted) ss << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
            }
            if (mesh_flags & MeshExportFlagBits::export_normals)
            {
                auto &normal = vertex.normal;
                transform_vertex(normal, mesh_flags);
                auto [it, inserted] = _vn_map.emplace(normal, _vn_map.size());
                if (inserted) ss << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
            }
        }

        if ((mesh_flags & MeshExportFlagBits::transform_reverse_x) ||
            (mesh_flags & MeshExportFlagBits::transform_reverse_y) ||
            (mesh_flags & MeshExportFlagBits::transform_reverse_z))
            for (auto &face : geometry.faces) std::reverse(face.vertices.begin(), face.vertices.end());
    }

    void Exporter::write_triangles(umbf::mesh::Mesh *meta, acul::stringstream &os, const acul::vector<u32> &faces,
                                   const acul::vector<umbf::mesh::VertexGroup> &groups)
    {
        const auto &m = meta->geometry;
        acul::vector<u32> positions(m.vertices.size());
        for (size_t g = 0; g < groups.size(); g++)
            for (auto id : groups[g].vertices) positions[id] = g;

        size_t thread_count = oneapi::tbb::this_task_arena::max_concurrency();
        acul::vector<acul::stringstream> blocks(thread_count);
        oneapi::tbb::parallel_for(
            oneapi::tbb::blocked_range<size_t>(0, faces.size()), [&](const tbb::blocked_range<size_t> &range) {
                size_t thread_id = oneapi::tbb::this_task_arena::current_thread_index();
                for (size_t r = range.begin(); r != range.end(); ++r)
                {
                    auto &face = m.faces[faces[r]];
                    for (u32 iter = 0, current_id = face.first_vertex; iter < face.count / 3; ++iter)
                    {
                        blocks[thread_id] << "f ";
                        for (size_t vertex_id = 0; vertex_id < 3; ++vertex_id)
                        {
                            auto id = m.indices[current_id + vertex_id];
                            blocks[thread_id] << positions[id] + 1 << "/";
                            if (mesh_flags & MeshExportFlagBits::export_uv)
                                blocks[thread_id] << _vt_map[m.vertices[id].uv] + 1;
                            if (mesh_flags & MeshExportFlagBits::export_normals)
                                blocks[thread_id] << "/" << _vn_map[m.vertices[id].normal] + 1;
                            blocks[thread_id] << " ";
                        }
                        blocks[thread_id] << "\n";
                        current_id += 3;
                    }
                }
            });
        for (const auto &block : blocks) os << block.str();
    }

    void Exporter::write_faces(umbf::mesh::Mesh *meta, acul::stringstream &os, const acul::vector<u32> &faces)
    {
        size_t thread_count = oneapi::tbb::this_task_arena::max_concurrency();
        acul::vector<acul::stringstream> blocks(thread_count);
        auto &origin_faces = meta->geometry.faces;
        oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faces.size()),
                                  [&](const tbb::blocked_range<size_t> &range) {
                                      size_t thread_id = oneapi::tbb::this_task_arena::current_thread_index();
                                      for (size_t i = range.begin(); i != range.end(); ++i)
                                      {
                                          blocks[thread_id] << "f ";
                                          for (auto &ref : origin_faces[faces[i]].vertices)
                                          {
                                              blocks[thread_id] << ref.group + 1 << "/";
                                              auto &vertex = meta->geometry.vertices[ref.vertex];
                                              if (mesh_flags & MeshExportFlagBits::export_uv)
                                                  blocks[thread_id] << _vt_map[vertex.uv] + 1;
                                              if (mesh_flags & MeshExportFlagBits::export_normals)
                                                  blocks[thread_id] << "/" << _vn_map[vertex.normal] + 1;
                                              blocks[thread_id] << " ";
                                          }
                                          blocks[thread_id] << "\n";
                                      }
                                  });

        for (const auto &block : blocks) os << block.str();
    }

    inline void write_vec3_as_rgb(acul::stringstream &os, const acul::string &token, const amal::vec3 &vec)
    {
        os << token << " " << vec.x << " " << vec.y << " " << vec.z << "\n";
    }

    /**
     * Writes a number to the output stream with a given token.
     *
     * @param os The output stream to write to.
     * @param token The token to write before the number.
     * @param value The number to write.
     */
    template <typename T>
    inline void write_number(acul::stringstream &os, const acul::string &token, const T &value)
    {
        os << token << " " << value << "\n";
    }

    bool Exporter::write_texture(acul::stringstream &os, const acul::string &token, u64 texture_id)
    {
        if (material_flags == MaterialExportFlags::none || material_flags == MaterialExportFlags::texture_none)
            return true;
        if (texture_id >= textures.size() || textures[texture_id].blocks.empty())
        {
            _error = acul::format("Missing texture resource #%" PRIu64, texture_id);
            return false;
        }
        acul::shared_ptr<umbf::Image2D> image;
        acul::shared_ptr<umbf::Target> target;
        for (const auto &block : textures[texture_id].blocks)
        {
            if (!block) continue;
            if (block->signature() == umbf::sign_block::image)
            {
                if (image)
                {
                    _error = "Multiple image blocks: prepare a single image for OBJ export";
                    return false;
                }
                image = acul::static_pointer_cast<umbf::Image2D>(block);
            }
            else if (block->signature() == umbf::sign_block::target)
            {
                if (target)
                {
                    _error = "Multiple texture targets: prepare a single target for OBJ export";
                    return false;
                }
                target = acul::static_pointer_cast<umbf::Target>(block);
            }
        }
        const auto output_dir = acul::path(path).parent_path();
        const auto texture_dir = output_dir / "tex";
        const auto ensure_texture_dir = [&]() {
            if (acul::fs::is_directory(texture_dir.str().c_str())) return true;
            if (acul::fs::create_directory(texture_dir.str().c_str()).success()) return true;
            _error = "Failed to create texture directory: " + texture_dir.str();
            return false;
        };
        const acul::string basename = acul::format("%s_texture_%" PRIu64, acul::path(path).stem().c_str(), texture_id);
        if (image)
        {
            if (!image->pixels || !image->width || !image->height || image->channels.empty())
            {
                _error = "Texture has no prepared pixel data";
                return false;
            }
            if (!ensure_texture_dir()) return false;
            acul::string filename;
            bool saved = false;
            if (image->format.type == umbf::ImageFormat::Type::uint &&
                (image->format.bytes_per_channel == 1u || image->format.bytes_per_channel == 2u))
            {
                filename = basename + ".png";
                aecl::image::png::Params params(*image);
                saved = aecl::image::png::save(texture_dir / filename, params, image->format.bytes_per_channel);
                if (!saved) _error = params.error;
            }
            else if (image->format.type == umbf::ImageFormat::Type::sfloat &&
                     (image->format.bytes_per_channel == 2u || image->format.bytes_per_channel == 4u))
            {
                filename = basename + ".exr";
                acul::vector<umbf::Image2D> layers{*image};
                aecl::image::openexr::Params params(layers, "zip");
                saved = aecl::image::openexr::save(texture_dir / filename, params, image->format.bytes_per_channel);
                if (!saved) _error = params.error;
            }
            else _error = "Unsupported prepared texture format for OBJ export";
            if (!saved)
            {
                if (_error.empty()) _error = "Failed to export prepared texture";
                return false;
            }
            os << token << " ./tex/" << filename << "\n";
            return true;
        }
        if (!target || target->url.empty())
        {
            _error = "Texture requires prepared pixels or a target";
            return false;
        }
        const acul::path source(target->url);
        if (source.scheme() != "file")
        {
            _error = "Unresolved texture target: " + target->url;
            return false;
        }
        const auto type = aecl::image::get_type_by_extension(source.extension());
        if (type == aecl::image::Type::umbf || type == aecl::image::Type::unknown)
        {
            _error = "Prepare image data before exporting texture target to OBJ: " + target->url;
            return false;
        }
        if (material_flags == MaterialExportFlags::texture_origin)
        {
            os << token << " " << source.str() << "\n";
            return true;
        }
        if (!ensure_texture_dir()) return false;
        const auto filename = basename + source.extension();
        const auto destination = texture_dir / filename;
        if (source != destination &&
            !acul::fs::copy_file(source.str().c_str(), destination.str().c_str(), true).success())
        {
            _error = "Failed to copy texture: " + source.str();
            return false;
        }
        os << token << " ./tex/" << filename << "\n";
        return true;
    }

    void write_default_material(std::ofstream &os, bool use_pbr)
    {
        acul::stringstream mat_block;
        mat_block << "newmtl default\n";
        write_vec3_as_rgb(mat_block, "Ka", {1, 1, 1});
        write_vec3_as_rgb(mat_block, "Kd", {1, 1, 1});
        write_vec3_as_rgb(mat_block, "Ks", {1, 1, 1});
        write_number(mat_block, "Ns", 80);
        if (use_pbr)
        {
            write_number(mat_block, "Pr", 0.33);
            write_number(mat_block, "Pm", 1);
        }
        write_number(mat_block, "illum", 7);
        os << "\n" << mat_block.str().c_str();
    }

    bool Exporter::write_material(const acul::shared_ptr<umbf::MaterialBinding> &material_info,
                                  const acul::shared_ptr<umbf::Material> &material, std::ostream &os)
    {
        acul::stringstream mat_block;
        mat_block << "newmtl " << material_info->name << "\n";
        write_vec3_as_rgb(mat_block, "Ka", {1, 1, 1});
        if (!material)
        {
            _error = "Material target must be resolved before OBJ export";
            return false;
        }
        write_vec3_as_rgb(mat_block, "Kd", material->albedo.rgb);
        if (material->albedo.textured)
        {
            if (!write_texture(mat_block, "map_Kd", material->albedo.texture_id)) return false;
        }
        write_vec3_as_rgb(mat_block, "Ks", {1, 1, 1});
        write_number(mat_block, "Ns", 80);
        if (obj_flags & ObjExportFlagBits::materials_pbr)
        {
            write_number(mat_block, "Pr", 0.33);
            write_number(mat_block, "Pm", 1);
        }
        write_number(mat_block, "illum", 7);
        os << "\n" << mat_block.str().c_str();
        return true;
    }

    bool Exporter::write_mtllib_info(std::ofstream &mtl_stream, acul::stringstream &obj_stream)
    {
        if (material_flags != MaterialExportFlags::none)
        {
            acul::string mtl_path = acul::fs::replace_extension(path, ".mtl");
            mtl_stream.open(mtl_path.c_str());
            if (!mtl_stream.is_open())
            {
                _error = acul::format("Failed to write mtl file. Error: %s", std::strerror(errno));
                return false;
            }
            else
            {
                obj_stream << "mtllib ./" << acul::fs::get_filename(mtl_path) << "\n";
                mtl_stream << "# App3D ECL MTL Exporter\n";
            }
        }

        for (auto &material : Exporter::materials)
        {
            if (material.blocks.empty()) continue;
            acul::shared_ptr<umbf::Material> ptr;
            for (const auto &block : material.blocks)
            {
                if (!block) continue;
                switch (block->signature())
                {
                    case umbf::sign_block::material:
                        ptr = acul::static_pointer_cast<umbf::Material>(block);
                        break;
                    case umbf::sign_block::material_info:
                    {
                        auto info = acul::static_pointer_cast<umbf::MaterialBinding>(block);
                        _material_map[info->id] = {info, ptr};
                    }
                    break;
                    default:
                        break;
                }
            }
        }
        return true;
    }

    u32 Exporter::write_object(const aecl::Asset &object, acul::stringstream &stream)
    {
        acul::shared_ptr<umbf::ObjectInfo> descriptor;
        for (const auto &block : object.blocks)
        {
            if (block && block->signature() == umbf::sign_block::object_info)
            {
                descriptor = acul::static_pointer_cast<umbf::ObjectInfo>(block);
                break;
            }
        }
        if (!descriptor)
        {
            _error = "Scene object descriptor block not found";
            return AECL_OP_CODE_MESH_ERROR;
        }
        acul::shared_ptr<umbf::mesh::Mesh> mesh;
        acul::vector<acul::shared_ptr<umbf::MaterialRange>> assignes;
        for (const auto &block : object.blocks)
        {
            if (!block) continue;
            switch (block->signature())
            {
                case umbf::sign_block::mesh:
                    mesh = acul::static_pointer_cast<umbf::mesh::Mesh>(block);
                    break;
                case umbf::sign_block::material_range:
                    assignes.push_back(acul::static_pointer_cast<umbf::MaterialRange>(block));
                    break;
            }
        }
        if (!mesh)
        {
            _error = acul::format("Mesh block not found in object: 0x%" PRIx64, descriptor->id);
            return AECL_OP_CODE_MESH_ERROR;
        }
        acul::vector<umbf::mesh::VertexGroup> vertex_groups;
        umbf::mesh::fill_vertex_groups(mesh->geometry, vertex_groups);
        if (obj_flags & ObjExportFlagBits::object_policy_groups) stream << "g " << descriptor->name << "\n";
        else if (obj_flags & ObjExportFlagBits::object_policy_objects) stream << "o " << descriptor->name << "\n";
        auto &geometry = mesh->geometry;
        write_vertices(geometry, vertex_groups, stream);
        acul::vector<acul::shared_ptr<umbf::MaterialRange>> assignes_attr;
        auto default_mat_id_it =
            std::find_if(assignes.begin(), assignes.end(),
                         [](const acul::shared_ptr<umbf::MaterialRange> &range) { return range->faces.empty(); });
        u64 default_mat_id = default_mat_id_it == assignes.end() ? 0 : (*default_mat_id_it)->mat_id;
        umbf::filter_material_assignments(assignes, geometry.faces.size(), default_mat_id, assignes_attr);
        u32 op_code = 0;
        for (auto &assign : assignes_attr)
        {
            if (assign->faces.empty()) continue;
            if (material_flags != MaterialExportFlags::none)
            {
                if (assign->mat_id == 0)
                {
                    stream << "usemtl default\n";
                    _all_materials_exist = false;
                }
                else
                {
                    auto it = _material_map.find(assign->mat_id);
                    if (it == _material_map.end())
                    {
                        _error = acul::format("Material not found: 0x%" PRIx64, assign->mat_id);
                        op_code |= AECL_OP_CODE_MATERIAL_ERROR;
                    }
                    else stream << "usemtl " << it->second.info->name << "\n";
                }
            }
            if (mesh_flags & MeshExportFlagBits::export_triangulated)
                write_triangles(mesh.get(), stream, assign->faces, vertex_groups);
            else write_faces(mesh.get(), stream, assign->faces);
        }
        return op_code;
    }

    bool Exporter::write_mtl(std::ofstream &stream)
    {
        if (material_flags == MaterialExportFlags::none) return true;
        if (!stream) return false;
        if (!_all_materials_exist) write_default_material(stream, obj_flags & ObjExportFlagBits::materials_pbr);
        for (auto it = _material_map.begin(); it != _material_map.end(); it++)
        {
            auto &ref = it->second;
            if (!write_material(ref.info, ref.mat, stream)) return false;
        }
        stream.close();
        return !stream.fail();
    }

    acul::op_result Exporter::save()
    {
        _error.clear();
        _material_map.clear();
        _vt_map.clear();
        _vn_map.clear();
        _all_materials_exist = true;
        acul::stringstream ss;
        ss << "# App3D ECL OBJ Exporter\n";
        std::ofstream mtl_stream;
        u32 op_code = write_mtllib_info(mtl_stream, ss) ? 0 : AECL_OP_CODE_MATERIAL_ERROR;
        for (auto &object : objects)
            if (!object.blocks.empty()) op_code |= write_object(object, ss);

        auto wr = acul::fs::write_by_block(path, ss.str().c_str(), 1024 * 1024);
        if (!wr.success()) return wr;
        if (!write_mtl(mtl_stream))
            return acul::op_result(ACUL_OP_WRITE_ERROR, AECL_OP_DOMAIN, AECL_OP_CODE_MATERIAL_ERROR);
        return acul::op_result(ACUL_OP_SUCCESS, AECL_OP_DOMAIN, op_code);
    }
} // namespace aecl::scene::obj
