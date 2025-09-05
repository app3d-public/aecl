#include <acul/log.hpp>
#include <aecl/image/export.hpp>
#include <umbf/utils.hpp>
#include <umbf/version.h>

namespace aecl
{
    namespace image
    {
        bool bmp::save(const acul::string &path, Params &bp)
        {
            auto &image = bp.image;
            void *pixels = image.pixels;
            acul::unique_ptr<void> tmp;
            ::umbf::ImageFormat dst_format = {bp.format.format_types[0], 1};
            if (!is_image_equals(image, dst_format, 3))
            {
                pixels = ::umbf::utils::convert_image(image, dst_format, 3);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, image.channels.size(), OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", bp.dpi);
            spec.attribute("YResolution", bp.dpi);
            if (bp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool gif::save(const acul::string &path, Params &gp)
        {
            acul::vector<acul::unique_ptr<void>> tmp;
            acul::vector<void *> pixels;
            pixels.reserve(gp.images.size());
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            acul::vector<OIIO::ImageSpec> specs;

            ::umbf::ImageFormat dst_format = {gp.format.format_types[0], 1};
            for (auto &image : gp.images)
            {
                if (!is_image_equals(image, dst_format, 3))
                {
                    pixels.emplace_back(::umbf::utils::convert_image(image, dst_format, 3));
                    tmp.emplace_back(pixels.back());
                }
                else
                    pixels.emplace_back(image.pixels);
                OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
                if (gp.interlacing) spec.attribute("gif:Interlacing", 1);
                if (gp.images.size() > 1)
                {
                    spec.attribute("oiio:LoopCount", gp.loops);
                    spec.attribute("oiio:Movie", 1);
                    spec.attribute("gif:FPS", gp.fps);
                }
                specs.push_back(spec);
            }

            if (!out->open(pPath, specs.size(), specs.data()))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }

            for (size_t i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        LOG_ERROR("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(OIIO::TypeDesc::UINT8, pixels[i]))
                {
                    LOG_ERROR("Failed to write subimage#%zu", i);
                    return false;
                }
            }
            return out->close();
        }

        bool hdr::save(const acul::string &path, Params &hp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = hp.image.pixels;
            ::umbf::ImageFormat dst_format = {hp.format.format_types[2], 4};
            if (!is_image_equals(hp.image, dst_format, 3))
            {
                pixels = ::umbf::utils::convert_image(hp.image, dst_format, 3);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(hp.image.width, hp.image.height, 3, OIIO::TypeDesc::FLOAT);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::FLOAT, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool heif::save(const acul::string &path, Params &hp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = hp.image.pixels;
            ::umbf::ImageFormat dst_format = {hp.format.format_types[0], 1};
            size_t dst_channels = hp.image.channels.size() > 3 ? 4 : 3;
            if (!is_image_equals(hp.image, dst_format, dst_channels))
            {
                pixels = ::umbf::utils::convert_image(hp.image, dst_format, dst_channels);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(hp.image.width, hp.image.height, dst_channels, OIIO::TypeDesc::UINT8);
            auto comp_attr = acul::format("heic:%d", hp.compression);
            spec.attribute("Compression", comp_attr.c_str());
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, hp.image.pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool jpeg::save(const acul::string &path, Params &jp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = jp.image.pixels;
            ::umbf::ImageFormat dst_format = {jp.format.format_types[0], 1};
            if (!is_image_equals(jp.image, dst_format, 3))
            {
                pixels = ::umbf::utils::convert_image(jp.image, dst_format, 3);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(jp.image.width, jp.image.height, 3, OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", jp.dpi);
            spec.attribute("YResolution", jp.dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("Compression", "jpeg:" + std::to_string(jp.compression));
            if (jp.dither) spec.attribute("oiio:dither", 1);
            if (jp.progressive) spec.attribute("jpeg:progressive", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            spec.attribute("Software", jp.app_name.c_str());
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool openexr::save(const acul::string &path, Params &op, u8 dst_bit)
        {
            assert(dst_bit == 2 || dst_bit == 4);
            acul::vector<acul::unique_ptr<void>> tmp;
            acul::vector<void *> pixels;
            pixels.reserve(op.images.size());
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            acul::vector<OIIO::ImageSpec> specs;
            u32 max_width = 0, max_height = 0;
            for (const auto &image : op.images)
            {
                max_width = std::max(max_width, image.width);
                max_height = std::max(max_height, image.height);
            }
            ::umbf::ImageFormat dst_format = {op.format.format_types[dst_bit / 2], dst_bit};
            const OIIO::TypeDesc dst_type = umbf_format_to_oiio(dst_format);

            for (auto &image : op.images)
            {
                if (!is_image_equals(image, dst_format, image.channels.size()))
                {
                    pixels.emplace_back(::umbf::utils::convert_image(image, dst_format, image.channels.size()));
                    tmp.emplace_back(pixels.back());
                }
                else
                    pixels.emplace_back(image.pixels);
                OIIO::ImageSpec spec(image.width, image.height, image.channels.size(), dst_type);
                OIIO::ROI full_roi(0, max_width, 0, max_height);
                spec.full_x = full_roi.xbegin;
                spec.full_y = full_roi.ybegin;
                spec.full_width = full_roi.width();
                spec.full_height = full_roi.height();
                spec.attribute("compression", op.compression.c_str());
                specs.push_back(spec);
            }

            if (!out->open(pPath, specs.size(), specs.data()))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }

            for (size_t i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        LOG_ERROR("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(dst_type, pixels[i]))
                {
                    LOG_ERROR("Failed to write subimage#%zu", i);
                    return false;
                }
            }
            return out->close();
        }

        bool png::save(const acul::string &path, Params &pp, u8 dst_bit)
        {
            assert(dst_bit == 1 || dst_bit == 2);
            acul::unique_ptr<void> tmp;
            void *pixels = pp.image.pixels;
            const ::umbf::ImageFormat dst_format = {pp.format.format_types[dst_bit / 2], dst_bit};
            const OIIO::TypeDesc dst_type = umbf_format_to_oiio(dst_format);
            int dst_channels = pp.image.channels.size() > 3 ? 4 : 3;
            if (!is_image_equals(pp.image, dst_format, dst_channels))
            {
                pixels = ::umbf::utils::convert_image(pp.image, dst_format, dst_channels);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(pp.image.width, pp.image.height, dst_channels, dst_type);
            spec.attribute("XResolution", pp.dpi);
            spec.attribute("YResolution", pp.dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("png:compressionLevel", pp.compression);
            spec.attribute("png:filter", pp.filter);
            if (pp.dither) spec.attribute("oiio:dither", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (pp.unassociated_alpha) spec.attribute("oiio:UnassociatedAlpha", 1);
            if (!out->open(pPath, spec) || !out->write_image(dst_type, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool pnm::save(const acul::string &path, Params &pp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = pp.image.pixels;
            ::umbf::ImageFormat dst_format = {pp.format.format_types[0], 1};
            if (!is_image_equals(pp.image, dst_format, 3))
            {
                pixels = ::umbf::utils::convert_image(pp.image, dst_format, 3);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(pp.image.width, pp.image.height, 3, OIIO::TypeDesc::UINT8);
            if (pp.binary) spec.attribute("pnm:binary", 1);
            if (pp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool targa::save(const acul::string &path, Params &tp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = tp.image.pixels;
            ::umbf::ImageFormat dst_format = {tp.format.format_types[0], 1};
            size_t dst_channels = tp.image.channels.size() > 3 ? 4 : 3;
            if (!is_image_equals(tp.image, dst_format, dst_channels))
            {
                pixels = ::umbf::utils::convert_image(tp.image, dst_format, dst_channels);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(tp.image.width, tp.image.height, dst_channels, OIIO::TypeDesc::UINT8);
            spec.attribute("targa:compression", tp.compression.c_str());
            spec.attribute("targa:alpha_type", tp.alpha_type);
            spec.attribute("Software", tp.app_name.c_str());
            spec.attribute("oiio:BitsPerSample", 8);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (tp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool tiff::save(const acul::string &path, Params &tp, u8 dst_bit)
        {
            acul::vector<acul::unique_ptr<void>> tmp;
            acul::vector<void *> pixels;
            pixels.reserve(tp.images.size());
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            assert(out);
            acul::vector<OIIO::ImageSpec> specs;
            const ::umbf::ImageFormat dst_format = {tp.format.format_types[dst_bit / 2], dst_bit};
            const OIIO::TypeDesc dst_type = umbf_format_to_oiio(dst_format);

            for (auto &image : tp.images)
            {
                if (!is_image_equals(image, dst_format, image.channels.size()))
                {
                    pixels.emplace_back(::umbf::utils::convert_image(image, dst_format, image.channels.size()));
                    tmp.emplace_back(pixels.back());
                }
                else
                    pixels.emplace_back(image.pixels);
                OIIO::ImageSpec spec(image.width, image.height, image.channels.size(), dst_type);
                if (tp.dither) spec.attribute("oiio:dither", 1);
                if (tp.unassociated_alpha) spec.attribute("oiio:UnassociatedAlpha", 1);
                spec.attribute("tiff:zipquality", tp.zipquality);
                spec.attribute("tiff:bigtiff", tp.force_big_tiff);
                spec.attribute("XResolution", tp.dpi);
                spec.attribute("YResolution", tp.dpi);
                spec.attribute("ResolutionUnit", "in");
                spec.attribute("compression", tp.compression.c_str());
                specs.push_back(spec);
            }

            if (!out->open(pPath, specs.size(), specs.data()))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }

            for (size_t i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        LOG_ERROR("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(dst_type, pixels[i]))
                {
                    LOG_ERROR("Failed to write subimage#%zu", i);
                    return false;
                }
            }
            return out->close();
        }

        bool webp::save(const acul::string &path, Params &wp)
        {
            acul::unique_ptr<void> tmp;
            void *pixels = wp.image.pixels;
            ::umbf::ImageFormat dst_format = {wp.format.format_types[0], 1};
            if (!is_image_equals(wp.image, dst_format, 3))
            {
                pixels = ::umbf::utils::convert_image(wp.image, dst_format, wp.image.channels.size() > 3 ? 4 : 3);
                tmp = acul::unique_ptr<void>(pixels); // Release on call end
            }
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(wp.image.width, wp.image.height, wp.image.channels.size(), OIIO::TypeDesc::UINT8);
            if (wp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, pixels))
            {
                LOG_ERROR("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool umbf::save(const acul::string &path, Params &up)
        {
            ::umbf::File asset;
            asset.header.vendor_sign = UMBF_VENDOR_ID;
            asset.header.vendor_version = UMBF_VERSION;
            asset.header.spec_version = UMBF_VERSION;
            asset.header.type_sign = ::umbf::sign_block::format::image;
            asset.header.compressed = up.compression > 0;
            asset.blocks.push_back(acul::make_shared<::umbf::Image2D>(up.image));
            asset.checksum = up.checksum;
            return asset.save(path, up.compression);
        }
    } // namespace image
} // namespace aecl