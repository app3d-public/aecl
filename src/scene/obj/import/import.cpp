#include <acul/hash/hl_hashmap.hpp>
#include <aecl/scene/obj/import.hpp>
#include <aecl/scene/utils.hpp>
#include <oneapi/tbb/parallel_sort.h>
#include <sstream>
#include <umbf/version.h>
#include "geom.cpp_"
#include "mat.cpp_"

namespace aecl
{
    namespace scene
    {
        namespace obj
        {
            inline int get_group_range_end(int start_index, int range_end, ParseDataRead &data)
            {
                for (size_t i = start_index; i < data.f.size(); ++i)
                    if (data.f[i].index >= range_end) return i;
                return data.f.size();
            }

            using namespace umbf::mesh;

            struct GroupRange
            {
                int start_index;
                int range_end;
                acul::string name;
                acul::shared_ptr<Mesh> mesh;
            };

            amal::vec3 calculate_normal(const ParseDataRead &data, const acul::vector<amal::ivec3> &__restrict in_face)
            {
                amal::vec3 normal{0.0f};
                for (size_t v = 0; v < in_face.size(); ++v)
                {
                    amal::vec3 current = data.v[in_face[v].x - 1].value;
                    amal::vec3 next = data.v[in_face[(v + 1) % in_face.size()].x - 1].value;
                    normal.x += (current.y - next.y) * (current.z + next.z);
                    normal.y += (current.z - next.z) * (current.x + next.x);
                    normal.z += (current.x - next.x) * (current.y + next.y);
                }
                return amal::normalize(normal);
            }

