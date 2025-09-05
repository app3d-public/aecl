#pragma once

#include <umbf/umbf.hpp>
#include "format.hpp"

namespace aecl
{
    namespace image
    {
        struct OIIOParams
        {
            Format format;
            OIIOParams(Format format) : format(format) {}
        };

        namespace bmp
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                f32 dpi;
                bool dither;

                Params(const umbf::Image2D &image, f32 dpi = 72.0f, bool dither = false)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::alpha,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      dpi(dpi),
                      dither(dither)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &bp);
        } // namespace bmp

        namespace gif
        {
            struct Params : OIIOParams
            {
                acul::vector<umbf::Image2D> images;
                bool interlacing;
                int loops;
                int fps;

                Params(const acul::vector<umbf::Image2D> &images, bool interlacing = false, int loops = 0, int fps = 0)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::multilayer,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      images(images),
                      interlacing(interlacing),
                      loops(loops),
                      fps(fps)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &gp);
        } // namespace gif

        namespace hdr
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;

                Params(const umbf::Image2D &image)
                    : OIIOParams({FormatFlagBits::bit32,
                                  {umbf::ImageFormat::Type::none, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::sfloat}}),
                      image(image)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &hp);
        } // namespace hdr

        namespace heif
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                int compression; // Quality can be 1-100, with 100 meaning lossless

                Params(const umbf::Image2D &image, int compression = 100)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::alpha | FormatFlagBits::multilayer,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      compression(compression)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &hp);
        } // namespace heif

        namespace jpeg
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                f32 dpi;
                bool dither;
                bool progressive;
                int compression;
                acul::string app_name;

                Params(const umbf::Image2D &image, f32 dpi = 72.0f, bool dither = false, bool progressive = false,
                       int compression = 100, const acul::string &app_name = {})
                    : OIIOParams({FormatFlagBits::bit8,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      dpi(dpi),
                      dither(dither),
                      progressive(progressive),
                      compression(compression),
                      app_name(app_name)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &jp);
        } // namespace jpeg

        namespace openexr
        {
            struct Params : OIIOParams
            {
                const acul::vector<umbf::Image2D> &images;
                /*
                 * The image comression. Can be the one of:
                 * "none", "rle", "zip", "zips", "piz", "pxr24", "b44", "b44a",
                 * "dwaa", or "dwab". If the writer receives a request for a compression type
                 * it does not recognize or is not supported by the version of OpenEXR on the system,
                 * it will use "zip" by default. For "dwaa" and "dwab", the dwaCompressionLevel
                 * may be optionally appended to the compression name after a colon, like this: "dwaa:200".
                 * (The default DWA compression value is 45.) For "zip" and "zips" compression,
                 * a level from 1 to 9 may be appended (the default is "zip:4"),
                 * but note that this is only honored when building against OpenEXR 3.1.3 or later.
                 */
                acul::string compression;

                Params(const acul::vector<umbf::Image2D> &images, const acul::string &compression = "none")
                    : OIIOParams({FormatFlagBits::bit16 | FormatFlagBits::bit32 | FormatFlagBits::alpha |
                                      FormatFlagBits::multilayer,
                                  {umbf::ImageFormat::Type::none, umbf::ImageFormat::Type::sfloat,
                                   umbf::ImageFormat::Type::sfloat}}),
                      images(images),
                      compression(compression)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &op, u8 dst_bit);
        } // namespace openexr
        namespace png
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                f32 dpi;
                bool dither;
                /*   Compression level for zip/deflate compression,
                 * on a scale from 0 (fastest, minimal compression) to 9 (slowest, maximal compression).
                 * The default is 6. PNG compression is always lossless.
                 * @param filter Controls the "row filters" that prepare the image for optimal compression.
                 * The default is 0 (PNG_NO_FILTERS), but other values
                 * (which may be "or-ed" or summed to combine their effects) are 8 (PNG_FILTER_NONE),
                 * 16 (PNG_FILTER_SUB), 32 (PNG_FILTER_UP), 64 (PNG_FILTER_AVG), or 128 (PNG_FILTER_PAETH).
                 * @param unassociated_alpha Whether to use unassociated alpha in the exported image.
                 */
                int compression;
                int filter;
                bool unassociated_alpha;

                Params(const umbf::Image2D &image, f32 dpi = 72.0f, bool dither = false, int compression = 6,
                       int filter = 0, bool unassociated_alpha = false)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::uint,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      dpi(dpi),
                      dither(dither),
                      compression(compression),
                      filter(filter),
                      unassociated_alpha(unassociated_alpha)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &pp, u8 dst_bit);
        } // namespace png

        namespace pnm
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                bool binary;
                bool dither;

                Params(const umbf::Image2D &image, bool binary = false, bool dither = false)
                    : OIIOParams({FormatFlagBits::bit8,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      binary(binary),
                      dither(dither)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &pp);
        } // namespace pnm

        namespace targa
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                bool dither;
                // Compression level for the exporting image.
                // Values of none and rle are supported. The default is RLE.
                acul::string compression;
                acul::string app_name;
                // Meaning of any alpha channel (0 = none; 1 = undefined, ignore; 2 = undefined, preserve; 3 = useful
                // unassociated alpha; 4 = useful associated alpha / premultiplied color).
                int alpha_type;

                Params(const umbf::Image2D &image, bool dither = false, const acul::string &compression = "rle",
                       const acul::string &app_name = {}, int alpha_type = 4)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::alpha |
                                      FormatFlagBits::read_only,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::uint,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      dither(dither),
                      compression(compression),
                      app_name(app_name),
                      alpha_type(alpha_type)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &tp);
        } // namespace targa

        namespace tiff
        {
            struct Params : OIIOParams
            {
                acul::vector<umbf::Image2D> images;
                bool unassociated_alpha;
                bool dither;
                /* An integer representing the quality level for ZIP compression.
                 * A time-vs-space knob for zip compression, ranging from 1-9 (default is 6).
                 * Higher means compress to less space, but taking longer to do so.
                 * It is strictly a time vs space tradeoff, the visual image quality is identical (lossless)
                 * no matter what the setting.
                 */
                int zipquality;
                // A boolean representing whether to force the use of BigTIFF format that allows files to be more than
                // 4 GB (default: 0).
                bool force_big_tiff;
                f32 dpi;
                // A string representing the compression algorithm to be used for the exported image.
                acul::string compression;

                Params(const acul::vector<umbf::Image2D> &images, bool unassociated_alpha = false, bool dither = false,
                       int zipquality = 6, bool force_big_tiff = false, const acul::string &app_name = {},
                       f32 dpi = 72.0f, const acul::string &compression = "none")
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::bit16 | FormatFlagBits::bit32 |
                                      FormatFlagBits::alpha | FormatFlagBits::multilayer,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::uint,
                                   umbf::ImageFormat::Type::sfloat}}),
                      images(images),
                      unassociated_alpha(unassociated_alpha),
                      dither(dither),
                      zipquality(zipquality),
                      force_big_tiff(force_big_tiff),
                      dpi(dpi),
                      compression(compression)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &tp, u8 dst_bit);
        } // namespace tiff

        namespace webp
        {
            struct Params : OIIOParams
            {
                umbf::Image2D image;
                bool dither;

                Params(const umbf::Image2D &image, bool dither = false)
                    : OIIOParams({FormatFlagBits::bit8 | FormatFlagBits::alpha,
                                  {umbf::ImageFormat::Type::uint, umbf::ImageFormat::Type::none,
                                   umbf::ImageFormat::Type::none}}),
                      image(image),
                      dither(dither)
                {
                }
            };

            APPLIB_API bool save(const acul::string &path, Params &wp);
        } // namespace webp

        namespace umbf
        {
            struct Params
            {
                ::umbf::Image2D image;
                int compression = 5;
                u32 checksum;
            };

            APPLIB_API bool save(const acul::string &path, Params &up);
        } // namespace umbf

        inline bool is_image_equals(const ::umbf::Image2D &image, ::umbf::ImageFormat dst_format, size_t dst_channels)
        {
            if (image.channels.size() < dst_channels) return false;
            return dst_format == image.format;
        }
    } // namespace image
} // namespace aecl