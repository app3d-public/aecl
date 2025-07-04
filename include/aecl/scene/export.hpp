#pragma once

#include <acul/enum.hpp>
#include <umbf/umbf.hpp>

namespace aecl
{
    namespace scene
    {
        struct MeshExportFlagBits
        {
            enum enum_type
            {
                None,
                TransformReverseX = 0x1,
                TransformReverseY = 0x2,
                TransformReverseZ = 0x4,
                TransformSwapXY = 0x8,
                TransformSwapXZ = 0x10,
                TransformSwapYZ = 0x20,
                ExportUV = 0x40,
                ExportNormals = 0x80,
                ExportTriangulated = 0x100
            };
            using flag_bitmask = std::true_type;
        };

        using MeshExportFlags = acul::flags<MeshExportFlagBits>;

        namespace MaterialExportFlags
        {
            enum
            {
                None,              // Do not export materials
                TextureNone,       // Do not export textures
                TextureOrigin,     // As present in the material manager (external references)
                TextureCopyToLocal // Create "tex" folder and copy textures there (keeping them external, but local)
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

            virtual bool save() = 0;

            inline void clear()
            {
                objects.clear();
                materials.clear();
                textures.clear();
            }
        };
    } // namespace scene
} // namespace aecl