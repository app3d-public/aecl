# App3D Extended Content Loader

**AECL** is a C++ library for importing and exporting images and 3D scenes through a unified interface.   
UMBF data structures are used as an intermediate representation for imported and exported assets. AECL does not implement the full UMBF feature set and uses an eager data model.

## Limitations
- Memory-mapped access is not supported.
- Nested UMBF containers are not supported.

## Supported Formats

### Images
- BMP
- GIF
- HDR
- HEIF
- JPEG
- OpenEXR
- PNG
- PBM
- Targa
- TIFF
- WebP
- UMBF

### Scenes
- OBJ

## Building

### Supported compilers:
- GNU GCC
- Clang

### Supported OS:
- Linux
- Microsoft Windows

### External packages
These are system libraries that must be available at build time:
- [OpenImageIO](https://openimageio.readthedocs.io/)

### Bundled submodules

- [acbt](https://repos.wusikijeronii.me/app3d/acbt)
- [acul](https://repos.wusikijeronii.me/app3d/acul)
- [umbf](https://repos.wusikijeronii.me/app3d/umbf)
- [earcut](https://github.com/mapbox/earcut)

## License
This project is licensed under the [MIT License](LICENSE).

## Contacts
For any questions or feedback, you can reach out via [email](mailto:wusikijeronii@gmail.com) or open a new issue.
