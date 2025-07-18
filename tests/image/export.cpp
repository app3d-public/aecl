#include <aecl/image/export.hpp>
#include <aecl/image/import.hpp>
#include "../env.hpp"

using namespace aecl;

void test_image_export()
{
    test_environment env;
    create_test_environment(env);
    umbf::streams::HashResolver meta_resolver;
    meta_resolver.streams = {
        {umbf::sign_block::image, &umbf::streams::image},
    };
    umbf::streams::resolver = &meta_resolver;

    auto p = acul::io::path(env.data_dir) / "image.umia";
    auto loader = aecl::image::get_importer_by_path(p);
    assert(loader);
    acul::vector<image::umbf::Image2D> images;
    auto state = loader->load(p, images);
    assert(state == acul::io::file::op_state::success);
    assert(!images.empty());
    auto &inp = images.front();

    acul::io::path op = env.output_dir;
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
    openEXR::Params openEXRp(images);
    assert(openEXR::save(op / "image_export.exr", openEXRp, 2));

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
    aecl::image::umbf::Params umbfp(inp);
    assert(aecl::image::umbf::save(op / "image_export.umia", umbfp));

    for (auto &image : images) acul::release(image.pixels);
    acul::release(loader);
}