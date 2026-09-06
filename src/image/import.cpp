#include <acul/functional/unique_function.hpp>
#include <acul/hash/hashmap.hpp>
#include <acul/io/fs/path.hpp>
#include <aecl/image/import.hpp>
#include <inttypes.h>
#include <umbf/ext/image/utils.hpp>

namespace aecl::image
{
    bool OIIOLoader::load_image(
        const std::unique_ptr<OIIO::ImageInput> &inp, int subimage,
        acul::unique_function<bool(const std::unique_ptr<OIIO::ImageInput> &, int, int, void *, size_t)> load_handler,
        umbf::Image2D &info)
    {
        info.pixels = acul::mem_allocator<std::byte>::allocate(info.size());
        return load_handler(inp, subimage, info.channels.size(), info.pixels, info.size());
    }

    bool OIIOLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
    {
        auto inp = OIIO::ImageInput::open(path.c_str());
        if (!inp)
        {
            _error = OIIO::geterror().c_str();
            return false;
        }
        acul::unique_function<bool(const std::unique_ptr<OIIO::ImageInput> &inp, int, int, void *, size_t)>
            load_handler{nullptr};
        umbf::ImageFormat image_format;
        for (int subimage{0}; inp->seek_subimage(subimage, 0); subimage++)
        {
            const OIIO::ImageSpec &spec = inp->spec();
            acul::vector<acul::string> channel_names(spec.channelnames.size());
            for (int i = 0; i < spec.nchannels; i++) channel_names[i] = spec.channelnames[i].c_str();
            umbf::Image2D info;
            info.width = spec.width;
            info.height = spec.height;
            info.channels = channel_names;
            image_format.bytes_per_channel = spec.pixel_bytes() / spec.nchannels;
            assert(image_format.bytes_per_channel <= 4);
            image_format.type = _format.format_types[image_format.bytes_per_channel / 2];
            info.pixels = nullptr;
            if (!load_handler)
            {
                if (image_format.type == umbf::ImageFormat::Type::none)
                {
                    _error = "Unsupported image format";
                    return false;
                }
                OIIO::TypeDesc dst_type = umbf_format_to_oiio(image_format);
                load_handler = [dst_type](const std::unique_ptr<OIIO::ImageInput> &inp, int subimage, int channels,
                                          void *dst, size_t size) {
                    if (!inp->read_image(subimage, 0, 0, channels, dst_type, dst)) return false;
                    return true;
                };
            }
            info.format = image_format;
            load_image(inp, subimage, std::move(load_handler), info);
            images.push_back(info);
        }
        inp->close();
        return !images.empty();
    }

    bool UMBFLoader::load(const acul::string &path, acul::vector<umbf::Image2D> &images)
    {
        umbf::ReadDescriptor asset;
        auto res = umbf::create_read_descriptor(path, asset);
        if (!res.success())
        {
            _error = acul::format("Failed to load file. Error code: 0x%016" PRIx64, static_cast<u64>(res));
            return false;
        }
        _checksum = asset.file->checksum;
        acul::unique_ptr<umbf::Image2D> image;
        for (auto block = asset.begin(); block != asset.end(); ++block)
        {
            if (block->signature != umbf::sign_block::image) continue;
            auto value = umbf::get_block(block);
            if (value && value->signature() == umbf::sign_block::image)
                image = acul::unique_ptr<umbf::Image2D>(static_cast<umbf::Image2D *>(value.release()));
            break;
        }
        if (!image)
        {
            _error = "Provided file is not an image";
            umbf::close_read_descriptor(asset);
            return false;
        }
        images.push_back(std::move(*image));
        umbf::close_read_descriptor(asset);
        return true;
    }

    ILoader *get_importer_by_path(const acul::string &path)
    {
        switch (get_type_by_extension(acul::fs::get_extension(path)))
        {
            case Type::bmp:
                return acul::alloc<BMPLoader>();
            case Type::gif:
                return acul::alloc<GIFLoader>();
            case Type::hdr:
                return acul::alloc<HDRLoader>();
            case Type::heif:
                return acul::alloc<HEIFLoader>();
            case Type::jpeg:
                return acul::alloc<JPEGLoader>();
            case Type::openexr:
                return acul::alloc<OpenEXRLoader>();
            case Type::png:
                return acul::alloc<PNGLoader>();
            case Type::pbm:
                return acul::alloc<PBMLoader>();
            case Type::targa:
                return acul::alloc<TargaLoader>();
            case Type::tiff:
                return acul::alloc<TIFFLoader>();
            case Type::webp:
                return acul::alloc<WebPLoader>();
            case Type::umbf:
                return acul::alloc<UMBFLoader>();
            default:
                return nullptr;
        }
    }
} // namespace aecl::image
