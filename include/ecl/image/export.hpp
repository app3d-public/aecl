#pragma once

#include <assets/image.hpp>
#include "format.hpp"

namespace ecl
{
    namespace image
    {
        /**
         * @brief Copies the source buffer to a new buffer based on the specified Vulkan format.
         *
         * This function creates a dynamically allocated copy of the source buffer based on the provided
         * Vulkan format. The correct bit depth is determined from the format, and a shared pointer
         * to the new buffer is returned.
         *
         * @param src Pointer to the source buffer.
         * @param size Size of the source buffer.
         * @param format Vulkan format that determines the bit depth of the source buffer.
         * @return Shared pointer to the dynamically allocated buffer containing the copied data.
         */
        std::shared_ptr<void> copySrcBuffer(const void *src, size_t size, vk::Format format);

        /**
         * @brief An abstract class for exporting images
         *
         * This class provides a common interface for exporting images in different formats.
         * It contains methods and fields that can be used by derived classes.
         */
        class IExporter
        {
        public:
            IExporter(const std::filesystem::path &path, const DArray<assets::ImageInfo> &images)
                : _path(path), _images(images)
            {
            }

            virtual ~IExporter() = default;

            /**
             * @brief Set path to exporting file
             *
             * @param path The new path to set
             */
            void path(const std::filesystem::path &path) { _path = path; }

            /**
             * @brief Getter for the exporting path
             *
             * @return The current path
             */
            std::filesystem::path path() const { return _path; }

            /**
             * @brief Get image subimages
             *
             * @return A vector of SubImageInfo objects containing information about the subimages
             */
            DArray<assets::ImageInfo> images() const { return _images; }

            virtual bool save(size_t dstBit) = 0;

        protected:
            std::filesystem::path _path;
            DArray<assets::ImageInfo> _images;
        };

        class OIIOExporter : public IExporter
        {
        public:
            OIIOExporter(const std::filesystem::path &path, const DArray<assets::ImageInfo> &images, Format format)
                : IExporter(path, images), _format(format)
            {
            }

            /**
             * @brief Get output image info
             *
             * @return An object containing information about the image format
             */
            Format format() { return _format; }

            virtual bool save(size_t dstBit) override
            {
                DArray<std::shared_ptr<void>> pixels;
                return save(dstBit, pixels);
            }

            virtual bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) = 0;

        protected:
            Format _format;
        };

