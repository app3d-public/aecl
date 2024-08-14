#include <assets/utils.hpp>
#include <core/hash.hpp>
#include <core/log.hpp>
#include <ecl/image/import.hpp>
#include <oneapi/tbb/scalable_allocator.h>
#include "core/io/file.hpp"

namespace ecl
{
    namespace image
    {
        bool OIIOLoader::loadImage(
            const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> loadHandler,
            assets::ImageInfo &info)
        {
            info.pixels = scalable_malloc(info.imageSize());
            return loadHandler(inp, subimage, info.channelCount, info.pixels, info.imageSize());
        }

        io::file::ReadState OIIOLoader::load(const std::filesystem::path &path, DArray<assets::ImageInfo> &images)
        {
            auto inp = OIIO::ImageInput::open(path.string());
            if (!inp)
            {
                logError("OIIO: %s", OIIO::geterror().c_str());
                return io::file::ReadState::Error;
            }
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &inp, int, int, void *, vk::DeviceSize)>
                loadHandler{nullptr};
            vk::Format imageFormat;
            for (int subimage{0}; inp->seek_subimage(subimage, 0); subimage++)
            {
                const OIIO::ImageSpec &spec = inp->spec();
                DArray<std::string> channelNames(spec.channelnames.size());
                for (size_t i = 0; i < spec.nchannels; i++) channelNames[i] = spec.channelnames[i];
                assets::ImageInfo info{static_cast<u16>(spec.width),
                                       static_cast<u16>(spec.height),
                                       spec.nchannels,
                                       channelNames,
                                       static_cast<u8>(spec.pixel_bytes() / spec.nchannels),
                                       nullptr};
                if (!loadHandler)
                {
                    switch (info.bytesPerChannel)
                    {
                        case 1:
                            imageFormat = _format.bit8format;
                            break;
                        case 2:
                            imageFormat = _format.bit16format;
                            break;
                        case 4:
                            imageFormat = _format.bit32format;
                            break;
                        default:
                            break;
                    }
                    if (imageFormat == vk::Format::eUndefined)
                    {
                        logError("Unsupported image format");
                        return io::file::ReadState::Error;
                    }
                    OIIO::TypeDesc dstType = vkFormatToOIIO(imageFormat);
                    loadHandler = [dstType](const std::unique_ptr<OIIO::ImageInput> &inp, int subimage, int channels,
                                            void *dst, vk::DeviceSize size) {
                        if (!inp->read_image(subimage, 0, 0, channels, dstType, dst)) return false;
                        return true;
                    };
                }
                info.imageFormat = imageFormat;
                loadImage(inp, subimage, loadHandler, info);
                images.push_back(info);
            }
            inp->close();
            return images.empty() ? io::file::ReadState::Error : io::file::ReadState::Success;
        }

        io::file::ReadState AssetLoader::load(const std::filesystem::path &path, DArray<assets::ImageInfo> &images)
        {
            auto asset = assets::Image::readFromFile(path);
            if (!asset) return io::file::ReadState::Error;
            _checksum = asset->checksum();
            auto textureInfo = std::dynamic_pointer_cast<assets::Image2D>(asset->stream());
            if (!textureInfo) return io::file::ReadState::Error;
            images.push_back(*textureInfo);
            return io::file::ReadState::Success;
        }

        ILoader *getImporterByPath(const std::filesystem::path &path)
        {
            switch (getTypeByExt(path.extension().string()))
            {
                case Type::BMP:
                    return new BMPLoader();
                case Type::GIF:
                    return new GIFLoader();
                case Type::HDR:
                    return new HDRLoader();
                case Type::HEIF:
                    return new HEIFLoader();
                case Type::JPEG:
                    return new JPEGLoader();
                case Type::JPEG2000:
                    return new JPEG2000Loader();
                case Type::JPEGXL:
                    return new JPEGXLLoader();
                case Type::OpenEXR:
                    return new OpenEXRLoader();
                case Type::PNG:
                    return new PNGLoader();
                case Type::PBM:
                    return new PBMLoader();
                case Type::RAW:
                    return new RAWLoader();
                case Type::Targa:
                    return new TargaLoader();
                case Type::TIFF:
                    return new TIFFLoader();
                case Type::WebP:
                    return new WebPLoader();
                case Type::Asset:
                    return new AssetLoader();
                default:
                    return nullptr;
            }
        }
    } // namespace image
} // namespace ecl