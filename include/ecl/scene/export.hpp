#pragma once

#include <acul/enum.hpp>
#include <umbf/umbf.hpp>

namespace ecl
{
    namespace scene
    {
        struct MeshExportFlagBits
        {
            enum enum_type
            {
                none,
                transform_reverseX = 0x1,
                transform_reverseY = 0x2,
                transform_reverseZ = 0x4,
                transform_swapXY = 0x8,
                transform_swapXZ = 0x10,
                transform_swapYZ = 0x20,
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
                none,               // Do not export materials
                texture_none,       // Do not export textures
                texture_origin,     // As present in the material manager (external references)
                texture_copyToLocal // Create "tex" folder and copy textures there (keeping them external, but local)
            };
        };

        class IExporter
        {
        public:
            acul::string path;
            MeshExportFlags meshFlags;
            int materialFlags;
            acul::vector<umbf::Object> objects;
            acul::vector<umbf::File> materials;
            acul::vector<acul::string> textures;

            /**
             * @brief Create a scene exporter
             * @param filename Name of the file
             **/
            IExporter(const acul::string &path) : path(path) {}

            virtual ~IExporter() = default;

            virtual bool save() = 0;

            inline void clear()
            {
                objects.clear();
                materials.clear();
                textures.clear();
            }
        };
    } // namespace scene
} // namespace ecl