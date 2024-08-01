#include <assets/utils.hpp>
#include <core/log.hpp>
#include <ecl/image/export.hpp>

namespace ecl
{
    namespace image
    {
        std::shared_ptr<void> copySrcBuffer(const void *src, size_t size, vk::Format format)
        {
            switch (format)
            {
                case vk::Format::eR8G8B8A8Srgb:
                {
                    std::shared_ptr<u8> copy(new u8[size], std::default_delete<u8[]>());
                    std::memcpy(copy.get(), src, size * sizeof(u8));
                    return copy;
                }
                case vk::Format::eR16G16B16A16Uint:
                {
                    std::shared_ptr<u16> copy(new u16[size], std::default_delete<u16[]>());
                    std::memcpy(copy.get(), src, size * sizeof(u16));
                    return copy;
                }
                case vk::Format::eR32G32B32A32Uint:
                {
                    std::shared_ptr<u32> copy(new u32[size], std::default_delete<u32[]>());
                    std::memcpy(copy.get(), src, size * sizeof(u32));
                    return copy;
                }
                case vk::Format::eR16G16B16A16Sfloat:
                case vk::Format::eR32G32B32A32Sfloat:
                {
                    std::shared_ptr<float> copy(new float[size], std::default_delete<float[]>());
                    std::memcpy(copy.get(), src, size * sizeof(float));
                    return copy;
                }
                default:
                    return nullptr;
            }
        }

        bool BMPExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, image.channelCount > 3 ? 4 : 3);
            pixels.emplace_back(copy);

            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, image.channelCount, OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", _dpi);
            spec.attribute("YResolution", _dpi);
            if (_dither) spec.attribute("oiio:dither", 1);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool GIFExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(_images.size());
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            DArray<OIIO::ImageSpec> specs;

            for (assets::ImageInfo image : _images)
            {
                std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();

                if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                    assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
                pixels.emplace_back(copy);
                OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
                if (_interlacing) spec.attribute("gif:Interlacing", 1);
                if (_images.size() > 1)
                {
                    spec.attribute("oiio:LoopCount", _loops);
                    spec.attribute("oiio:Movie", 1);
                    spec.attribute("gif:FPS", _fps);
                }
                specs.push_back(spec);
            }

