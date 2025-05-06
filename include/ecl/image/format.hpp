#pragma once

#include <OpenImageIO/imageio.h>
#include <acul/enum.hpp>
#include <acul/scalars.hpp>
#include <acul/string/string_view.hpp>
#include <vulkan/vulkan.hpp>

namespace ecl
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
                readOnly = 0x8,
                multilayer = 0x10,
                alpha = 0x20,
            };
            using flag_bitmask = std::true_type;
        };

        using FormatFlags = acul::flags<FormatFlagBits>;

        struct Format
        {
            FormatFlags flags = FormatFlagBits::none;
            vk::Format bit8format = vk::Format::eUndefined;
            vk::Format bit16format = vk::Format::eUndefined;
            vk::Format bit32format = vk::Format::eUndefined;
        };

        inline OIIO::TypeDesc vk_format_to_OIIO(vk::Format format)
        {
            switch (format)
            {
                case vk::Format::eR8G8B8A8Srgb:
                    return OIIO::TypeDesc::UINT8;
                case vk::Format::eR16G16B16A16Uint:
                    return OIIO::TypeDesc::UINT16;
                case vk::Format::eR32G32B32A32Uint:
                    return OIIO::TypeDesc::UINT32;
                case vk::Format::eR16G16B16A16Sfloat:
                    return OIIO::TypeDesc::HALF;
                case vk::Format::eR32G32B32A32Sfloat:
                    return OIIO::TypeDesc::FLOAT;
                default:
                    return OIIO::TypeDesc::UNKNOWN;
            }
        }

        inline vk::Format get_format_by_bit(size_t bit, Format format)
        {
            switch (bit)
            {
                case 1:
                    return format.bit8format;
                case 2:
                    return format.bit16format;
                case 4:
                    return format.bit32format;
                default:
                    return vk::Format::eUndefined;
            }
        }

        enum class Type
        {
            Unknown,
            BMP,
            GIF,
            HDR,
            HEIF,
            JPEG,
            OpenEXR,
            PNG,
            PBM,
            Targa,
            TIFF,
            WebP,
            UMBF
        };

        constexpr Type get_type_by_extension(acul::string_view extension)
        {
            constexpr std::array<std::pair<acul::string_view, Type>, 25> extension_map = {
                {{".bmp", Type::BMP},     {".gif", Type::GIF},   {".hdr", Type::HDR},   {".heif", Type::HEIF},
                 {".heic", Type::HEIF},   {".avif", Type::HEIF}, {".jpg", Type::JPEG},  {".jpe", Type::JPEG},
                 {".jpeg", Type::JPEG},   {".jif", Type::JPEG},  {".jfif", Type::JPEG}, {".jfi", Type::JPEG},
                 {".exr", Type::OpenEXR}, {".png", Type::PNG},   {".pbm", Type::PBM},   {".pgm", Type::PBM},
                 {".ppm", Type::PBM},     {".pnm", Type::PBM},   {".tga", Type::Targa}, {".tpic", Type::Targa},
                 {".tif", Type::TIFF},    {".tiff", Type::TIFF}, {".webp", Type::WebP}, {".umia", Type::UMBF},
                 {".umbf", Type::UMBF}}};

            for (const auto &[ext, type] : extension_map)
                if (ext == extension) return type;
            return Type::Unknown;
        }
    } // namespace image
} // namespace ecl