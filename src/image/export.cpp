#include <acul/log.hpp>
#include <assets/utils.hpp>
#include <ecl/image/export.hpp>
#include <umbf/version.h>

namespace ecl
{
    namespace image
    {
        using ::umbf::utils::convertImage;

        acul::shared_ptr<void> copySrcBuffer(const void *src, size_t size, vk::Format format)
        {
            switch (format)
            {
                case vk::Format::eR8G8B8A8Srgb:
                {
                    acul::shared_ptr<u8[]> copy(size);
                    memcpy(copy.get(), src, size * sizeof(u8));
                    return copy;
                }
                case vk::Format::eR16G16B16A16Uint:
                {
                    acul::shared_ptr<u16[]> copy(size);
                    memcpy(copy.get(), src, size * sizeof(u16));
                    return copy;
                }
                case vk::Format::eR32G32B32A32Uint:
                {
                    acul::shared_ptr<u32[]> copy(size);
                    memcpy(copy.get(), src, size * sizeof(u32));
                    return copy;
                }
                case vk::Format::eR16G16B16A16Sfloat:
                case vk::Format::eR32G32B32A32Sfloat:
                {
                    acul::shared_ptr<float[]> copy(size);
                    memcpy(copy.get(), src, size * sizeof(float));
                    return copy;
                }
                default:
                    return nullptr;
            }
        }

