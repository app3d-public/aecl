#include <acul/hash/hashmap.hpp>
#include <acul/log.hpp>
#include <ecl/image/import.hpp>
#include <umbf/utils.hpp>

namespace ecl
{
    namespace image
    {
        bool OIIOLoader::loadImage(
            const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> loadHandler,
            umbf::Image2D &info)
        {
            info.pixels = acul::mem_allocator<std::byte>::allocate(info.imageSize());
            return loadHandler(inp, subimage, info.channelCount, info.pixels, info.imageSize());
        }

        acul::io::file::op_state OIIOLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
        {
            auto inp = OIIO::ImageInput::open(path.c_str());
            if (!inp)
            {
                logError("OIIO: %s", OIIO::geterror().c_str());
                return acul::io::file::op_state::error;
            }
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &inp, int, int, void *, vk::DeviceSize)>
                loadHandler{nullptr};
            vk::Format imageFormat;
            for (int subimage{0}; inp->seek_subimage(subimage, 0); subimage++)
            {
                const OIIO::ImageSpec &spec = inp->spec();
                acul::vector<acul::string> channelNames(spec.channelnames.size());
                for (size_t i = 0; i < spec.nchannels; i++) channelNames[i] = spec.channelnames[i].c_str();
                umbf::Image2D info;
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
                        return acul::io::file::op_state::error;
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
            return images.empty() ? acul::io::file::op_state::error : acul::io::file::op_state::success;
        }

        acul::io::file::op_state UMBFLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
        {
            auto asset = umbf::File::readFromDisk(path);
            if (!asset) return acul::io::file::op_state::error;
            _checksum = asset->checksum;
            if (asset->blocks.empty() || asset->blocks.front()->signature() != umbf::sign_block::meta::image2D)
            {
                logWarn("ECL UMBF Loader can recognize only 2D images");
                return acul::io::file::op_state::error;
            }
            auto image = acul::static_pointer_cast<umbf::Image2D>(asset->blocks.front());
            images.push_back(*image);
            return acul::io::file::op_state::success;
        }

        ILoader *getImporterByPath(const acul::string &path)
        {
            switch (getTypeByExt(acul::io::get_extension(path)))
            {
                case Type::BMP:
                    return acul::alloc<BMPLoader>();
                case Type::GIF:
                    return acul::alloc<GIFLoader>();
                case Type::HDR:
                    return acul::alloc<HDRLoader>();
                case Type::HEIF:
                    return acul::alloc<HEIFLoader>();
                case Type::JPEG:
                    return acul::alloc<JPEGLoader>();
                case Type::JPEG2000:
                    return acul::alloc<JPEG2000Loader>();
                case Type::JPEGXL:
                    return acul::alloc<JPEGXLLoader>();
                case Type::OpenEXR:
                    return acul::alloc<OpenEXRLoader>();
                case Type::PNG:
                    return acul::alloc<PNGLoader>();
                case Type::PBM:
                    return acul::alloc<PBMLoader>();
                case Type::RAW:
                    return acul::alloc<RAWLoader>();
                case Type::Targa:
                    return acul::alloc<TargaLoader>();
                case Type::TIFF:
                    return acul::alloc<TIFFLoader>();
                case Type::WebP:
                    return acul::alloc<WebPLoader>();
                case Type::UMBF:
                    return acul::alloc<UMBFLoader>();
                default:
                    return nullptr;
            }
        }
    } // namespace image
} // namespace ecl