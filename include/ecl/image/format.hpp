#pragma once

#include <OpenImageIO/imageio.h>
#include <core/std/basic_types.hpp>
#include <core/std/enum.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>

namespace ecl
{
    namespace image
    {
        enum class FormatFlagBits
        {
            none = 0x0,
            bit8 = 0x1,
            bit16 = 0x2,
            bit32 = 0x4,
            readOnly = 0x8,
            multilayer = 0x10,
            alpha = 0x20
        };

        using FormatFlags = Flags<FormatFlagBits>;

        struct Format
        {
            FormatFlags flags = FormatFlagBits::none;
            vk::Format bit8format = vk::Format::eUndefined;
            vk::Format bit16format = vk::Format::eUndefined;
            vk::Format bit32format = vk::Format::eUndefined;
        };

        inline OIIO::TypeDesc vkFormatToOIIO(vk::Format format)
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

        inline vk::Format getFormatByBit(size_t bit, Format format)
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
            JPEG2000,
            JPEGXL,
            OpenEXR,
            PNG,
            PBM,
            RAW,
            Targa,
            TIFF,
            WebP,
            Asset
        };

        constexpr Type getTypeByExt(std::string_view extension)
        {
            constexpr std::array<std::pair<std::string_view, Type>, 54> extMap = {
                {{".bmp", Type::BMP},      {".gif", Type::GIF},      {".hdr", Type::HDR},    {".heif", Type::HEIF},
                 {".heic", Type::HEIF},    {".avif", Type::HEIF},    {".jpg", Type::JPEG},   {".jpe", Type::JPEG},
                 {".jpeg", Type::JPEG},    {".jif", Type::JPEG},     {".jfif", Type::JPEG},  {".jfi", Type::JPEG},
                 {".jp2", Type::JPEG2000}, {".j2k", Type::JPEG2000}, {".jxl", Type::JPEGXL}, {".exr", Type::OpenEXR},
                 {".png", Type::PNG},      {".pbm", Type::PBM},      {".pgm", Type::PBM},    {".ppm", Type::PBM},
                 {".ari", Type::RAW},      {".dpx", Type::RAW},      {".arw", Type::RAW},    {".srf", Type::RAW},
                 {".sr2", Type::RAW},      {".bay", Type::RAW},      {".crw", Type::RAW},    {".cr2", Type::RAW},
                 {".cr3", Type::RAW},      {".dng", Type::RAW},      {".dcr", Type::RAW},    {".kdc", Type::RAW},
                 {".erf", Type::RAW},      {".3fr", Type::RAW},      {".mef", Type::RAW},    {".mrw", Type::RAW},
                 {".nef", Type::RAW},      {".nrw", Type::RAW},      {".orf", Type::RAW},    {".ptx", Type::RAW},
                 {".pef", Type::RAW},      {".raf", Type::RAW},      {".raw", Type::RAW},    {".rwl", Type::RAW},
                 {".rw2", Type::RAW},      {".r3d", Type::RAW},      {".srw", Type::RAW},    {".x3f", Type::RAW},
                 {"tga", Type::Targa},     {".tpic", Type::Targa},   {".tif", Type::TIFF},   {".tiff", Type::TIFF},
                 {".webp", Type::WebP},    {".a3d", Type::Asset}}};

            for (const auto &[ext, type] : extMap)
                if (ext == extension) return type;
            return Type::Unknown;
        }
    } // namespace image
} // namespace ecl

template <>
struct FlagTraits<ecl::image::FormatFlagBits>
{
    static constexpr bool isBitmask = true;
    static constexpr ecl::image::FormatFlags allFlags =
        ecl::image::FormatFlagBits::none | ecl::image::FormatFlagBits::bit8 | ecl::image::FormatFlagBits::bit16 |
        ecl::image::FormatFlagBits::bit32 | ecl::image::FormatFlagBits::readOnly |
        ecl::image::FormatFlagBits::multilayer | ecl::image::FormatFlagBits::alpha;
};