        bool bmp::save(const acul::string &path, Params &bp)
        {
            auto &image = bp.image;
            auto copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, bp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, image.channelCount > 3 ? 4 : 3);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, image.channelCount, OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", bp.dpi);
            spec.attribute("YResolution", bp.dpi);
            if (bp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool gif::save(const acul::string &path, Params &gp)
        {
            acul::vector<acul::shared_ptr<void>> pixels(gp.images.size());
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            acul::vector<OIIO::ImageSpec> specs;

            for (auto &image : gp.images)
            {
                acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();

                if (!isImageEquals(image, gp.format, vk::Format::eR8G8B8A8Srgb))
                    convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
                pixels.emplace_back(copy);
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
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        logError("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(OIIO::TypeDesc::UINT8, pixels[i].get()))
                {
                    logError("Failed to write subimage#%d", i);
                    return false;
                }
            }
            return out->close();
        }

        bool hdr::save(const acul::string &path, Params &hp)
        {
            acul::shared_ptr<void> copy = copySrcBuffer(hp.image.pixels, hp.image.imageSize(), hp.image.imageFormat);
            hp.image.pixels = copy.get();
            if (!isImageEquals(hp.image, hp.format, vk::Format::eR32G32B32A32Sfloat))
                convertImage(hp.image, vk::Format::eR32G32B32A32Sfloat, 3);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(path.c_str());
            OIIO::ImageSpec spec(hp.image.width, hp.image.height, 3, OIIO::TypeDesc::FLOAT);
            if (!out->open(path.c_str(), spec) || !out->write_image(OIIO::TypeDesc::FLOAT, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool heif::save(const acul::string &path, Params &hp)
        {
            umbf::Image2D image = hp.image;
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, hp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, dstChannels);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, OIIO::TypeDesc::UINT8);
            auto comp_attr = acul::format("heic:%d", hp.compression);
            spec.attribute("Compression", comp_attr.c_str());
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool jpeg::save(const acul::string &path, Params &jp)
        {
            umbf::Image2D image = jp.image;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, jp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", jp.dpi);
            spec.attribute("YResolution", jp.dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("Compression", "jpeg:" + std::to_string(jp.compression));
            if (jp.dither) spec.attribute("oiio:dither", 1);
            if (jp.progressive) spec.attribute("jpeg:progressive", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            spec.attribute("Software", jp.appName.c_str());
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool jpeg2000::save(const acul::string &path, Params &jp, u8 dstBit)
        {
            assert(dstBit == 1 || dstBit == 2);
            umbf::Image2D image = jp.image;
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            const vk::Format dstFormat = getFormatByBit(dstBit, jp.format);
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, jp.format, dstFormat)) convertImage(image, dstFormat, dstChannels);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            if (jp.unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
            if (jp.dither) spec.attribute("oiio:dither", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (!out->open(pPath, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool jpegXL::save(const acul::string &path, Params &jp, u8 dstBit)
        {
            assert(dstBit == 1 || dstBit == 2);
            umbf::Image2D image = jp.image;
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            const vk::Format dstFormat = getFormatByBit(dstBit, jp.format);
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, jp.format, dstFormat)) convertImage(image, dstFormat, dstChannels);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (!out->open(pPath, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool openEXR::save(const acul::string &path, Params &op, u8 dstBit)
        {
            assert(dstBit == 2 || dstBit == 4);
            acul::vector<acul::shared_ptr<void>> pixels(op.images.size());
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            acul::vector<OIIO::ImageSpec> specs;
            const vk::Format dstFormat = getFormatByBit(dstBit, op.format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            u16 maxWidth = 0, maxHeight = 0;
            for (const auto &image : op.images)
            {
                maxWidth = std::max(maxWidth, image.width);
                maxHeight = std::max(maxHeight, image.height);
            }

            for (umbf::Image2D image : op.images)
            {
                acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();
                if (!isImageEquals(image, op.format, dstFormat)) convertImage(image, dstFormat, image.channelCount);
                pixels.emplace_back(copy);
                OIIO::ImageSpec spec(image.width, image.height, image.channelCount, dstType);
                OIIO::ROI fullROI(0, maxWidth, 0, maxHeight);
                spec.full_x = fullROI.xbegin;
                spec.full_y = fullROI.ybegin;
                spec.full_width = fullROI.width();
                spec.full_height = fullROI.height();
                spec.attribute("compression", op.compression.c_str());
                specs.push_back(spec);
            }

            if (!out->open(pPath, specs.size(), specs.data()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        logError("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(dstType, pixels[i].get()))
                {
                    logError("Failed to write subimage#%d", i);
                    return false;
                }
            }
            return out->close();
        }

        bool png::save(const acul::string &path, Params &pp, u8 dstBit)
        {
            assert(dstBit == 1 || dstBit == 2);
            const vk::Format dstFormat = getFormatByBit(dstBit, pp.format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            umbf::Image2D image = pp.image;
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, pp.format, dstFormat)) convertImage(image, dstFormat, dstChannels);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            spec.attribute("XResolution", pp.dpi);
            spec.attribute("YResolution", pp.dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("png:compressionLevel", pp.compression);
            spec.attribute("png:filter", pp.filter);
            if (pp.dither) spec.attribute("oiio:dither", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (pp.unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
            assert(image.pixels);
            if (!out->open(pPath, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool pnm::save(const acul::string &path, Params &pp)
        {
            umbf::Image2D image = pp.image;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, pp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
            if (pp.binary) spec.attribute("pnm:binary", 1);
            if (pp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool targa::save(const acul::string &path, Params &tp, u8 dstBit)
        {
            assert(dstBit == 1);
            umbf::Image2D image = tp.image;
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, tp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, dstChannels);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, OIIO::TypeDesc::UINT8);
            spec.attribute("targa:compression", tp.compression.c_str());
            spec.attribute("targa:alpha_type", tp.alphaType);
            spec.attribute("Software", tp.appName.c_str());
            spec.attribute("oiio:BitsPerSample", dstBit * 8);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (tp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool tiff::save(const acul::string &path, Params &tp, u8 dstBit)
        {
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            acul::vector<OIIO::ImageSpec> specs;
            acul::vector<acul::shared_ptr<void>> pixels(tp.images.size());
            const vk::Format dstFormat = getFormatByBit(dstBit, tp.format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);

            for (umbf::Image2D image : tp.images)
            {
                acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();
                if (!isImageEquals(image, tp.format, dstFormat)) convertImage(image, dstFormat, image.channelCount);
                pixels.emplace_back(copy);
                OIIO::ImageSpec spec(image.width, image.height, image.channelCount, dstType);
                if (tp.dither) spec.attribute("oiio:dither", 1);
                if (tp.unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
                spec.attribute("tiff:zipquality", tp.zipquality);
                spec.attribute("tiff:bigtiff", tp.forceBigTIFF);
                spec.attribute("XResolution", tp.dpi);
                spec.attribute("YResolution", tp.dpi);
                spec.attribute("ResolutionUnit", "in");
                spec.attribute("compression", tp.compression.c_str());
                specs.push_back(spec);
            }

            if (!out->open(pPath, specs.size(), specs.data()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(pPath, specs[i], OIIO::ImageOutput::AppendSubimage))
                    {
                        logError("%s", out->geterror().c_str());
                        return false;
                    }
                }
                if (!out->write_image(dstType, pixels[i].get()))
                {
                    logError("Failed to write subimage#%d", i);
                    return false;
                }
            }
            return out->close();
        }

        bool webp::save(const acul::string &path, Params &wp)
        {
            umbf::Image2D image = wp.image;
            acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, wp.format, vk::Format::eR8G8B8A8Srgb))
                convertImage(image, vk::Format::eR8G8B8A8Srgb, image.channelCount > 3 ? 4 : 3);
            auto *pPath = path.c_str();
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(pPath);
            OIIO::ImageSpec spec(image.width, image.height, image.channelCount, OIIO::TypeDesc::UINT8);
            if (wp.dither) spec.attribute("oiio:dither", 1);
            if (!out->open(pPath, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool umbf::save(const acul::string &path, Params &up, u8 dstBit)
        {
            auto &image = up.image;
            if (image.bytesPerChannel != dstBit)
            {
                acul::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();
                convertImage(image, image.imageFormat, dstBit);
            }
            umbf::File asset;
            asset.header.vendor_sign = UMBF_VENDOR_ID;
            asset.header.vendor_version = UMBF_VERSION;
            asset.header.spec_version = UMBF_VERSION;
            asset.header.type_sign = ::umbf::sign_block::format::image;
            asset.header.compressed = up.compression > 0;
            asset.blocks.push_back(acul::make_shared<umbf::Image2D>(image));
            asset.checksum = up.checksum;
            return asset.save(path, up.compression);
        }
    } // namespace image
} // namespace ecl