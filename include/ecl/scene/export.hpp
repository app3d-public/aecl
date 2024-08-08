#pragma once

#include <assets/image.hpp>
#include <assets/material.hpp>
#include <assets/scene.hpp>

namespace ecl
{
    namespace scene
    {
        enum class MeshExportFlagBits
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

        using MeshExportFlags = Flags<MeshExportFlagBits>;

        enum class MaterialExportFlagBits
        {
            none = 0x0,               // Do not export materials
            texture_none = 0x1,       // Do not export textures
            texture_origin = 0x2,     // As present in the material manager (external references)
            texture_copyToLocal = 0x4 // Create "tex" folder and copy textures there (keeping them external, but local)
        };

        using MaterialExportFlags = Flags<MaterialExportFlagBits>;

        struct MeshNode
        {
            assets::meta::mesh::MeshBlock *meta;
            std::string name;
            int matID = -1;
        };

        struct MaterialNode
        {
            std::string name;
            assets::MaterialInfo info;
        };

        struct TextureNode
        {
            assets::ImageInfo image;
            assets::ImageTypeFlags flags;
            std::filesystem::path path;
        };

        class IExporter
        {
        public:
            /**
             * @brief Create a scene exporter
             * @param filename Name of the file
             **/
            IExporter(const std::filesystem::path &path, MeshExportFlags meshFlags, MaterialExportFlags materialFlags)
                : _path(path), _meshFlags(meshFlags), _materialFlags(materialFlags)
            {
            }

            virtual ~IExporter() = default;

            virtual bool save() = 0;

            // Get the filename path
            const std::filesystem::path path() const { return _path.string(); }

            // Set the list of exported meshes
            IExporter &meshes(const DArray<MeshNode> &meshes)
            {
                _meshes = meshes;
                return *this;
            }

            // Get the list of exported meshes
            DArray<MeshNode> &meshes() { return _meshes; }

            // Set the list of exported materials
            IExporter &materials(const DArray<MaterialNode> &materials)
            {
                _materials = materials;
                return *this;
            }

            // Get the list of exported materials
            DArray<MaterialNode> &materials() { return _materials; }


            // Set the list of exported textures
            IExporter &textures(const DArray<TextureNode> &textures)
            {
                _textures = textures;
                return *this;
            }
            
            // Get the list of imported textures
            DArray<TextureNode> &textures() { return _textures; }

            inline void clear()
            {
                for (auto &mesh : _meshes) delete mesh.meta;
                _meshes.clear();
                _materials.clear();
                _textures.clear();
            }

        protected:
            std::filesystem::path _path;
            MeshExportFlags _meshFlags;
            MaterialExportFlags _materialFlags;
            DArray<MeshNode> _meshes;
            DArray<MaterialNode> _materials;
            DArray<TextureNode> _textures;
        };
    } // namespace scene
} // namespace ecl

template <>
struct FlagTraits<ecl::scene::MeshExportFlagBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ecl::scene::MeshExportFlags allFlags =
        ecl::scene::MeshExportFlagBits::none | ecl::scene::MeshExportFlagBits::transform_reverseX |
        ecl::scene::MeshExportFlagBits::transform_reverseY | ecl::scene::MeshExportFlagBits::transform_reverseZ |
        ecl::scene::MeshExportFlagBits::transform_swapXY | ecl::scene::MeshExportFlagBits::transform_swapXZ |
        ecl::scene::MeshExportFlagBits::transform_swapYZ | ecl::scene::MeshExportFlagBits::export_uv |
        ecl::scene::MeshExportFlagBits::export_normals | ecl::scene::MeshExportFlagBits::export_triangulated;
};

template <>
struct FlagTraits<ecl::scene::MaterialExportFlagBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ecl::scene::MaterialExportFlags allFlags =
        ecl::scene::MaterialExportFlagBits::none | ecl::scene::MaterialExportFlagBits::texture_none |
        ecl::scene::MaterialExportFlagBits::texture_origin | ecl::scene::MaterialExportFlagBits::texture_copyToLocal;
};