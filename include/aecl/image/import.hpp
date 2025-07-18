#pragma once

#include <acul/api.hpp>
#include <acul/io/file.hpp>
#include <umbf/umbf.hpp>
#include "format.hpp"

namespace aecl
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
            virtual acul::io::file::op_state load(const acul::string &path, acul::vector<umbf::Image2D> &images) = 0;
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
            virtual acul::io::file::op_state load(const acul::string &path,
                                                  acul::vector<umbf::Image2D> &images) override;

        protected:
            Format _format;

            static bool load_image(
                const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
                std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> load_handler,
                umbf::Image2D &info);
        };

        class BMPLoader final : public OIIOLoader
        {
        public:
            BMPLoader() : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Alpha, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class GIFLoader final : public OIIOLoader
        {
        public:
            GIFLoader() : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Multilayer, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class HDRLoader final : public OIIOLoader
        {
        public:
            HDRLoader()
                : OIIOLoader({FormatFlagBits::Bit16 | FormatFlagBits::Bit32, vk::Format::eUndefined,
                              vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class HEIFLoader final : public OIIOLoader
        {
        public:
            HEIFLoader()
                : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Alpha | FormatFlagBits::Multilayer,
                              vk::Format::eR8G8B8A8Srgb})
            {
            }
        };

        class JPEGLoader final : public OIIOLoader
        {
        public:
            JPEGLoader() : OIIOLoader({FormatFlagBits::Bit8, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class OpenEXRLoader final : public OIIOLoader
        {
        public:
            OpenEXRLoader()
                : OIIOLoader({FormatFlagBits::Bit16 | FormatFlagBits::Bit32 | FormatFlagBits::Alpha |
                                  FormatFlagBits::Multilayer,
                              vk::Format::eUndefined, vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class PNGLoader final : public OIIOLoader
        {
        public:
            PNGLoader()
                : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Bit16 | FormatFlagBits::Alpha,
                              vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class PBMLoader final : public OIIOLoader
        {
        public:
            PBMLoader() : OIIOLoader({FormatFlagBits::Bit8, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class TargaLoader final : public OIIOLoader
        {
        public:
            TargaLoader()
                : OIIOLoader(
                      {FormatFlagBits::Bit8 | FormatFlagBits::Bit16 | FormatFlagBits::Alpha | FormatFlagBits::ReadOnly,
                       vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint})
            {
            }
        };

        class TIFFLoader final : public OIIOLoader
        {
        public:
            TIFFLoader()
                : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Bit16 | FormatFlagBits::Bit32 |
                                  FormatFlagBits::Alpha | FormatFlagBits::Multilayer,
                              vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint,
                              vk::Format::eR32G32B32A32Sfloat})
            {
            }
        };

        class WebPLoader final : public OIIOLoader
        {
        public:
            WebPLoader() : OIIOLoader({FormatFlagBits::Bit8 | FormatFlagBits::Alpha, vk::Format::eR8G8B8A8Srgb}) {}
        };

        class UMBFLoader final : public ILoader
        {
        public:
            /**
             * @brief Load asset images from a given file path.
             *
             * @param path Filesystem path to the image.
             * @param images Vector to store the loaded image data.
             * @return True if the image is successfully loaded, false otherwise.
             */
            virtual acul::io::file::op_state load(const acul::string &path,
                                                  acul::vector<umbf::Image2D> &images) override;

        private:
            u32 _checksum = 0;
        };

        APPLIB_API ILoader *get_importer_by_path(const acul::string &path);
    } // namespace image
} // namespace aecl