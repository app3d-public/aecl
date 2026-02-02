#pragma once

#include <acul/enum.hpp>
#include <acul/op_result.hpp>
#include <umbf/umbf.hpp>

namespace aecl
{
    namespace scene
    {
        struct MeshExportFlagBits
        {
            enum enum_type
            {
                none,
                transform_reverse_x = 0x1,
                transform_reverse_y = 0x2,
                transform_reverse_z = 0x4,
                transform_swap_xy = 0x8,
                transform_swap_xz = 0x10,
                transform_swap_yz = 0x20,
                export_uv = 0x40,
                export_normals = 0x80,
                export_triangulated = 0x100
            };
            using flag_bitmask = std::true_type;
        };

        using MeshExportFlags = acul::flags<MeshExportFlagBits>;

        namespace MaterialExportFlags
        {
            enum
            {
                none,           // Do not export materials
                texture_none,   // Do not export textures
                texture_origin, // As present in the material manager (external references)
                texture_copy    // Create "tex" folder and copy textures there (keeping them external, but local)
            };
        };

        class IExporter
        {
        public:
            acul::string path;
            MeshExportFlags mesh_flags;
            int material_flags;
            acul::vector<umbf::Object> objects;
            acul::vector<umbf::File> materials;
            acul::vector<acul::string> textures;

            /**
             * @brief Create a scene exporter
             * @param filename Name of the file
             **/
            IExporter(const acul::string &path) : path(path) {}

            virtual ~IExporter() = default;

            virtual acul::op_result save() = 0;

            inline void clear()
            {
                objects.clear();
                materials.clear();
                textures.clear();
            }

            const acul::string &error() const { return _error; }

        protected:
            acul::string _error;
        };
    } // namespace scene
} // namespace aecl