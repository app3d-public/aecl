#include <assets/utils.hpp>
#include <core/log.hpp>
#include <core/std/hash.hpp>
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
            assets::Image2D &info)
        {
            info.pixels = scalable_malloc(info.imageSize());
            return loadHandler(inp, subimage, info.channelCount, info.pixels, info.imageSize());
        }

        io::file::ReadState OIIOLoader::load(const std::filesystem::path &path, astl::vector<assets::Image2D> &images)
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
                astl::vector<std::string> channelNames(spec.channelnames.size());
                for (size_t i = 0; i < spec.nchannels; i++) channelNames[i] = spec.channelnames[i];
                assets::Image2D info;
                info.width = spec.width;
                info.height = spec.height;
                info.channelCount = spec.nchannels;
                info.channelNames = channelNames;
                info.bytesPerChannel = spec.pixel_bytes() / spec.nchannels;
                info.pixels = nullptr;
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

        io::file::ReadState AssetLoader::load(const std::filesystem::path &path, astl::vector<assets::Image2D> &images)
        {
            auto asset = assets::Asset::readFromFile(path);
            if (!asset) return io::file::ReadState::Error;
            _checksum = asset->checksum;
            if (asset->blocks.empty() || asset->blocks.front()->signature() != assets::sign_block::image2D)
            {
                logWarn("ECL Asset Loader can recognize only 2D images");
                return io::file::ReadState::Error;
            }
            auto image = astl::static_pointer_cast<assets::Image2D>(asset->blocks.front());
            images.push_back(*image);
            return io::file::ReadState::Success;
        }

        ILoader *getImporterByPath(const std::filesystem::path &path)
        {
            switch (getTypeByExt(path.extension().string()))
            {
                case Type::BMP:
                    return astl::alloc<BMPLoader>();
                case Type::GIF:
                    return astl::alloc<GIFLoader>();
                case Type::HDR:
                    return astl::alloc<HDRLoader>();
                case Type::HEIF:
                    return astl::alloc<HEIFLoader>();
                case Type::JPEG:
                    return astl::alloc<JPEGLoader>();
                case Type::JPEG2000:
                    return astl::alloc<JPEG2000Loader>();
                case Type::JPEGXL:
                    return astl::alloc<JPEGXLLoader>();
                case Type::OpenEXR:
                    return astl::alloc<OpenEXRLoader>();
                case Type::PNG:
                    return astl::alloc<PNGLoader>();
                case Type::PBM:
                    return astl::alloc<PBMLoader>();
                case Type::RAW:
                    return astl::alloc<RAWLoader>();
                case Type::Targa:
                    return astl::alloc<TargaLoader>();
                case Type::TIFF:
                    return astl::alloc<TIFFLoader>();
                case Type::WebP:
                    return astl::alloc<WebPLoader>();
                case Type::Asset:
                    return astl::alloc<AssetLoader>();
                default:
                    return nullptr;
            }
        }
    } // namespace image
} // namespace ecl