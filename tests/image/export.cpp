#include <aecl/image/export.hpp>
#include <aecl/image/import.hpp>
#include "../env.hpp"

using namespace aecl;

void test_image_export()
{
    test_environment env;
    create_test_environment(env);

    auto p = acul::path(env.data_dir) / "image.umia";
    auto loader = aecl::image::get_importer_by_path(p);
    assert(loader);
    acul::vector<umbf::Image2D> images;
    assert(loader->load(p, images));
    assert(!images.empty());
    auto &inp = images.front();

    acul::path op = env.output_dir;
    using namespace aecl::image;

    // BMP
    bmp::Params bmpp(inp, 72.0f, false);
    assert(bmp::save(op / "image_export.bmp", bmpp));

    // GIF
    gif::Params gifp(images);
    assert(gif::save(op / "image_export.gif", gifp));

    // HDR
    hdr::Params hdrp(inp);
    assert(hdr::save(op / "image_export.hdr", hdrp));

    // HEIF
    heif::Params heifp(inp);
    assert(heif::save(op / "image_export.heif", heifp));

    // JPEG
    jpeg::Params jpegp(inp);
    assert(jpeg::save(op / "image_export.jpg", jpegp));

    // OpenEXR
    openexr::Params openEXRp(images);
    assert(openexr::save(op / "image_export.exr", openEXRp, 2));

    // PNG
    png::Params pngp(inp);
    assert(png::save(op / "image_export.png", pngp, 1));

    // PNM
    pnm::Params pnmp(inp);
    assert(pnm::save(op / "image_export.ppm", pnmp));

    // Targa
    targa::Params targap(inp);
    assert(targa::save(op / "image_export.tga", targap));

    // TIFF
    tiff::Params tiffp(images);
    assert(tiff::save(op / "image_export.tiff", tiffp, 1));

    // WebP
    webp::Params webpp(inp);
    assert(webp::save(op / "image_export.webp", webpp));

    // UMBF
    aecl::image::umbf::Params umbfp{inp};
    const auto umbf_path = op / "image_export.umia";
    assert(aecl::image::umbf::save(umbf_path, umbfp));
    auto umbf_loader = aecl::image::get_importer_by_path(umbf_path);
    assert(umbf_loader);
    acul::vector<::umbf::Image2D> restored_images;
    assert(umbf_loader->load(umbf_path, restored_images));
    assert(restored_images.size() == 1u);
    const auto &restored = restored_images.front();
    assert(restored.width == inp.width && restored.height == inp.height && restored.channels == inp.channels &&
           restored.format == inp.format && restored.size() == inp.size());
    assert(std::memcmp(restored.pixels, inp.pixels, inp.size()) == 0);
    acul::release(restored.pixels);
    acul::release(umbf_loader);

    for (auto &image : images) acul::release(image.pixels);
    acul::release(loader);
}