            void add_vertex_to_face(const ParseDataRead &data, u32 vertex_group_id, u32 current,
                                    acul::hl_hashmap<amal::ivec3, u32> &vtn_map, const amal::ivec3 &vtn, Model &m,
                                    Face &face)
            {
                auto [it, inserted] = vtn_map.emplace(vtn, vtn_map.size());
                if (inserted)
                {
                    face.vertices.emplace_back(vertex_group_id, static_cast<u32>(m.vertices.size()));
                    m.vertices.emplace_back(data.v[current].value);
                    auto &vertex = m.vertices.back();
                    if (vtn.y != 0 && (int)data.vt.size() > vtn.y) vertex.uv = data.vt[vtn.y - 1].value;
                    vertex.normal = vtn.z != 0 && (int)data.vn.size() > vtn.z ? data.vn[vtn.z - 1].value : face.normal;
                    m.aabb.min = amal::min(m.aabb.min, vertex.pos);
                    m.aabb.max = amal::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(vertex_group_id, it->second);
            }

            void add_vertex_to_face(const ParseDataRead &data, u32 vertex_group_id, u32 current,
                                    acul::vector<VertexGroup> &groups, const amal::ivec3 &vtn, Model &m, Face &face)
            {
                Vertex vertex{data.v[current].value};
                if (vtn.y != 0 && (int)data.vt.size() > vtn.y) vertex.uv = data.vt[vtn.y - 1].value;
                if (vtn.z != 0 && (int)data.vn.size() > vtn.z)
                    vertex.normal = data.vn[vtn.z - 1].value;
                else
                    vertex.normal = face.normal;
                auto &vgv = groups[vertex_group_id].vertices;
                auto it = std::find_if(vgv.begin(), vgv.end(),
                                       [&m, &vertex](u32 index) { return m.vertices[index] == vertex; });
                if (it == vgv.end())
                {
                    vgv.emplace_back(m.vertices.size());
                    face.vertices.emplace_back(vertex_group_id, static_cast<u32>(m.vertices.size()));
                    m.vertices.emplace_back(vertex);
                    m.aabb.min = amal::min(m.aabb.min, vertex.pos);
                    m.aabb.max = amal::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(vertex_group_id, *it);
            }

            void index_mesh(size_t face_count, const ParseDataRead &data, GroupRange &group)
            {
                acul::hl_hashmap<amal::ivec3, u32> vtn_map;
                acul::vector<VertexGroup> vertex_groups;
                const bool use_normals = !data.vn.empty();
                if (use_normals)
                    vtn_map.reserve(data.v.size());
                else
                    vertex_groups.resize(data.v.size());
                auto &m = group.mesh->model;
                m.faces.resize(face_count);
                acul::vector<int> pos_map(data.v.size(), -1);
                for (size_t f = 0; f < face_count; ++f)
                {
                    auto &in_face = data.f[group.start_index + f].value;
                    auto &face = m.faces[f];
                    face.normal = calculate_normal(data, *in_face);
                    for (size_t v = 0; v < in_face->size(); ++v)
                    {
                        auto &vtn = (*in_face)[v];
                        const int current = vtn.x - 1;
                        if ((int)data.v.size() < vtn.x) continue;
                        if (pos_map[current] == -1) pos_map[current] = m.group_count++;
                        if (use_normals)
                            add_vertex_to_face(data, pos_map[current], current, vtn_map, vtn, m, face);
                        else
                            add_vertex_to_face(data, pos_map[current], current, vertex_groups, vtn, m, face);
                    }
                }
            }

            void parse_mtl(const acul::string &filename, acul::vector<Material> &materials)
            {
                std::ifstream fs(filename.c_str());
                if (!fs.is_open())
                {
                    LOG_ERROR("Could not open file: %s", filename.c_str());
                    return;
                }

                LOG_INFO("Loading MTL file: %s", filename.c_str());
                std::ostringstream oss;
                oss << fs.rdbuf();
                std::string content = oss.str();
                fs.close();
                acul::string_view_pool<char> string_view_pool(content.size());
                acul::io::file::fill_line_buffer(content.c_str(), content.size(), string_view_pool);

                // Process each line from the buffer
                int material_index = -1;
                int line_index = 1;
                for (const auto &line : string_view_pool) parse_mtl_line(line, materials, material_index, line_index++);
                LOG_INFO("Loaded %zu materials", materials.size());
            }

            void convert_to_materials(const acul::string &base_path, const acul::vector<Material> &mtl_list,
                                      acul::hl_hashmap<acul::string, int> &mat_map,
                                      acul::vector<acul::shared_ptr<umbf::File>> &materials,
                                      acul::vector<acul::shared_ptr<umbf::Target>> &textures)
            {
                acul::hl_hashmap<acul::string, size_t> tex_map;
                mat_map.reserve(mtl_list.size());
                materials.resize(mtl_list.size());
                auto generator = acul::id_gen();
                for (size_t i = 0; i < mtl_list.size(); ++i)
                {
                    auto &mtl = mtl_list[i];
                    auto [mat_it, is_mat_inserted] = mat_map.emplace(mtl.name, i);
                    auto mat = acul::make_shared<umbf::Material>();
                    if (mtl.map_Kd.path.empty())
                        mat->albedo.rgb = mtl.Kd.value;
                    else
                    {
                        acul::string parsedPath = acul::io::replace_filename(base_path, mtl.map_Kd.path);
                        auto [it, inserted] = tex_map.insert({parsedPath, textures.size()});
                        if (inserted)
                        {
                            auto target = acul::make_shared<umbf::Target>();
                            target->header.vendor_sign = UMBF_VENDOR_ID;
                            target->header.vendor_version = UMBF_VERSION;
                            target->header.type_sign = umbf::sign_block::format::target;
                            target->header.spec_version = UMBF_VERSION;
                            target->header.compressed = false;
                            target->url = base_path;
                            target->checksum = 0;
                            textures.push_back(target);
                        }
                        mat->albedo.textured = true;
                        mat->albedo.texture_id = it->second;
                    }
                    materials[i] = acul::make_shared<umbf::File>();
                    materials[i]->header.vendor_sign = UMBF_VENDOR_ID;
                    materials[i]->header.vendor_version = UMBF_VERSION;
                    materials[i]->header.spec_version = UMBF_VERSION;
                    materials[i]->header.type_sign = umbf::sign_block::format::material;
                    materials[i]->blocks.push_back(mat);
                    materials[i]->blocks.push_back(acul::make_shared<umbf::MaterialInfo>(generator(), mat_it->first));
                }
            }

            void copy_write_data(ParseDataWrite &src, ParseDataRead &dst)
            {
                oneapi::tbb::parallel_sort(src.v.begin(), src.v.end());
                oneapi::tbb::parallel_sort(src.vn.begin(), src.vn.end());
                oneapi::tbb::parallel_sort(src.vt.begin(), src.vt.end());
                oneapi::tbb::parallel_sort(src.g.begin(), src.g.end());
                oneapi::tbb::parallel_sort(src.f.begin(), src.f.end());

                dst.v.assign(src.v.begin(), src.v.end());
                dst.vn.assign(src.vn.begin(), src.vn.end());
                dst.vt.assign(src.vt.begin(), src.vt.end());
                dst.g.assign(src.g.begin(), src.g.end());
                dst.f.assign(src.f.begin(), src.f.end());
                dst.use_mtl.assign(src.use_mtl.begin(), src.use_mtl.end());
            }

            void create_group_ranges(ParseDataRead &data, acul::vector<GroupRange> &groups)
            {
                groups.reserve(data.g.size() + 1);

                if (data.f.size() == 0) return;

                int lfi = 0;
                if (data.g.empty() || data.f.front().index < data.g.front().index)
                {
                    int range_end = data.g.size() == 0 ? data.f.back().index + 1 : data.g.front().index;
                    lfi = get_group_range_end(0, range_end, data);
                    groups.emplace_back(0, lfi, "default");
                }

                for (size_t g = 0; g < data.g.size(); ++g)
                {
                    int range_end = (g < data.g.size() - 1) ? data.g[g + 1].index : data.f.back().index + 1;
                    int temp = get_group_range_end(lfi, range_end, data);
                    groups.emplace_back(lfi, temp, data.g[g].value);
                    lfi = temp;
                }
            }

            void assign_materials_to_groups(ParseDataRead &data, const acul::vector<GroupRange> &groups,
                                            const acul::hl_hashmap<acul::string, int> &mat_map,
                                            acul::vector<acul::shared_ptr<umbf::File>> &materials,
                                            acul::vector<acul::vector<u32>> &ranges)
            {
                if (data.use_mtl.size() == 0) return;
                int um_id = 0;
                ranges.resize(groups.size());
                for (size_t g = 0; g < groups.size(); ++g)
                {
                    auto &group = groups[g];
                    const auto first = data.f[group.start_index].index;
                    const auto last = data.f[group.range_end - 1].index;
                    if (data.use_mtl[um_id].index < first)
                    {
                        while (um_id < (int)data.use_mtl.size() && data.use_mtl[um_id].index <= first) ++um_id;
                        for (--um_id; um_id < (int)data.use_mtl.size() && data.use_mtl[um_id].index <= last; ++um_id)
                        {
                            ranges[g].emplace_back(um_id);
                            auto it = mat_map.find(data.use_mtl[um_id].value);
                            if (it == mat_map.end())
                                LOG_WARN("Can't find mtl in library: %s", data.use_mtl[um_id].value.c_str());
                            else
                            {
                                auto meta = materials[it->second]->blocks;
                                auto m_it = std::find_if(meta.begin(), meta.end(), [](auto &block) {
                                    return block->signature() == umbf::sign_block::material_info;
                                });
                                if (m_it == meta.end())
                                {
                                    LOG_WARN("Can't find material info block");
                                    continue;
                                }
                                auto assignments = acul::static_pointer_cast<umbf::MaterialInfo>(*m_it)->assignments;
                                if (std::find(assignments.begin(), assignments.end(), g) == assignments.end())
                                    assignments.push_back(g);
                            }
                        }
                    }
                }
            }

            void assign_ranges_to_objects(ParseDataRead &data, const acul::vector<acul::vector<u32>> &ranges,
                                          acul::hl_hashmap<acul::string, int> &mat_map,
                                          const acul::vector<GroupRange> &groups, acul::vector<umbf::Object> &objects)
            {
                for (size_t gr = 0; gr < ranges.size(); ++gr)
                {
                    auto &group_range = ranges[gr];
                    if (group_range.size() > 1)
                    {
                        auto &group = groups[gr];
                        int f = group.start_index;
                        for (size_t i = 0; i < group_range.size(); ++i)
                        {
                            int m_next = (i == group_range.size() - 1) ? data.f[group.range_end - 1].index + 1
                                                                       : data.use_mtl[group_range[i + 1]].index;

                            auto it = mat_map.find(data.use_mtl[group_range[i]].value);
                            if (it == mat_map.end())
                                for (; f < (int)data.f.size() && data.f[f].index < m_next; ++f);
                            else
                            {
                                auto meta = acul::make_shared<umbf::MaterialRange>();
                                meta->mat_id = it->second;
                                for (; f < (int)data.f.size() && data.f[f].index < m_next; ++f)
                                    meta->faces.push_back(f - group.start_index);
                                objects[gr].meta.push_back(meta);
                            }
                        }
                    }
                }
            }

            struct ImportCtx
            {
                ParseDataRead data;
                acul::string mtlLib;
                acul::vector<GroupRange> groups;
            };

            Importer::~Importer() { acul::release(_ctx); }

            acul::io::file::op_state Importer::read_source()
            {
                _ctx = acul::alloc<ImportCtx>();
                ParseDataWrite parsed;
                acul::io::file::op_state result =
                    acul::io::file::read_by_block(_path, [&parsed](char *data, size_t size) {
                        acul::string_view_pool<char> pool;
                        pool.reserve(size / 40);
                        acul::io::file::fill_line_buffer(data, size, pool);
                        oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, pool.size(), 512),
                                                  [&](const oneapi::tbb::blocked_range<size_t> &range) {
                                                      for (size_t i = range.begin(); i != range.end(); ++i)
                                                          parse_line(parsed, pool[i], i);
                                                  });
                    });
                if (result != acul::io::file::op_state::success) return result;
                copy_write_data(parsed, _ctx->data);
                _ctx->mtlLib = parsed.mtllib;
                return acul::io::file::op_state::success;
            }