            if (!out->open(_path, specs.size(), specs.data()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(_path, specs[i], OIIO::ImageOutput::AppendSubimage))
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

        bool HDRExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 4);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR32G32B32A32Sfloat))
                assets::utils::convertImage(image, vk::Format::eR32G32B32A32Sfloat, 3);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::FLOAT);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::FLOAT, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool HEIFExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            assets::ImageInfo image = _images[0];
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            pixels.resize(1);
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, dstChannels);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, OIIO::TypeDesc::UINT8);
            spec.attribute("Compression", "heic:" + std::to_string(_compression));
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool JPEGExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
            spec.attribute("XResolution", _dpi);
            spec.attribute("YResolution", _dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("Compression", "jpeg:" + std::to_string(_compression));
            if (_dither) spec.attribute("oiio:dither", 1);
            if (_progressive) spec.attribute("jpeg:progressive", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            spec.attribute("Software", _appName);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool JPEG2000Exporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1 || dstBit == 2);
            assets::ImageInfo image = _images[0];
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            const vk::Format dstFormat = getFormatByBit(dstBit, _format);
            pixels.resize(1);
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, dstFormat)) assets::utils::convertImage(image, dstFormat, dstChannels);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            if (_unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
            if (_dither) spec.attribute("oiio:dither", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (!out->open(_path, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool JPEGXLExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1 || dstBit == 2);
            assets::ImageInfo image = _images[0];
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            const vk::Format dstFormat = getFormatByBit(dstBit, _format);
            pixels.resize(1);
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, dstFormat)) assets::utils::convertImage(image, dstFormat, dstChannels);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (!out->open(_path, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool OpenEXRExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 2 || dstBit == 4);
            pixels.resize(_images.size());
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            DArray<OIIO::ImageSpec> specs;
            const vk::Format dstFormat = getFormatByBit(dstBit, _format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            u16 maxWidth = 0, maxHeight = 0;
            for (const auto &image : _images)
            {
                maxWidth = std::max(maxWidth, image.width);
                maxHeight = std::max(maxHeight, image.height);
            }

            for (assets::ImageInfo image : _images)
            {
                std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();
                if (!isImageEquals(image, _format, dstFormat))
                    assets::utils::convertImage(image, dstFormat, image.channelCount);
                pixels.emplace_back(copy);
                OIIO::ImageSpec spec(image.width, image.height, image.channelCount, dstType);
                OIIO::ROI fullROI(0, maxWidth, 0, maxHeight);
                spec.full_x = fullROI.xbegin;
                spec.full_y = fullROI.ybegin;
                spec.full_width = fullROI.width();
                spec.full_height = fullROI.height();
                spec.attribute("compression", _compression);
                specs.push_back(spec);
            }

            if (!out->open(_path, specs.size(), specs.data()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(_path, specs[i], OIIO::ImageOutput::AppendSubimage))
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

        bool PNGExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1 || dstBit == 2);
            pixels.resize(1);
            const vk::Format dstFormat = getFormatByBit(dstBit, _format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);
            assets::ImageInfo image = _images[0];
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, dstFormat)) assets::utils::convertImage(image, dstFormat, dstChannels);
            pixels.emplace_back(copy);

            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, dstType);
            spec.attribute("XResolution", _dpi);
            spec.attribute("YResolution", _dpi);
            spec.attribute("ResolutionUnit", "in");
            spec.attribute("png:compressionLevel", _compression);
            spec.attribute("png:filter", _filter);
            if (_dither) spec.attribute("oiio:dither", 1);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (_unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
            assert(pixels.size() > 0);
            if (!out->open(_path, spec) || !out->write_image(dstType, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool PNMExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, 3);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, 3, OIIO::TypeDesc::UINT8);
            if (_binary) spec.attribute("pnm:binary", 1);
            if (_dither) spec.attribute("oiio:dither", 1);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool TargaExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            int dstChannels = image.channelCount > 3 ? 4 : 3;
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, dstChannels);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, dstChannels, OIIO::TypeDesc::UINT8);
            spec.attribute("targa:compression", _compression);
            spec.attribute("Software", _appName);
            spec.attribute("oiio:ColorSpace", "sRGB");
            if (_dither) spec.attribute("oiio:dither", 1);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }

        bool TIFFExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            DArray<OIIO::ImageSpec> specs;
            pixels.resize(_images.size());
            const vk::Format dstFormat = getFormatByBit(dstBit, _format);
            const OIIO::TypeDesc dstType = vkFormatToOIIO(dstFormat);

            for (assets::ImageInfo image : _images)
            {
                std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
                image.pixels = copy.get();
                if (!isImageEquals(image, _format, dstFormat))
                    assets::utils::convertImage(image, dstFormat, image.channelCount);
                pixels.emplace_back(copy);
                OIIO::ImageSpec spec(image.width, image.height, image.channelCount, dstType);
                if (_dither) spec.attribute("oiio:dither", 1);
                if (_unassociatedAlpha) spec.attribute("oiio:UnassociatedAlpha", 1);
                spec.attribute("Software", _appName);
                spec.attribute("tiff:zipquality", _zipquality);
                spec.attribute("tiff:bigtiff", _forceBigTIFF);
                spec.attribute("XResolution", _dpi);
                spec.attribute("YResolution", _dpi);
                spec.attribute("ResolutionUnit", "in");
                spec.attribute("compression", _compression);
                specs.push_back(spec);
            }

            if (!out->open(_path, specs.size(), specs.data()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }

            for (int i{0}; i < pixels.size(); i++)
            {
                if (i > 0)
                {
                    if (!out->open(_path, specs[i], OIIO::ImageOutput::AppendSubimage))
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

        bool WebPExporter::save(size_t dstBit, DArray<std::shared_ptr<void>> &pixels)
        {
            assert(dstBit == 1);
            pixels.resize(1);
            assets::ImageInfo image = _images[0];
            std::shared_ptr<void> copy = copySrcBuffer(image.pixels, image.imageSize(), image.imageFormat);
            image.pixels = copy.get();
            if (!isImageEquals(image, _format, vk::Format::eR8G8B8A8Srgb))
                assets::utils::convertImage(image, vk::Format::eR8G8B8A8Srgb, image.channelCount > 3 ? 4 : 3);
            pixels.emplace_back(copy);
            std::unique_ptr<OIIO::ImageOutput> out = OIIO::ImageOutput::create(_path.string());
            OIIO::ImageSpec spec(image.width, image.height, image.channelCount, OIIO::TypeDesc::UINT8);
            if (_dither) spec.attribute("oiio:dither", 1);
            if (!out->open(_path, spec) || !out->write_image(OIIO::TypeDesc::UINT8, copy.get()))
            {
                logError("%s", out->geterror().c_str());
                return false;
            }
            return out->close();
        }
    } // namespace image
} // namespace ecl