#include <acul/hash/hashmap.hpp>
#include <acul/locales.hpp>
#include <acul/log.hpp>
#include <acul/string/string.hpp>
#include <acul/task.hpp>
#include <ecl/scene/obj/import.hpp>
#include <ecl/scene/utils.hpp>
#include <emhash/hash_table8.hpp>
#include <oneapi/tbb/parallel_sort.h>
#include <umbf/version.h>

namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            void parseLine(ParseIndexed &buffer, const char *line, int lineIndex)
            {
                if (line[0] == '\0' || line[0] == '#') return;
                const char *token = line;
                if (token[0] == 'v')
                {
                    if (isspace(token[1]))
                    {
                        token += 2;
                        glm::vec3 v;
                        if (!acul::stov3(token, v))
                            throw ParseException(line, lineIndex);
                        else
                            buffer.v.emplace_back(lineIndex, v);
                    }
                    else if (token[1] == 't')
                    {
                        token += 3;
                        glm::vec2 vt;
                        if (!acul::stov2(token, vt))
                            throw ParseException(line, lineIndex);
                        else
                            buffer.vt.emplace_back(lineIndex, vt);
                    }
                    else if (token[1] == 'n')
                    {
                        token += 3;
                        glm::vec3 vn;
                        if (!acul::stov3(token, vn))
                            throw ParseException(line, lineIndex);
                        else
                            buffer.vn.emplace_back(lineIndex, vn);
                    }
                }
                else if (token[0] == 'g' || token[0] == 'o')
                {
                    token += 2;
                    acul::string str = acul::trim_end(token);
                    if (str != "off" && !str.empty()) buffer.g.emplace_back(lineIndex, str);
                }
                else if (token[0] == 'f')
                {
                    token += 2;
                    acul::vector<glm::ivec3> *vtn = acul::alloc<acul::vector<glm::ivec3>>();
                    while (true)
                    {
                        int vId{0}, vtId{0}, vnId{0};
                        if (!(acul::stoi(token, vId))) break;
                        // Handle negative indices
                        if (vId < 0) vId += buffer.v.size() + 1;

                        if (vId > 0)
                        {
                            // Changed this section
                            if (*token == '/')
                            {
                                ++token;
                                if (acul::stoi(token, vtId))
                                    if (vtId < 0) vtId += buffer.vt.size() + 1;
                                if (*token == '/')
                                {
                                    ++token;
                                    if (acul::stoi(token, vnId))
                                        if (vnId < 0) vnId += buffer.vn.size() + 1;
                                }
                            }
                            vtn->emplace_back(vId, vtId, vnId);

                            // Skip spaces
                            while (isspace(*token)) ++token;
                        }
                        else
                            break;
                    }

                    buffer.f.emplace_back(lineIndex, vtn);
                }
                else if (strncmp(token, "mtllib", 6) == 0)
                {
                    token += 7;
                    buffer.mtllib = acul::str_range(token);
                }
                else if (strncmp(token, "usemtl", 6) == 0)
                {
                    token += 7;
                    buffer.useMtl.emplace_back(lineIndex, acul::str_range(token));
                }
            }

            struct ParseSingleThread
            {
                Line<glm::vec3> *__restrict v;
                size_t vsize;
                Line<glm::vec2> *__restrict vt;
                size_t vtsize;
                Line<glm::vec3> *__restrict vn;
                size_t vnsize;
                Line<acul::vector<glm::ivec3> *> *__restrict f;
                size_t fsize;
                Line<acul::string> *g;
                size_t gsize;
                Line<acul::string> *useMtl;
                size_t useMtlsize;
            };

            int getGroupRangeEnd(int startIndex, int rangeEnd, ParseSingleThread &ps)
            {
                for (int i = startIndex; i < ps.fsize; ++i)
                    if (ps.f[i].index >= rangeEnd) return i;
                return ps.fsize;
            }

            using namespace umbf::mesh;

            struct GroupRange
            {
                int startIndex;
                int rangeEnd;
                acul::string name;
                acul::shared_ptr<MeshBlock> mesh;
            };

            glm::vec3 calculateNormal(const ParseSingleThread &ps, const acul::vector<glm::ivec3> &__restrict inFace)
            {
                glm::vec3 normal{0.0f};
                for (int v = 0; v < inFace.size(); ++v)
                {
                    glm::vec3 current = ps.v[inFace[v].x - 1].value;
                    glm::vec3 next = ps.v[inFace[(v + 1) % inFace.size()].x - 1].value;
                    normal.x += (current.y - next.y) * (current.z + next.z);
                    normal.y += (current.z - next.z) * (current.x + next.x);
                    normal.z += (current.x - next.x) * (current.y + next.y);
                }
                return glm::normalize(normal);
            }

            void addVertexToFace(const ParseSingleThread &__restrict ps, u32 vgi, u32 current,
                                 emhash8::HashMap<glm::ivec3, u32> &vtnMap, const glm::ivec3 &vtn, Model &m,
                                 IndexedFace &face)
            {
                auto [vIt, vInserted] = vtnMap.emplace(vtn, vtnMap.size());
                if (vInserted)
                {
                    face.vertices.emplace_back(vgi, m.vertices.size());
                    m.vertices.emplace_back(ps.v[current].value);
                    auto &vertex = m.vertices.back();
                    if (vtn.y != 0 && ps.vtsize > vtn.y) vertex.uv = ps.vt[vtn.y - 1].value;
                    vertex.normal = vtn.z != 0 && ps.vnsize > vtn.z ? ps.vn[vtn.z - 1].value : face.normal;
                    m.aabb.min = glm::min(m.aabb.min, vertex.pos);
                    m.aabb.max = glm::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(vgi, vIt->second);
            }

            void addVertexToFace(const ParseSingleThread &__restrict ps, u32 vgi, u32 current,
                                 acul::vector<VertexGroup> &groups, const glm::ivec3 &vtn, Model &m, IndexedFace &face)
            {
                Vertex vertex{ps.v[current].value};
                if (vtn.y != 0 && ps.vtsize > vtn.y) vertex.uv = ps.vt[vtn.y - 1].value;
                if (vtn.z != 0 && ps.vnsize > vtn.z)
                    vertex.normal = ps.vn[vtn.z - 1].value;
                else
                    vertex.normal = face.normal;
                auto &vgv = groups[vgi].vertices;
                auto it = std::find_if(vgv.begin(), vgv.end(),
                                       [&m, &vertex](u32 index) { return m.vertices[index] == vertex; });
                if (it == vgv.end())
                {
                    vgv.emplace_back(m.vertices.size());
                    face.vertices.emplace_back(vgi, m.vertices.size());
                    m.vertices.emplace_back(vertex);
                    m.aabb.min = glm::min(m.aabb.min, vertex.pos);
                    m.aabb.max = glm::max(m.aabb.max, vertex.pos);
                }
                else
                    face.vertices.emplace_back(vgi, *it);
            }

            void indexMesh(size_t faceCount, const ParseSingleThread &ps, GroupRange &group)
            {
                emhash8::HashMap<glm::ivec3, u32> vtnMap;
                acul::vector<VertexGroup> mg;
                const bool useNormals = ps.vnsize > 0;
                if (useNormals)
                    vtnMap.reserve(ps.vsize);
                else
                    mg.resize(ps.vsize);
                auto &m = group.mesh->model;
                m.faces.resize(faceCount);
                acul::vector<int> posMap(ps.vsize, -1);
                for (size_t f = 0; f < faceCount; ++f)
                {
                    auto &inFace = ps.f[group.startIndex + f].value;
                    auto &face = m.faces[f];
                    face.normal = calculateNormal(ps, *inFace);
                    for (int v = 0; v < inFace->size(); ++v)
                    {
                        auto &vtn = (*inFace)[v];
                        const int current = vtn.x - 1;
                        if (ps.vsize < vtn.x) continue;
                        if (posMap[current] == -1) posMap[current] = m.group_count++;
                        if (useNormals)
                            addVertexToFace(ps, posMap[current], current, vtnMap, vtn, m, face);
                        else
                            addVertexToFace(ps, posMap[current], current, mg, vtn, m, face);
                    }
                }
            }

            void indexGroups(ParseSingleThread &ps, acul::vector<umbf::Object> &objects,
                             acul::vector<GroupRange> &groups)
            {
                for (auto &group : groups)
                {
                    logInfo("Indexing group data: '%s'", group.name.c_str());
                    const size_t faceCount = group.rangeEnd - group.startIndex;
                    group.mesh = acul::make_shared<MeshBlock>();
                    auto &m = group.mesh->model;
                    indexMesh(faceCount, ps, group);
                    logInfo("Imported vertices: %zu", m.vertices.size());
                    logInfo("Imported faces: %zu", m.faces.size());
                    logInfo("Triangulating mesh group");
                    acul::vector<acul::vector<u32>> ires(faceCount);
                    acul::vector<acul::vector<bary::Vertex>> bres(faceCount);
                    oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<size_t>(0, faceCount),
                                              [&](const oneapi::tbb::blocked_range<size_t> &range) {
                                                  for (size_t i = range.begin(); i < range.end(); ++i)
                                                  {
                                                      ires[i] = utils::triangulate(m.faces[i], m.vertices);
                                                      m.faces[i].indexCount = ires[i].size();
                                                      utils::buildBarycentric(bres[i], m.faces[i], m.vertices, ires[i]);
                                                  }
                                              });
                    auto &barycentrics = group.mesh->baryVertices;
                    u32 currentID = 0;
                    for (int i = 0; i < faceCount; ++i)
                    {
                        m.faces[i].startID = currentID;
                        m.indices.insert(m.indices.end(), ires[i].begin(), ires[i].end());
                        barycentrics.insert(barycentrics.end(), bres[i].begin(), bres[i].end());
                        currentID += ires[i].size();
                    }
                    logDebug("Indices: %zu", m.indices.size());
                    objects.emplace_back(acul::IDGen()(), group.name);
                    objects.back().meta.push_back(group.mesh);
                }
            }

            void parseMTL(const acul::string &basePath, const ParseIndexed &parsed, acul::vector<Material> &materials)
            {
                acul::string filename = acul::io::replace_filename(basePath, parsed.mtllib);
                std::ifstream fileStream(filename.c_str());
                if (!fileStream.is_open())
                {
                    logError("Could not open file: %s", filename.c_str());
                    return;
                }

                logInfo("Loading MTL file: %s", filename.c_str());
                std::ostringstream oss;
                oss << fileStream.rdbuf();
                acul::string fileContent = oss.str().c_str();
                fileStream.close();
                acul::string_pool<char> stringPool(fileContent.size());
                acul::io::file::fill_line_buffer(fileContent.c_str(), fileContent.size(), stringPool);

                // Process each line from the buffer
                int materialIndex = -1;
                int lineIndex = 1;
                for (const auto &line : stringPool) parseMTLline(line, materials, materialIndex, lineIndex++);
                logInfo("Loaded %zu materials", materials.size());
            }

            bool processMTLColorOption(const char *&token, ColorOption &dst)
            {
                ColorOption colorOption{};
                while (isspace(*token)) ++token;
                if (!isdigit(*token))
                {
                    if (strncmp(token, "xyz", 3) == 0)
                    {
                        colorOption.type = ColorOption::Type::XYZ;
                        token += 3;
                    }
                    else
                        return false;
                }
                glm::vec3 color;
                if (acul::stov3(token, color))
                    colorOption.value = color;
                else
                    return false;
                dst = colorOption;
                return true;
            }

            bool processOnOffFlag(const char *&token, bool &dst)
            {
                while (isspace(*token)) ++token;
                if (strncmp(token, "on", 2) == 0)
                {
                    dst = true;
                    token += 2;
                }
                else if (strncmp(token, "off", 3) == 0)
                {
                    dst = false;
                    token += 3;
                }
                else
                    return false;
                return true;
            }

            bool processMTLTextureOption(const char *&token, TextureOption &dst)
            {
                bool pathProcessed = false;
                do {
                    while (isspace(*token)) ++token;

                    if (token[0] == '-')
                    {
                        ++token;

                        if (strncmp(token, "blendu", 6) == 0)
                        {
                            token += 7;
                            if (!processOnOffFlag(token, dst.blendu)) return false;
                        }
                        else if (strncmp(token, "blendv", 6) == 0)
                        {
                            token += 7;
                            if (!processOnOffFlag(token, dst.blendv)) return false;
                        }
                        else if (strncmp(token, "boost", 5) == 0)
                        {
                            token += 6;
                            if (!acul::stof(token, dst.boost)) return false;
                        }
                        else if (strncmp(token, "mm", 2) == 0)
                        {
                            token += 3;
                            if (!acul::stov2(token, dst.mm)) return false;
                        }
                        else if (token[0] == 'o')
                        {
                            token += 2;
                            acul::stov3_opt(token, dst.offset);
                        }
                        else if (token[0] == 's')
                        {
                            token += 2;
                            acul::stov3_opt(token, dst.scale);
                        }
                        else if (token[0] == 't')
                        {
                            if (isspace(token[1]))
                            {
                                token += 2;
                                acul::stov3_opt(token, dst.turbulence);
                            }
                            else if (strncmp(token, "texres", 6) == 0)
                            {
                                token += 7;
                                if (!acul::stoi(token, dst.resolution)) return false;
                            }
                            else if (strncmp(token, "type", 4) == 0)
                            {
                                token += 5;
                                dst.type = acul::str_range(token);
                            }
                            else
                                return false;
                        }
                        else if (strncmp(token, "clamp", 5) == 0)
                        {
                            token += 6;
                            if (!processOnOffFlag(token, dst.clamp)) return false;
                        }
                        else if (strncmp(token, "bm", 2) == 0)
                        {
                            token += 3;
                            if (!acul::stof(token, dst.bumpIntensity)) return false;
                        }
                        else if (strncmp(token, "imfchan", 7) == 0)
                        {
                            token += 8;
                            while (isspace(*token)) ++token;
                            dst.imfchan = token[0];
                            ++token;
                        }
                        else
                        {
                            logWarn("Unknown option: %s", acul::str_range(token).c_str());
                            return false;
                        }
                    }
                    else
                    {
                        if (pathProcessed) break;
                        dst.path = acul::str_range(token);
                        pathProcessed = true;
                    }
                } while (true);
                return true;
            }

            void parseMTLline(const acul::string_view &line, acul::vector<Material> &materials, int &matIndex,
                              int lineIndex)
            {
                try
                {
                    if (line.empty() || line[0] == '\0' || line[0] == '#') return;
                    const char *token = line.data();
                    if (strncmp(token, "newmtl", 6) == 0)
                    {
                        token += 7;
                        Material material(acul::str_range(token));
                        materials.push_back(std::move(material));
                        ++matIndex;
                    }
                    else if (token[0] == 'K')
                    {
                        if (token[1] == 'a')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Ka))
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'd')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Kd))
                                throw ParseException(line, lineIndex);
                        }
                    }
                    else if (token[0] == 'N')
                    {
                        if (token[1] == 's')
                        {
                            token += 3;
                            float ns;
                            if (!acul::stof(token, ns)) throw ParseException(line, lineIndex);
                            materials[matIndex].Ns = ns;
                        }
                        else if (token[1] == 'i')
                        {
                            token += 3;
                            if (!acul::stof(token, materials[matIndex].Ni)) throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "illum", 5) == 0)
                    {
                        token += 6;
                        if (!acul::stoi(token, materials[matIndex].illum)) throw ParseException(line, lineIndex);
                    }
                    else if (token[0] == 'd')
                    {
                        token += 2;
                        if (!acul::stof(token, materials[matIndex].d)) throw ParseException(line, lineIndex);
                    }
                    else if (token[0] == 'T')
                    {
                        if (token[1] == 'r')
                        {
                            token += 3;
                            if (!acul::stof(token, materials[matIndex].Tr)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'f')
                        {
                            token += 3;
                            if (!processMTLColorOption(token, materials[matIndex].Tf))
                                throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "map_", 4) == 0)
                    {
                        if (token[4] == 'K')
                        {
                            if (token[5] == 'a')
                            {
                                token += 6;
                                TextureOption map_Ka;
                                if (processMTLTextureOption(token, map_Ka))
                                    materials[matIndex].map_Ka = std::move(map_Ka);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'd')
                            {
                                token += 6;
                                TextureOption map_Kd;
                                if (processMTLTextureOption(token, map_Kd))
                                    materials[matIndex].map_Kd = std::move(map_Kd);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 's')
                            {
                                token += 6;
                                TextureOption map_Ks;
                                if (processMTLTextureOption(token, map_Ks))
                                    materials[matIndex].map_Ks = std::move(map_Ks);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'e')
                            {
                                token += 6;
                                TextureOption map_Ke;
                                if (processMTLTextureOption(token, map_Ke))
                                    materials[matIndex].map_Ke = std::move(map_Ke);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                        }
                        else if (token[5] == 'N' && token[6] == 's')
                        {
                            token += 6;
                            TextureOption map_Ns;
                            if (processMTLTextureOption(token, map_Ns))
                                materials[matIndex].map_Ns = std::move(map_Ns);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'd')
                        {
                            token += 6;
                            TextureOption map_d;
                            if (processMTLTextureOption(token, map_d))
                                materials[matIndex].map_d = std::move(map_d);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'T' && token[6] == 'r')
                        {
                            token += 6;
                            TextureOption map_Tr;
                            if (processMTLTextureOption(token, map_Tr))
                                materials[matIndex].map_Tr = std::move(map_Tr);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        else if (strncmp(token, "map_bump", 8) == 0)
                        {
                            token += 9;
                            TextureOption map_bump;
                            if (processMTLTextureOption(token, map_bump))
                                materials[matIndex].map_bump = std::move(map_bump);
                            else
                                throw ParseException(line, lineIndex);
                        }
                        /// PBR Workflow
                        else if (token[4] == 'P')
                        {
                            if (token[5] == 'r')
                            {
                                token += 6;
                                TextureOption map_Pr;
                                if (processMTLTextureOption(token, map_Pr))
                                    materials[matIndex].map_Pr = std::move(map_Pr);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 'm')
                            {
                                token += 6;
                                TextureOption map_Pm;
                                if (processMTLTextureOption(token, map_Pm))
                                    materials[matIndex].map_Pm = std::move(map_Pm);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                            else if (token[5] == 's')
                            {
                                token += 6;
                                TextureOption map_Ps;
                                if (processMTLTextureOption(token, map_Ps))
                                    materials[matIndex].map_Ps = std::move(map_Ps);
                                else
                                    throw ParseException(line, lineIndex);
                            }
                        }
                    }
                    else if (strncmp(token, "bump", 4) == 0)
                    {
                        token += 5;
                        TextureOption bump;
                        if (processMTLTextureOption(token, materials[matIndex].map_bump))
                            materials[matIndex].map_bump = std::move(bump);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    else if (strncmp(token, "disp", 4) == 0)
                    {
                        token += 5;
                        TextureOption disp;
                        if (processMTLTextureOption(token, disp))
                            materials[matIndex].disp = std::move(disp);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    else if (strncmp(token, "decal", 5) == 0)
                    {
                        token += 6;
                        TextureOption decal;
                        if (processMTLTextureOption(token, decal))
                            materials[matIndex].decal = std::move(decal);
                        else
                            throw ParseException(line, lineIndex);
                    }
                    /// PBR Workflow
                    else if (token[0] == 'P')
                    {
                        if (token[1] == 'r')
                        {
                            token += 3;
                            if (!acul::stof(token, materials[matIndex].Pr)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'm')
                        {
                            token += 3;
                            if (!acul::stof(token, materials[matIndex].Pm)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 's')
                        {
                            token += 3;
                            if (!acul::stof(token, materials[matIndex].Ps)) throw ParseException(line, lineIndex);
                        }
                        else if (token[1] == 'c')
                        {
                            if (isspace(token[2]))
                            {
                                token += 3;
                                if (!acul::stof(token, materials[matIndex].Pc)) throw ParseException(line, lineIndex);
                            }
                            else if (token[2] == 'r')
                            {
                                token += 4;
                                if (!acul::stof(token, materials[matIndex].Pcr)) throw ParseException(line, lineIndex);
                            }
                        }
                    }
                    else if (strncmp(token, "aniso", 5) == 0)
                    {
                        if (isspace(token[5]))
                        {
                            token += 6;
                            if (!acul::stof(token, materials[matIndex].aniso)) throw ParseException(line, lineIndex);
                        }
                        else if (token[5] == 'r')
                        {
                            token += 7;
                            if (!acul::stof(token, materials[matIndex].anisor)) throw ParseException(line, lineIndex);
                        }
                    }
                    else if (strncmp(token, "norm", 4) == 0)
                    {
                        token += 5;
                        TextureOption norm;
                        if (processMTLTextureOption(token, norm))
                            materials[matIndex].norm = std::move(norm);
                        else
                            throw ParseException(line, lineIndex);
                    }
                }
                catch (const ParseException &e)
                {
                    logWarn("%s", e.what());
                }
                catch (const std::exception &e)
                {
                    logWarn("Failed to process line with error: %s", e.what());
                }
            }

            void convertToMaterials(const acul::string &basePath, const acul::vector<Material> &mtlMatList,
                                    emhash8::HashMap<acul::string, int> &matMap,
                                    acul::vector<acul::shared_ptr<umbf::File>> &materials,
                                    acul::vector<acul::shared_ptr<umbf::Target>> &textures)
            {
                emhash8::HashMap<acul::string, size_t> texMap;
                matMap.reserve(mtlMatList.size());
                materials.resize(mtlMatList.size());
                for (int i = 0; i < mtlMatList.size(); ++i)
                {
                    auto &mtl = mtlMatList[i];
                    auto [mIt, mInserted] = matMap.emplace(mtl.name, i);
                    auto mat = acul::make_shared<umbf::Material>();
                    if (mtl.map_Kd.path.empty())
                        mat->albedo.rgb = mtl.Kd.value;
                    else
                    {
                        acul::string parsedPath = acul::io::replace_filename(basePath, mtl.map_Kd.path);
                        auto [it, inserted] = texMap.insert({parsedPath, textures.size()});
                        if (inserted)
                        {
                            auto target = acul::make_shared<umbf::Target>();
                            target->header.vendor_sign = UMBF_VENDOR_ID;
                            target->header.vendor_version = UMBF_VERSION;
                            target->header.type_sign = umbf::sign_block::format::target;
                            target->header.spec_version = UMBF_VERSION;
                            target->header.compressed = false;
                            target->url = basePath;
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
                    materials[i]->blocks.push_back(acul::make_shared<umbf::MaterialInfo>(acul::IDGen()(), mIt->first));
                }
            }

            void allocatePS(ParseIndexed &pmt, ParseSingleThread &pst)
            {
                oneapi::tbb::parallel_sort(pmt.v.begin(), pmt.v.end());
                oneapi::tbb::parallel_sort(pmt.vn.begin(), pmt.vn.end());
                oneapi::tbb::parallel_sort(pmt.vt.begin(), pmt.vt.end());
                oneapi::tbb::parallel_sort(pmt.g.begin(), pmt.g.end());
                oneapi::tbb::parallel_sort(pmt.f.begin(), pmt.f.end());

                pst.vsize = pmt.v.size();
                pst.vtsize = pmt.vt.size();
                pst.vnsize = pmt.vn.size();
                pst.fsize = pmt.f.size();
                pst.gsize = pmt.g.size();
                pst.useMtlsize = pmt.useMtl.size();

                // Allocate
                pst.v = acul::alloc_n<Line<glm::vec3>>(pst.vsize);
                pst.vt = acul::alloc_n<Line<glm::vec2>>(pst.vtsize);
                pst.vn = acul::alloc_n<Line<glm::vec3>>(pst.vnsize);
                pst.f = acul::alloc_n<Line<acul::vector<glm::ivec3> *>>(pst.fsize);
                pst.g = acul::alloc_n<Line<acul::string>>(pst.gsize);
                pst.useMtl = acul::alloc_n<Line<acul::string>>(pst.useMtlsize);

                // Construct
                for (size_t i = 0; i < pmt.v.size(); ++i) new (&pst.v[i]) Line<glm::vec3>(pmt.v[i]);
                for (size_t i = 0; i < pmt.vt.size(); ++i) new (&pst.vt[i]) Line<glm::vec2>(pmt.vt[i]);
                for (size_t i = 0; i < pmt.vn.size(); ++i) new (&pst.vn[i]) Line<glm::vec3>(pmt.vn[i]);
                for (size_t i = 0; i < pmt.f.size(); ++i) new (&pst.f[i]) Line<acul::vector<glm::ivec3> *>(pmt.f[i]);
                for (size_t i = 0; i < pmt.g.size(); ++i) new (&pst.g[i]) Line<acul::string>(pmt.g[i]);
                for (size_t i = 0; i < pmt.useMtl.size(); ++i) new (&pst.useMtl[i]) Line<acul::string>(pmt.useMtl[i]);
            }

            void freePS(ParseSingleThread &ps)
            {
                acul::release(ps.v, ps.vsize);
                acul::release(ps.vt, ps.vtsize);
                acul::release(ps.vn, ps.vnsize);
                for (int i = 0; i < ps.fsize; ++i) acul::release(ps.f[i].value);
                acul::release(ps.f, ps.fsize);
                acul::release(ps.g, ps.gsize);
                acul::release(ps.useMtl, ps.useMtlsize);
            }

            void createGroupRanges(ParseSingleThread &ps, acul::vector<GroupRange> &groups)
            {
                groups.reserve(ps.gsize + 1);

                if (ps.fsize == 0) return;

                int lfi = 0;
                if (ps.gsize == 0 || ps.f->index < ps.g->index)
                {
                    int rangeEnd = ps.gsize == 0 ? ps.f[ps.fsize - 1].index + 1 : ps.g->index;
                    lfi = getGroupRangeEnd(0, rangeEnd, ps);
                    groups.emplace_back(0, lfi, "default");
                }

                for (int g = 0; g < ps.gsize; ++g)
                {
                    int rangeEnd = (g < ps.gsize - 1) ? ps.g[g + 1].index : ps.f[ps.fsize - 1].index + 1;
                    int temp = getGroupRangeEnd(lfi, rangeEnd, ps);
                    groups.emplace_back(lfi, temp, ps.g[g].value);
                    lfi = temp;
                }
            }

            void assignMaterialsToGroups(ParseSingleThread &ps, const acul::vector<GroupRange> &groups,
                                         const emhash8::HashMap<acul::string, int> &matMap,
                                         acul::vector<acul::shared_ptr<umbf::File>> &materials,
                                         acul::vector<acul::vector<u32>> &ranges)
            {
                if (ps.useMtlsize == 0) return;
                int umID = 0;
                ranges.resize(groups.size());
                for (int g = 0; g < groups.size(); ++g)
                {
                    auto &group = groups[g];
                    const auto first = ps.f[group.startIndex].index;
                    const auto last = ps.f[group.rangeEnd - 1].index;
                    if (ps.useMtl[umID].index < first)
                    {
                        while (umID < ps.useMtlsize && ps.useMtl[umID].index <= first) ++umID;
                        for (--umID; umID < ps.useMtlsize && ps.useMtl[umID].index <= last; ++umID)
                        {
                            ranges[g].emplace_back(umID);
                            auto it = matMap.find(ps.useMtl[umID].value);
                            if (it == matMap.end())
                                logWarn("Can't find mtl in library: %s", ps.useMtl[umID].value.c_str());
                            else
                            {
                                auto meta = materials[it->second]->blocks;
                                auto m_it = std::find_if(meta.begin(), meta.end(), [](auto &block) {
                                    return block->signature() == umbf::sign_block::meta::material_info;
                                });
                                if (m_it == meta.end())
                                {
                                    logWarn("Can't find material info block");
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

            void assignRangesToObjects(ParseSingleThread &ps, const acul::vector<acul::vector<u32>> &ranges,
                                       emhash8::HashMap<acul::string, int> &matMap,
                                       const acul::vector<GroupRange> &groups, acul::vector<umbf::Object> &objects)
            {
                for (int gr = 0; gr < ranges.size(); ++gr)
                {
                    auto &groupRange = ranges[gr];
                    if (groupRange.size() > 1)
                    {
                        auto &group = groups[gr];
                        int f = group.startIndex;
                        for (int i = 0; i < groupRange.size(); ++i)
                        {
                            int m_next = (i == groupRange.size() - 1) ? ps.f[group.rangeEnd - 1].index + 1
                                                                      : ps.useMtl[groupRange[i + 1]].index;

                            auto it = matMap.find(ps.useMtl[groupRange[i]].value);
                            if (it == matMap.end())
                                for (; f < ps.fsize && ps.f[f].index < m_next; ++f);
                            else
                            {
                                auto meta = acul::make_shared<umbf::MatRangeAssignAtrr>();
                                meta->matID = it->second;
                                for (; f < ps.fsize && ps.f[f].index < m_next; ++f)
                                    meta->faces.push_back(f - group.startIndex);
                                objects[gr].meta.push_back(meta);
                            }
                        }
                    }
                }
            }

            acul::io::file::op_state Importer::load(events::Manager &e)
            {
                auto start = std::chrono::high_resolution_clock::now();
                logInfo("Loading OBJ file: %s", _path.c_str());
                acul::string header = acul::format("%s %ls", _("loading"), acul::io::get_filename(_path).c_str());
                e.dispatch<task::UpdateEvent>((void *)this, header, _("file:read"));
                ParseIndexed parsed;
                acul::io::file::op_state result = acul::io::file::read_by_block(_path, parsed, parseLine);
                if (result != acul::io::file::op_state::success) return result;

                e.dispatch<task::UpdateEvent>((void *)this, header, _("data_serialization"), 0.2f);
                logInfo("Serializing parse result");
                ParseSingleThread ps;
                allocatePS(parsed, ps);
                acul::vector<GroupRange> groups;
                createGroupRanges(ps, groups);
                indexGroups(ps, _objects, groups);
                e.dispatch<task::UpdateEvent>((void *)this, header, _("materials:loading"), 0.8f);
                if (!parsed.mtllib.empty())
                {
                    acul::vector<Material> mtlMaterials;
                    parseMTL(_path, parsed, mtlMaterials);
                    emhash8::HashMap<acul::string, int> matMap;
                    acul::vector<acul::vector<u32>> faceMatRanges;
                    convertToMaterials(_path, mtlMaterials, matMap, _materials, _textures);
                    assignMaterialsToGroups(ps, groups, matMap, _materials, faceMatRanges);
                    assignRangesToObjects(ps, faceMatRanges, matMap, groups, _objects);
                }
                freePS(ps);
                auto end = std::chrono::high_resolution_clock::now();
                logInfo("Loaded in %.5f ms", std::chrono::duration<f64, std::milli>(end - start).count());

                return acul::io::file::op_state::success;
            }
        } // namespace obj
    } // namespace scene
} // namespace ecl