            void Importer::build_geometry()
            {
                create_group_ranges(_ctx->data, _ctx->groups);
                // Index Groups
                for (auto &group : _ctx->groups)
                {
                    LOG_INFO("Indexing group data: '%s'", group.name.c_str());
                    const size_t face_count = group.range_end - group.start_index;
                    group.mesh = acul::make_shared<Mesh>();
                    auto &m = group.mesh->model;
                    index_mesh(face_count, _ctx->data, group);
                    LOG_INFO("Imported vertices: %zu", m.vertices.size());
                    LOG_INFO("Imported faces: %zu", m.faces.size());
                    LOG_INFO("Triangulating mesh group");
                    acul::vector<acul::vector<u32>> ires(face_count);
                    oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, face_count),
                                              [&](const oneapi::tbb::blocked_range<size_t> &range) {
                                                  for (size_t i = range.begin(); i < range.end(); ++i)
                                                  {
                                                      ires[i] = utils::triangulate(m.faces[i], m.vertices);
                                                      m.faces[i].count = ires[i].size();
                                                  }
                                              });
                    u32 current_id = 0;
                    for (size_t i = 0; i < face_count; ++i)
                    {
                        m.faces[i].first_vertex = current_id;
                        m.indices.insert(m.indices.end(), ires[i].begin(), ires[i].end());
                        current_id += ires[i].size();
                    }
                    LOG_DEBUG("Indices: %zu", m.indices.size());
                    _objects.emplace_back(acul::id_gen()(), group.name);
                    _objects.back().meta.push_back(group.mesh);
                }
            }

            void Importer::load_materials()
            {
                if (_ctx->mtlLib.empty()) return;
                acul::vector<Material> mtl_materials;
                acul::io::path mtl_path = acul::io::path(_path).parent_path() / _ctx->mtlLib;
                parse_mtl(mtl_path, mtl_materials);
                acul::hl_hashmap<acul::string, int> mat_map;
                acul::vector<acul::vector<u32>> face_mat_ranges;
                convert_to_materials(_path, mtl_materials, mat_map, _materials, _textures);
                assign_materials_to_groups(_ctx->data, _ctx->groups, mat_map, _materials, face_mat_ranges);
                assign_ranges_to_objects(_ctx->data, face_mat_ranges, mat_map, _ctx->groups, _objects);
            }
        } // namespace obj
    } // namespace scene
} // namespace aecl