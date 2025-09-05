#pragma once

#include <OpenImageIO/imageio.h>
#include <acul/enum.hpp>
#include <acul/scalars.hpp>
#include <acul/string/string_view.hpp>
#include "umbf/umbf.hpp"

namespace aecl
{
    namespace image
    {
        struct FormatFlagBits
        {
            enum enum_type
            {
                none = 0x0,
                bit8 = 0x1,
                bit16 = 0x2,
                bit32 = 0x4,
                read_only = 0x8,
                multilayer = 0x10,
                alpha = 0x20,
            };
            using flag_bitmask = std::true_type;
        };

        using FormatFlags = acul::flags<FormatFlagBits>;

        struct Format
        {
            FormatFlags flags = FormatFlagBits::none;
            u8 format_types[3];
        };

        inline OIIO::TypeDesc umbf_format_to_oiio(umbf::ImageFormat format)
        {
            switch (format.type)
            {
                case umbf::ImageFormat::Type::uint:
                    switch (format.bytes_per_channel)
                    {
                        case 1:
                            return OIIO::TypeDesc::UINT8;
                        case 2:
                            return OIIO::TypeDesc::UINT16;
                        case 4:
                            return OIIO::TypeDesc::UINT32;
                    }
                    break;
                case umbf::ImageFormat::Type::sfloat:
                    switch (format.bytes_per_channel)
                    {
                        case 2:
                            return OIIO::TypeDesc::HALF;
                        case 4:
                            return OIIO::TypeDesc::FLOAT;
                    }
                    break;
                default:
                    break;
            }
            return OIIO::TypeDesc::UNKNOWN;
        }

        enum class Type
        {
            unknown,
            bmp,
            gif,
            hdr,
            heif,
            jpeg,
            openexr,
            png,
            pbm,
            targa,
            tiff,
            webp,
            umbf
        };

        constexpr Type get_type_by_extension(acul::string_view extension)
        {
            constexpr std::array<std::pair<acul::string_view, Type>, 25> extension_map = {
                {{".bmp", Type::bmp},     {".gif", Type::gif},   {".hdr", Type::hdr},   {".heif", Type::heif},
                 {".heic", Type::heif},   {".avif", Type::heif}, {".jpg", Type::jpeg},  {".jpe", Type::jpeg},
                 {".jpeg", Type::jpeg},   {".jif", Type::jpeg},  {".jfif", Type::jpeg}, {".jfi", Type::jpeg},
                 {".exr", Type::openexr}, {".png", Type::png},   {".pbm", Type::pbm},   {".pgm", Type::pbm},
                 {".ppm", Type::pbm},     {".pnm", Type::pbm},   {".tga", Type::targa}, {".tpic", Type::targa},
                 {".tif", Type::tiff},    {".tiff", Type::tiff}, {".webp", Type::webp}, {".umia", Type::umbf},
                 {".umbf", Type::umbf}}};

            for (const auto &[ext, type] : extension_map)
                if (ext == extension) return type;
            return Type::unknown;
        }
    } // namespace image
} // namespace aecl