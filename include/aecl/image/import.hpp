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
            virtual acul::io::file::op_state load(const acul::string &path, acul::vector<::umbf::Image2D> &images) = 0;
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
                                                  acul::vector<::umbf::Image2D> &images) override;

        protected:
            Format _format;

            static bool load_image(
                const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
                std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> load_handler,
                ::umbf::Image2D &info);
        };

        class BMPLoader final : public OIIOLoader
        {
        public:
            BMPLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class GIFLoader final : public OIIOLoader
        {
        public:
            GIFLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::multilayer,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class HDRLoader final : public OIIOLoader
        {
        public:
            HDRLoader()
                : OIIOLoader({FormatFlagBits::bit16 | FormatFlagBits::bit32,
                              {::umbf::ImageFormat::Type::none, ::umbf::ImageFormat::Type::sfloat,
                               ::umbf::ImageFormat::Type::sfloat}})
            {
            }
        };

        class HEIFLoader final : public OIIOLoader
        {
        public:
            HEIFLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha | FormatFlagBits::multilayer,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class JPEGLoader final : public OIIOLoader
        {
        public:
            JPEGLoader()
                : OIIOLoader({FormatFlagBits::bit8,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class OpenEXRLoader final : public OIIOLoader
        {
        public:
            OpenEXRLoader()
                : OIIOLoader({FormatFlagBits::bit16 | FormatFlagBits::bit32 | FormatFlagBits::alpha |
                                  FormatFlagBits::multilayer,
                              {::umbf::ImageFormat::Type::none, ::umbf::ImageFormat::Type::sfloat,
                               ::umbf::ImageFormat::Type::sfloat}})
            {
            }
        };

        class PNGLoader final : public OIIOLoader
        {
        public:
            PNGLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::uint,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class PBMLoader final : public OIIOLoader
        {
        public:
            PBMLoader()
                : OIIOLoader({FormatFlagBits::bit8,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class TargaLoader final : public OIIOLoader
        {
        public:
            TargaLoader()
                : OIIOLoader(
                      {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha | FormatFlagBits::read_only,
                       {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::uint,
                        ::umbf::ImageFormat::Type::none}})
            {
            }
        };

        class TIFFLoader final : public OIIOLoader
        {
        public:
            TIFFLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::bit32 |
                                  FormatFlagBits::alpha | FormatFlagBits::multilayer,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::uint,
                               ::umbf::ImageFormat::Type::sfloat}})
            {
            }
        };

        class WebPLoader final : public OIIOLoader
        {
        public:
            WebPLoader()
                : OIIOLoader({FormatFlagBits::bit8 | FormatFlagBits::alpha,
                              {::umbf::ImageFormat::Type::uint, ::umbf::ImageFormat::Type::none,
                               ::umbf::ImageFormat::Type::none}})
            {
            }
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
                                                  acul::vector<::umbf::Image2D> &images) override;

        private:
            u32 _checksum = 0;
        };

        APPLIB_API ILoader *get_importer_by_path(const acul::string &path);
    } // namespace image
} // namespace aecl