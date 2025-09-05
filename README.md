# App3D Extended Content Loader

**AECL** is a C++ library for importing and exporting images and 3D scenes, developed as part of the App3D project.  
Its purpose is to provide a unified interface for asset I/O, ensure seamless integration with other App3D modules, and meet the project's performance and data fidelity requirements.

Unlike direct use of third-party libraries, AECL operates on native App3D data structures, ensuring that assets — whether images, geometry, or entire scenes — are exported exactly as represented internally.

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

- [acbt](https://git.homedatasrv.ru/app3d/acbt)
- [acul](https://git.homedatasrv.ru/app3d/acul)
- [umbf](https://git.homedatasrv.ru/app3d/umbf)
- [earcut](https://github.com/mapbox/earcut)

## License
This project is licensed under the [MIT License](LICENSE).

## Contacts
For any questions or feedback, you can reach out via [email](mailto:wusikijeronii@gmail.com) or open a new issue.