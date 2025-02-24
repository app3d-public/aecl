#pragma once

#include <assets/asset.hpp>
#include <astl/enum.hpp>

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

        using MeshExportFlags = astl::flags<MeshExportFlagBits>;

        struct MaterialExportFlagBits
        {
            enum enum_type
            {
                none = 0x0,           // Do not export materials
                texture_none = 0x1,   // Do not export textures
                texture_origin = 0x2, // As present in the material manager (external references)
                texture_copyToLocal =
                    0x4 // Create "tex" folder and copy textures there (keeping them external, but local)
            };
            using flag_bitmask = std::true_type;
        };

        using MaterialExportFlags = astl::flags<MaterialExportFlagBits>;

        class IExporter
        {
        public:
            std::filesystem::path path;
            MeshExportFlags meshFlags;
            MaterialExportFlags materialFlags;
            astl::vector<assets::Object> objects;
            astl::vector<assets::Asset> materials;
            astl::vector<assets::Target::Addr> textures;

            /**
             * @brief Create a scene exporter
             * @param filename Name of the file
             **/
            IExporter(const std::filesystem::path &path) : path(path) {}

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