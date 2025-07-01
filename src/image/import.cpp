#include <acul/hash/hashmap.hpp>
#include <acul/log.hpp>
#include <ecl/image/import.hpp>
#include <umbf/utils.hpp>

namespace ecl
{
    namespace image
    {
        bool OIIOLoader::load_image(
            const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> loadHandler,
            umbf::Image2D &info)
        {
            info.pixels = acul::mem_allocator<std::byte>::allocate(info.size());
            return loadHandler(inp, subimage, info.channel_count, info.pixels, info.size());
        }

        acul::io::file::op_state OIIOLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
        {
            auto inp = OIIO::ImageInput::open(path.c_str());
            if (!inp)
            {
                LOG_ERROR("OIIO: %s", OIIO::geterror().c_str());
                return acul::io::file::op_state::Error;
            }
            std::function<bool(const std::unique_ptr<OIIO::ImageInput> &inp, int, int, void *, vk::DeviceSize)>
                load_handler{nullptr};
            vk::Format image_format;
            for (int subimage{0}; inp->seek_subimage(subimage, 0); subimage++)
            {
                const OIIO::ImageSpec &spec = inp->spec();
                acul::vector<acul::string> channel_names(spec.channelnames.size());
                for (int i = 0; i < spec.nchannels; i++) channel_names[i] = spec.channelnames[i].c_str();
                umbf::Image2D info;
                info.width = spec.width;
                info.height = spec.height;
                info.channel_count = spec.nchannels;
                info.channel_names = channel_names;
                info.bytes_per_channel = spec.pixel_bytes() / spec.nchannels;
                info.pixels = nullptr;
                if (!load_handler)
                {
                    switch (info.bytes_per_channel)
                    {
                        case 1:
                            image_format = _format.bit8format;
                            break;
                        case 2:
                            image_format = _format.bit16format;
                            break;
                        case 4:
                            image_format = _format.bit32format;
                            break;
                        default:
                            break;
                    }
                    if (image_format == vk::Format::eUndefined)
                    {
                        LOG_ERROR("Unsupported image format");
                        return acul::io::file::op_state::Error;
                    }
                    OIIO::TypeDesc dst_type = vk_format_to_OIIO(image_format);
                    load_handler = [dst_type](const std::unique_ptr<OIIO::ImageInput> &inp, int subimage, int channels,
                                              void *dst, vk::DeviceSize size) {
                        if (!inp->read_image(subimage, 0, 0, channels, dst_type, dst)) return false;
                        return true;
                    };
                }
                info.format = image_format;
                load_image(inp, subimage, load_handler, info);
                images.push_back(info);
            }
            inp->close();
            return images.empty() ? acul::io::file::op_state::Error : acul::io::file::op_state::Success;
        }

        acul::io::file::op_state UMBFLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
        {
            auto asset = umbf::File::read_from_disk(path);
            if (!asset) return acul::io::file::op_state::Error;
            _checksum = asset->checksum;
            if (asset->blocks.empty() || asset->blocks.front()->signature() != umbf::sign_block::Image2D)
            {
                LOG_WARN("ECL UMBF Loader can recognize only 2D images");
                return acul::io::file::op_state::Error;
            }
            auto image = acul::static_pointer_cast<umbf::Image2D>(asset->blocks.front());
            images.push_back(*image);
            return acul::io::file::op_state::Success;
        }

        ILoader *get_importer_by_path(const acul::string &path)
        {
            switch (get_type_by_extension(acul::io::get_extension(path)))
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
                case Type::OpenEXR:
                    return acul::alloc<OpenEXRLoader>();
                case Type::PNG:
                    return acul::alloc<PNGLoader>();
                case Type::PBM:
                    return acul::alloc<PBMLoader>();
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