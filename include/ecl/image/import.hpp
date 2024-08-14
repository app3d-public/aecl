#pragma once

#include <assets/image.hpp>
#include <core/api.hpp>
#include "core/io/file.hpp"
#include "format.hpp"

namespace ecl
{
    namespace image
    {
        /**
         * @brief Abstract base class for image loaders.
         *
         * Defines the interface for classes responsible for loading images. Derived classes must implement
         * the load method to provide specific loading functionality.
         */
        class ILoader
        {
        public:
            virtual ~ILoader() = default;

            /**
             * @brief Load images from a given file path.
             *
             * @param path Filesystem path to the image.
             * @param images Vector to store the loaded image data.
             * @return True if the image is successfully loaded, false otherwise.
             */
            virtual io::file::ReadState load(const std::filesystem::path &path, DArray<assets::ImageInfo> &images) = 0;
        };

        class OIIOLoader : public ILoader
        {
        public:
            OIIOLoader(Format format) : _format(format) {}

            /**
             * @brief Load images from a given file path using OIIO.
             *
             * @param path Filesystem path to the image.
             * @param images Vector to store the loaded image data.
             * @return True if the image is successfully loaded, false otherwise.
             */
            virtual io::file::ReadState load(const std::filesystem::path &path, DArray<assets::ImageInfo> &images) override;

        protected:
            Format _format;

            static bool loadImage(
                const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
                std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> loadHandler,
                assets::ImageInfo &info);
        };

        class BMPLoader final : public OIIOLoader
        {
        public:
            BMPLoader() : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class GIFLoader final : public OIIOLoader
        {
        public:
            GIFLoader() : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::multilayer, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class HDRLoader final : public OIIOLoader
        {
        public:
            HDRLoader()
                : OIIOLoader({FormatFlagBits::bit16 | FormatFlagBits::bit32, vk::Format::eUndefined,
                              vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class HEIFLoader final : public OIIOLoader
        {
        public:
            HEIFLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha | FormatFlagBits::multilayer,
                              vk::Format::eR8G8B8A8Srgb})
            {
            }
        };

        class JPEGLoader final : public OIIOLoader
        {
        public:
            JPEGLoader() : OIIOLoader({FormatFlagBits::bit8, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class JPEG2000Loader final : public OIIOLoader
        {
        public:
            JPEG2000Loader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                              vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class JPEGXLLoader final : public OIIOLoader
        {
        public:
            JPEGXLLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16, vk::Format::eR8G8B8A8Srgb,
                              vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class OpenEXRLoader final : public OIIOLoader
        {
        public:
            OpenEXRLoader()
                : OIIOLoader({FormatFlagBits::bit16 | FormatFlagBits::bit32 | FormatFlagBits::alpha |
                                  FormatFlagBits::multilayer,
                              vk::Format::eUndefined, vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class PNGLoader final : public OIIOLoader
        {
        public:
            PNGLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                              vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class PBMLoader final : public OIIOLoader
        {
        public:
            PBMLoader() : OIIOLoader({FormatFlagBits::bit8, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class RAWLoader final : public OIIOLoader
        {
        public:
            RAWLoader() : OIIOLoader({FormatFlagBits::bit16 | FormatFlagBits::readOnly, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class TargaLoader final : public OIIOLoader
        {
        public:
            TargaLoader()
                : OIIOLoader(
                      {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha | FormatFlagBits::readOnly,
                       vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class TIFFLoader final : public OIIOLoader
        {
        public:
            TIFFLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::bit32 |
                                  FormatFlagBits::alpha | FormatFlagBits::multilayer,
                              vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint,
                              vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class WebPLoader final : public OIIOLoader
        {
        public:
            WebPLoader() : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class AssetLoader final : public ILoader
        {
        public:
        public:
            /**
             * @brief Load asset images from a given file path.
             *
             * @param path Filesystem path to the image.
             * @param images Vector to store the loaded image data.
             * @return True if the image is successfully loaded, false otherwise.
             */
            virtual io::file::ReadState load(const std::filesystem::path &path, DArray<assets::ImageInfo> &images) override;

        private:
            u32 _checksum = 0;
        };

        APPLIB_API ILoader *getImporterByPath(const std::filesystem::path &path);
    } // namespace image
} // namespace ecl