        /**
         * @brief A class for exporting images in BMP format
         *
         * This class inherits from OIIOExporter and provides an implementation for saving images in BMP
         * format.
         */
        class BMPExporter final : public OIIOExporter
        {
        public:
            BMPExporter(const std::filesystem::path &path, const assets::ImageInfo &image, f32 dpi = 72.0f,
                        bool dither = false)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::alpha, vk::Format::eR8G8B8A8Srgb}),
                  _dpi(dpi),
                  _dither(dither)
            {
            }

            /**
             * Setter for the dpi field
             * When not a whole number of bytes per channel, this describes the bits per pixel in the file
             * (16 for R4G4B4, 8 for a 256-color palette image, 4 for a 16-color palette image, 1
             * for a 2-color palette image).
             */
            BMPExporter &dpi(f32 dpi)
            {
                _dpi = dpi;
                return *this;
            }

            /**
             * Getter for the dpi field
             * When not a whole number of bytes per channel, this describes the bits per pixel in the file
             * (16 for R4G4B4, 8 for a 256-color palette image, 4 for a 16-color palette image, 1
             * for a 2-color palette image).
             */
            f32 dpi() const { return _dpi; }

            /**
             * Setter for the dither field
             * If nonzero and outputting UINT8 values in the file from a source of higher bit depth,
             * will add a small amount of random dither to combat the appearance of banding.
             */
            BMPExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * Getter for the dither field
             * If nonzero and outputting UINT8 values in the file from a source of higher bit depth,
             * will add a small amount of random dither to combat the appearance of banding.
             */
            bool dither() const { return _dither; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            f32 _dpi;
            bool _dither;
        };

        /**
         * @brief A class for exporting images in GIF format
         *
         * This class inherits from OIIOExporter and provides an implementation for saving images in GIF
         * format.
         */
        class GIFExporter final : public OIIOExporter
        {
        public:
            GIFExporter(const std::filesystem::path &path, const DArray<assets::ImageInfo> &images,
                        bool interlacing = false, int loops = 0, int fps = 0)
                : OIIOExporter(path, images,
                               {FormatFlagBits::bit8 | FormatFlagBits::multilayer, vk::Format::eR8G8B8A8Srgb}),
                  _interlacing(interlacing),
                  _loops(loops),
                  _fps(fps)
            {
            }

            /**
             * @brief Specifies if image is interlaced (0 or 1).
             */
            GIFExporter &interlacing(bool interlacing)
            {
                _interlacing = interlacing;
                return *this;
            }

            /**
             * @brief Is image interlaced (0 or 1).
             */
            bool interlacing() const { return _interlacing; }

            /**
             * @brief Specifies the number of loops
             */
            GIFExporter &loops(int loops)
            {
                _loops = loops;
                return *this;
            }

            /**
             * @brief Get the loops count
             */
            int loops() const { return _loops; }

            /**
             * @brief Specifies the number of frames per second
             */
            GIFExporter &fps(int fps)
            {
                _fps = fps;
                return *this;
            }

            /**
             * @brief Get the fps count
             */
            int fps() const { return _fps; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _interlacing;
            int _loops;
            int _fps;
        };

        /**
         * @brief A class for exporting images in HDR (RGBE) format
         *
         * This class inherits from OIIOExporter and provides an implementation for saving images in HDR
         * format.
         */
        class HDRExporter final : public OIIOExporter
        {
        public:
            HDRExporter(const std::filesystem::path &path, const assets::ImageInfo &image)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit16 | FormatFlagBits::bit32, vk::Format::eUndefined,
                                vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32B32A32Sfloat})
            {
            }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;
        };

        /**
         * @brief A class for exporting images in HEIF format
         *
         * This class inherits from OIIOExporter and provides an implementation for saving images in HEIF
         * format.
         */
        class HEIFExporter final : public OIIOExporter
        {
        public:
            HEIFExporter(const std::filesystem::path &path, const assets::ImageInfo &image, int compression = 100)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::alpha | FormatFlagBits::multilayer,
                                vk::Format::eR8G8B8A8Srgb}),
                  _compression(compression)
            {
            }

            /**
             * Specifies the compression level
             * Quality can be 1-100, with 100 meaning lossless
             */
            HEIFExporter &compression(int compression)
            {
                _compression = compression;
                return *this;
            }

            // Get the compression level
            int compression() const { return _compression; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            int _compression;
        };

        /**
         * @brief A class that exports an image in JPEG format.
         */
        class JPEGExporter final : public OIIOExporter
        {
        public:
            JPEGExporter(const std::filesystem::path &path, const assets::ImageInfo &image, f32 dpi = 72.0f,
                         bool dither = false, bool progressive = false, int compression = 100,
                         const std::string &appName = "")
                : OIIOExporter(path, {image}, {FormatFlagBits::bit8, vk::Format::eR8G8B8A8Srgb}),
                  _dpi(dpi),
                  _dither(dither),
                  _progressive(progressive),
                  _compression(compression),
                  _appName(appName)
            {
            }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

            /**
             * @brief Sets the dots per inch of the image.
             * @param dpi A float representing the dots per inch of the image.
             */
            JPEGExporter &dpi(f32 dpi)
            {
                _dpi = dpi;
                return *this;
            }

            /**
             * @brief Returns the dots per inch of the image.
             * @return A float representing the dots per inch of the image.
             */
            f32 dpi() const { return _dpi; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            JPEGExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             */
            bool dither() const { return _dither; }

            /**
             * @brief Sets whether to use progressive compression in the exported image.
             * @param progressive A boolean representing whether to use progressive compression in the exported
             * image.
             */
            JPEGExporter &progressive(bool progressive)
            {
                _progressive = progressive;
                return *this;
            }

            /**
             * @brief Returns whether to use progressive compression in the exported image.
             * @return A boolean representing whether to use progressive compression in the exported image.
             */
            bool progressive() const { return _progressive; }

            /**
             * @brief Sets the compression level of the exported image. Quality can be 1-100,
             * with 100 meaning lossless.
             * @param compression An integer representing the compression level of the exported image.
             */
            JPEGExporter &compression(int compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Returns the compression level of the exported image.
             * @return An integer representing the compression level of the exported image.
             */
            int compression() const { return _compression; }

            /**
             * @brief Sets the name of the application that is exporting the image.
             * @param appName A string representing the name of the application that is exporting the image.
             */
            JPEGExporter &appName(const std::string &appName)
            {
                _appName = appName;
                return *this;
            }

            /**
             * @brief Returns the name of the application that is exporting the image.
             * @return A string representing the name of the application that is exporting the image.
             */
            const std::string &appName() const { return _appName; }

        private:
            f32 _dpi;
            bool _dither;
            bool _progressive;
            int _compression;
            std::string _appName;
        };

        /**
         * @brief A class that exports an image in JPEG2000 format.
         */
        class JPEG2000Exporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the JPEG2000Exporter class.
             * @param path The file path where to save the exported image.
             * @param image An object representing the subimage to be exported.
             * @param unassociatedAlpha Whether to use unassociated alpha in the exported image.
             * @param dither Whether to use dithering in the exported image.
             */
            JPEG2000Exporter(const std::filesystem::path &path, const assets::ImageInfo &image,
                             bool unassociatedAlpha = false, bool dither = false)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                                vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint}),
                  _unassociatedAlpha(unassociatedAlpha),
                  _dither(dither)
            {
            }

            /**
             * @brief Sets whether to use unassociated alpha in the exported image.
             * @param unassociatedAlpha A boolean representing whether to use unassociated alpha in the exported
             * image.
             */
            JPEG2000Exporter &unassociatedAlpha(bool unassociatedAlpha)
            {
                _unassociatedAlpha = unassociatedAlpha;
                return *this;
            }

            /**
             * @brief Returns whether to use unassociated alpha in the exported image.
             * @return A boolean representing whether to use unassociated alpha in the exported image.
             */
            bool unassociatedAlpha() const { return _unassociatedAlpha; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            JPEG2000Exporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             */
            bool dither() const { return _dither; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _unassociatedAlpha;
            bool _dither;
        };

        class JPEGXLExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the JPEGXLExporter class.
             * @param path The file path where to save the exported image.
             * @param image An object representing the subimage to be exported.
             */
            JPEGXLExporter(const std::filesystem::path &path, const assets::ImageInfo &image)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::bit16, vk::Format::eR8G8B8A8Srgb,
                                vk::Format::eR16G16B16A16Uint})
            {
            }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;
        };

        /**
         * @brief A class that exports an image in OpenEXR format.
         */
        class OpenEXRExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the EXRExporter class.
             * @param path The file path where to save the exported image.
             * @param images A vector of objects containing information about the subimages
             * @param compression The image comression. Can be the one of:
             * "none", "rle", "zip", "zips", "piz", "pxr24", "b44", "b44a",
             * "dwaa", or "dwab". If the writer receives a request for a compression type
             * it does not recognize or is not supported by the version of OpenEXR on the system,
             * it will use "zip" by default. For "dwaa" and "dwab", the dwaCompressionLevel
             * may be optionally appended to the compression name after a colon, like this: "dwaa:200".
             * (The default DWA compression value is 45.) For "zip" and "zips" compression,
             * a level from 1 to 9 may be appended (the default is "zip:4"),
             * but note that this is only honored when building against OpenEXR 3.1.3 or later.
             */
            OpenEXRExporter(const std::filesystem::path &path, const DArray<assets::ImageInfo> &images,
                            const std::string &compression = "none")
                : OIIOExporter(path, images,
                               {FormatFlagBits::bit16 | FormatFlagBits::bit32 | FormatFlagBits::alpha |
                                    FormatFlagBits::multilayer,
                                vk::Format::eUndefined, vk::Format::eR16G16B16A16Sfloat,
                                vk::Format::eR32G32B32A32Sfloat}),
                  _compression(compression)
            {
            }

            /**
             * @brief Sets the compression level of the exported image. See the constructor notes.
             * @param compression An integer representing the compression level of the exported image.
             */
            OpenEXRExporter &compression(const std::string &compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Returns the compression level of the exported image.
             * @return An string representing the compression level of the exported image.
             * See the constructor notes.
             */
            std::string compression() const { return _compression; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            std::string _compression;
        };

        /**
         * @brief A class that exports an image in PNG format.
         */
        class PNGExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the PNGxporter class.
             * @param path The file path where to save the exported image.
             * @param image An object representing the subimage to be exported.
             * @param dpi The dots per inch of the image.
             * @param dither Whether to use dithering in the exported image.
             * @param compression Compression level for zip/deflate compression,
             * on a scale from 0 (fastest, minimal compression) to 9 (slowest, maximal compression).
             * The default is 6. PNG compression is always lossless.
             * @param filter Controls the “row filters” that prepare the image for optimal compression.
             * The default is 0 (PNG_NO_FILTERS), but other values
             * (which may be “or-ed” or summed to combine their effects) are 8 (PNG_FILTER_NONE),
             * 16 (PNG_FILTER_SUB), 32 (PNG_FILTER_UP), 64 (PNG_FILTER_AVG), or 128 (PNG_FILTER_PAETH).
             * @param unassociatedAlpha Whether to use unassociated alpha in the exported image.
             */
            PNGExporter(const std::string &path, const assets::ImageInfo &image, f32 dpi = 72.0f, bool dither = false,
                        int compression = 6, int filter = 0, bool unassociatedAlpha = false)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                                vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint}),
                  _dpi(dpi),
                  _dither(dither),
                  _compression(compression),
                  _filter(filter),
                  _unassociatedAlpha(unassociatedAlpha)
            {
            }

            /**
             * @brief Sets the dots per inch of the image.
             * @param dpi A float representing the dots per inch of the image.
             */
            PNGExporter &dpi(f32 dpi)
            {
                _dpi = dpi;
                return *this;
            }

            /**
             * @brief Returns the dots per inch of the image.
             * @return A float representing the dots per inch of the image.
             */
            f32 dpi() const { return _dpi; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            PNGExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             */
            bool dither() const { return _dither; }

            /**
             * @brief Sets the compression level for zip/deflate compression.
             * @param compression An integer representing the compression level on a scale from 0 (fastest,
             * minimal compression) to 9 (slowest, maximal compression).
             */
            PNGExporter &compression(int compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Returns the compression level of the exported image.
             * @return An integer representing the compression level of the exported image.
             */
            int compression() const { return _compression; }

            /**
             * @brief Sets the row filters that prepare the image for optimal compression.
             * @param filter An integer representing the row filters, which may be "or-ed" or summed to combine
             * their effects.
             */
            PNGExporter &filter(int filter)
            {
                _filter = filter;
                return *this;
            }

            /**
             * @brief Returns the row filters that prepare the image for optimal compression.
             * @return An integer representing the row filters of the exported image.
             */
            int filter() const { return _filter; }

            /**
             * @brief Sets whether to use unassociated alpha in the exported image.
             * @param unassociatedAlpha A boolean representing whether to use unassociated alpha in the exported
             * image.
             */
            PNGExporter &unassociatedAlpha(bool unassociatedAlpha)
            {
                _unassociatedAlpha = unassociatedAlpha;
                return *this;
            }

            /**
             * @brief Returns whether to use unassociated alpha in the exported image.
             * @return A boolean representing whether to use unassociated alpha in the exported image.
             */
            bool unassociatedAlpha() const { return _unassociatedAlpha; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            f32 _dpi;
            bool _dither;
            int _compression;
            int _filter;
            bool _unassociatedAlpha;
        };

        /**
         * @brief A class that exports an image in PNM/PBM format.
         */
        class PNMExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the PNMExporter class.
             * @param path The file path where to save the exported image.
             * @param image An object representing the subimage to be exported.
             * @param binary Save as binary file (PBM)
             * @param dither Whether to use dithering in the exported image.
             */
            PNMExporter(const std::filesystem::path &path, const assets::ImageInfo &image, bool binary = false,
                        bool dither = false)
                : OIIOExporter(path, {image}, {FormatFlagBits::bit8, vk::Format::eR8G8B8A8Srgb}),
                  _binary(binary),
                  _dither(dither)
            {
            }

            /**
             * @brief Is exporting image in binary format.
             * @return A boolean representing whether the image is in binary format.
             */
            bool binary() const { return _binary; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            PNMExporter &binary(bool binary)
            {
                _binary = binary;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             */
            bool dither() const { return _dither; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            PNMExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _binary;
            bool _dither;
        };

        /**
         * @brief A class that exports an image in Targa format.
         */
        class TargaExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the TargaExporter class.
             * @param path The file path where to save the exported image.
             * @param image An object representing the subimage to be exported.
             * @param dither Whether to use dithering in the exported image.
             * @param compression Compression level for the exporting image.
             * Values of none and rle are supported. The default is RLE.
             * @param appName The name of the application that created the image.
             */
            TargaExporter(const std::filesystem::path &path, const assets::ImageInfo &image, bool dither = false,
                          const std::string &compression = "rle", const std::string &appName = "")
                : OIIOExporter(
                      path, {image},
                      {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha | FormatFlagBits::readOnly,
                       vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint}),
                  _dither(dither),
                  _compression(compression),
                  _appName(appName)
            {
            }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            TargaExporter &appName(const std::string &appName)
            {
                _appName = appName;
                return *this;
            }

            /**
             * @brief Gets the name of the application that created the image.
             */
            std::string appName() const { return _appName; }

            /**
             * @brief Set compression level for the exporting image.
             * @param compression Compression level for the exporting image.
             * For more details see the constructor description.
             */
            TargaExporter &compression(const std::string &compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Gets compression level for the exporting image.
             * @return Compression level for the exporting image.
             */
            std::string compression() const { return _compression; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            TargaExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             **/
            bool dither() const { return _dither; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _dither;
            std::string _compression;
            std::string _appName;
        };

        /**
         * @brief A class that exports an image in TIFF format.
         */
        class TIFFExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the TIFFExporter class.
             * @param path The file path where to save the exported image.
             * @param images A vector of objects representing the subimages to be
             * exported.
             * @param unassociatedAlpha A boolean representing whether to use unassociated alpha in the exported
             * image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             * @param zipquality An integer representing the quality level for ZIP compression.
             * A time-vs-space knob for zip compression, ranging from 1-9 (default is 6).
             * Higher means compress to less space, but taking longer to do so.
             * It is strictly a time vs space tradeoff, the visual image quality is identical (lossless)
             * no matter what the setting.
             * @param forceBigTIFF A boolean representing whether to force the use of BigTIFF format
             * that allows files to be more than 4 GB (default: 0).
             * @param appName A string representing the name of the application creating the TIFF file.
             * @param dpi A float representing the dots per inch of the image.
             * @param compression A string representing the compression algorithm to be used for the exported
             * image.
             */
            TIFFExporter(const std::filesystem::path &path, const DArray<assets::ImageInfo> &images,
                         bool unassociatedAlpha = false, bool dither = false, int zipquality = 6,
                         bool forceBigTIFF = false, const std::string &appName = "", f32 dpi = 72.0f,
                         const std::string &compression = "none")
                : OIIOExporter(path, images,
                               {FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::bit32 |
                                    FormatFlagBits::alpha | FormatFlagBits::multilayer,
                                vk::Format::eR8G8B8A8Srgb, vk::Format::eR16G16B16A16Uint,
                                vk::Format::eR32G32B32A32Sfloat}),
                  _unassociatedAlpha(unassociatedAlpha),
                  _dither(dither),
                  _zipquality(zipquality),
                  _forceBigTIFF(forceBigTIFF),
                  _appName(appName),
                  _dpi(dpi),
                  _compression(compression)
            {
            }

            /**
             * @brief Sets whether to use unassociated alpha in the exported image.
             * @param unassociatedAlpha A boolean representing whether to use unassociated alpha in the exported
             * image.
             */
            TIFFExporter &unassociatedAlpha(bool unassociatedAlpha)
            {
                _unassociatedAlpha = unassociatedAlpha;
                return *this;
            }

            /**
             * @brief Returns whether to use unassociated alpha in the exported image.
             * @return A boolean representing whether to use unassociated alpha in the exported image.
             */
            bool unassociatedAlpha() const { return _unassociatedAlpha; }

            /**
             * @brief Sets the name of the application creating the TIFF file.
             * @param appName A string representing the name of the application creating the TIFF file.
             */
            TIFFExporter &appName(const std::string &appName)
            {
                _appName = appName;
                return *this;
            }

            /**
             * @brief Returns the name of the application creating the TIFF file.
             * @return A string representing the name of the application creating the TIFF file.
             */
            std::string appName() const { return _appName; }

            /**
             * @brief Sets the compression algorithm to be used for the exported image.
             * @param compression A string representing the compression algorithm to be used for the exported
             * image.
             */
            TIFFExporter &compression(const std::string &compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Returns the compression algorithm to be used for the exported image.
             * @return A string representing the compression algorithm to be used for the exported image.
             */
            std::string compression() const { return _compression; }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            TIFFExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             */
            bool dither() const { return _dither; }

            /**
             * @brief Sets the quality level for ZIP compression.
             * @param zipquality An integer representing the quality level for ZIP compression.
             * For more details see the constructor description.
             */
            TIFFExporter &zipquality(int zipquality)
            {
                _zipquality = zipquality;
                return *this;
            }

            /**
             * @brief Returns the quality level for ZIP compression.
             * @return An integer representing the quality level for ZIP compression.
             */
            int zipquality() const { return _zipquality; }

            /**
             * @brief Sets whether to force the use of BigTIFF format.
             * @param forceBigTIFF A boolean representing whether to force the use of BigTIFF format.
             */
            TIFFExporter &forceBigTIFF(bool forceBigTIFF)
            {
                _forceBigTIFF = forceBigTIFF;
                return *this;
            }

            /**
             * @brief Returns whether to force the use of BigTIFF format.
             * @return A boolean representing whether to force the use of BigTIFF format.
             */
            bool forceBigTIFF() const { return _forceBigTIFF; }

            /**
             * @brief Sets the dots per inch of the image.
             * @param dpi A float representing the dots per inch of the image.
             */
            TIFFExporter &dpi(f32 dpi)
            {
                _dpi = dpi;
                return *this;
            }

            /**
             * @brief Returns the dots per inch of the image.
             * @return A float representing the dots per inch of the image.
             */
            f32 dpi() const { return _dpi; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _unassociatedAlpha;
            bool _dither;
            int _zipquality;
            bool _forceBigTIFF;
            std::string _appName;
            f32 _dpi;
            std::string _compression;
        };

        /**
         * @brief A class that exports an image in WebP format.
         */
        class WebPExporter final : public OIIOExporter
        {
        public:
            /**
             * @brief Constructor for the WebPExporter class.
             * @param path The file path where to save the exported image.
             * @param subimage An object of class SubImageInfo representing the subimage to be exported.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            WebPExporter(const std::string &path, const assets::ImageInfo &image, bool dither = false)
                : OIIOExporter(path, {image},
                               {FormatFlagBits::bit8 | FormatFlagBits::alpha, vk::Format::eR8G8B8A8Srgb}),
                  _dither(dither)
            {
            }

            /**
             * @brief Sets whether to use dithering in the exported image.
             * @param dither A boolean representing whether to use dithering in the exported image.
             */
            WebPExporter &dither(bool dither)
            {
                _dither = dither;
                return *this;
            }

            /**
             * @brief Returns whether to use dithering in the exported image.
             * @return A boolean representing whether to use dithering in the exported image.
             **/
            bool dither() const { return _dither; }

            using OIIOExporter::save;
            bool save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels) override;

        private:
            bool _dither;
        };

        class AssetExporter final : public IExporter
        {
        public:
            AssetExporter(const std::string &path, const assets::ImageInfo &image, int compression = 5,
                          u32 checksum = 0, assets::ImageTypeFlags flags = assets::ImageTypeFlagBits::tUndefined)
                : IExporter(path, {image})
            {
            }

            /**
             * @brief Sets the compression level for the exported image.
             * @param compression An integer representing the compression level for the exported image.
             * If the compression level is not set, the default compression level is 5.
             * If the compression equals 0, the image will not be compressed.
             * You can use ZSTD compression levels to adjust the compression ratio and speed according to your
             * requirements. The ZSTD library supports compression levels from 1 to 22.
             */
            AssetExporter &compression(int compression)
            {
                _compression = compression;
                return *this;
            }

            /**
             * @brief Returns the compression level for the exported image.
             * @return An integer representing the compression level for the exported image.
             */
            int compression() const { return _compression; }

            /**
             * @brief Sets the checksum for the exported image.
             * @param checksum An u32 value representing the checksum for the exported image.
             * If the checksum is not set, the checksum will not be set in the image.
             */
            AssetExporter &checksum(u32 checksum)
            {
                _checksum = checksum;
                return *this;
            }

            /**
             * @brief Returns the checksum for the exported image.
             * @return An u32 value representing the checksum for the exported image.
             */
            u32 checksum() const { return _checksum; }

            /**
             * @brief Sets the image type flags for the exported image.
             * @param flags An assets::ImageTypeFlags value representing the image type flags for the exported image.
             * If the image type flags are not set, the default image type flags will be used.
             */
            AssetExporter &flags(assets::ImageTypeFlags flags)
            {
                _flags = flags;
                return *this;
            }

            /**
             * @brief Returns the image type flags for the exported image.
             * @return An assets::ImageTypeFlags value representing the image type flags for the exported image.
             */
            assets::ImageTypeFlags flags() const { return _flags; }

            virtual bool save(size_t dstBit) override;

        private:
            int _compression;
            u32 _checksum;
            assets::ImageTypeFlags _flags;
        };

        inline bool isImageEquals(const assets::ImageInfo &image, Format srcFormat, vk::Format dstFormat)
        {
            if (image.channelCount != 3 && !((srcFormat.flags & FormatFlagBits::alpha) && image.channelCount == 4))
                return false;
            return dstFormat == image.imageFormat;
        }
    } // namespace image
} // namespace ecl