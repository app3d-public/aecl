#pragma once

#include <acul/hash/hl_hashmap.hpp>
#include <acul/op_result.hpp>
#include <acul/string/sstream.hpp>
#include <oneapi/tbb/concurrent_unordered_set.h>
#include "../export.hpp"

namespace aecl
{
    namespace scene
    {
        namespace obj
        {
            // Policy for exporting objects
            struct ObjExportFlagBits
            {
                enum enum_type
                {
                    // Mesh Group Policy
                    object_policy_default = 0x0, // Exporting all object in 'default' group (without grouping)
                    object_policy_groups = 0x1,  // Exporting objects with 'g' flag
                    object_policy_objects = 0x2, // Exporting objects with 'o' flag
                    materials_pbr = 0x4          // Using PBR materials workflow
                };
                using flag_bitmask = std::true_type;
            };

            using ObjExportFlags = acul::flags<ObjExportFlagBits>;

            struct MaterialRef
            {
                acul::shared_ptr<umbf::MaterialInfo> info;
                acul::shared_ptr<umbf::Material> mat;
            };

            // OBJ file exporter
            class APPLIB_API Exporter final : public IExporter
            {
            public:
                ObjExportFlags obj_flags;
                /**
                 * Constructs an Exporter object with the given parameters.
                 *
                 * @param path The path to the output file.
                 */
                Exporter(const acul::string &path) : IExporter(path) {}

                /**
                 * Saves the exported scene to the output file.
                 *
                 * @return `true` if the export was successful, `false` otherwise.
                 */
                acul::op_result save() override;

            private:
                acul::hl_hashmap<amal::vec2, u32> _vt_map;
                acul::hl_hashmap<amal::vec3, u32> _vn_map;
                acul::hashmap<u64, MaterialRef> _material_map;
                bool _all_materials_exist = true;

                void write_vertices(umbf::mesh::Model &model, const acul::vector<umbf::mesh::VertexGroup> &groups,
                                    acul::stringstream &ss);
                void write_faces(umbf::mesh::Mesh *meta, acul::stringstream &os, const acul::vector<u32> &faces);
                void write_triangles(umbf::mesh::Mesh *meta, acul::stringstream &os, const acul::vector<u32> &faces,
                                     const acul::vector<umbf::mesh::VertexGroup> &groups);
                void write_texture(acul::stringstream &os, const acul::string &token, const acul::string &tex);

                void write_material(const acul::shared_ptr<umbf::MaterialInfo> &material_info,
                                    const acul::shared_ptr<umbf::Material> &material, std::ostream &os);
                bool write_mtllib_info(std::ofstream &mtl_stream, acul::stringstream &obj_stream);
                void write_mtl(std::ofstream &stream);
                u32 write_object(const umbf::Object &object, acul::stringstream &stream);
            };
        } // namespace obj
    } // namespace scene
} // namespace aecl