#pragma once

#include <oneapi/tbb/concurrent_vector.h>
#include "../import.hpp"

namespace ecl
{
    namespace scene
    {
        namespace obj
        {
            /**
             * @brief Info about a line in text based file which store a content
             * and the index of the line
             * @tparam T The type of the value
             */
            template <typename T>
            struct Line
            {
                int index; // Line index
                T value;   // Line content

                bool operator<(const Line &other) const { return index < other.index; }
            };

            // MTL Texture parameters
            struct TextureOption
            {
                std::string path; // Path to the texture
                /**
                 * @brief The blendu option turns texture blending in the horizontal direction
                    (u direction) on or off.  The default is on.
                **/
                bool blendu{true};
                /**
                 * @brief The blendv option turns texture blending in the vertical direction
                 *  (v direction) on or off.  The default is on.
                 */
                bool blendv{true};
                /**
                 *  The boost option increases the sharpness, or clarity. If you render animations with boost,
                 *you may experience some texture crawling.
                 *
                 * Value is any non-negative floating point value representing the
                 * degree of increased clarity; the greater the value, the greater the
                 * clarity.  You should start with a boost value of no more than 1 or 2 and
                 * increase the value as needed. Note that larger values have more
                 * potential to introduce texture crawling when animated.
                 **/
                float boost{0};
                /**
                 *  The mm option modifies the range over which scalar or color texture
                 * values may vary.  This has an effect only during rendering and does not
                 * change the file. It uses two values: base and gain.
                 *
                 *  "base" adds a base value to the texture values.  A positive value makes
                 * everything brighter; a negative value makes everything dimmer.  The
                 * default is 0; the range is unlimited.
                 *
                 *  "gain" expands the range of the texture values.  Increasing the number
                 * increases the contrast.  The default is 1; the range is unlimited.
                 */
                glm::vec2 mm{0.0f, 1.0f};
                /**
                 * The option offsets the position of the texture map on the surface by
                 * shifting the position of the map origin.  The default is 0, 0, 0.
                 *
                 * "u" is the value for the horizontal direction of the texture
                 *
                 * "v" is an optional argument.
                 * "v" is the value for the vertical direction of the texture.
                 *
                 * "w" is an optional argument.
                 * "w" is the value used for the depth of a 3D texture.
                 */
                glm::vec3 offset{0.0f, 0.0f, 0.0f};
                /**
                 * The option scales the size of the texture pattern on the textured
                 * surface by expanding or shrinking the pattern.  The default is 1, 1, 1.
                 *
                 * "u" is the value for the horizontal direction of the texture
                 *
                 * "v" is an optional argument.
                 * "v" is the value for the vertical direction of the texture.
                 *
                 * "w" is an optional argument.
                 * "w" is a value used for the depth of a 3D texture.
                 * "w" is a value used for the amount of tessellation of the displacement map.
                 */
                glm::vec3 scale{1.0f, 1.0f, 1.0f};
                /**
                 * The option turns on turbulence for textures. Adding turbulence to a
                 * texture along a specified direction adds variance to the original image
                 * and allows a simple image to be repeated over a larger area without
                 * noticeable tiling effects.
                 *
                 * Turbulence also lets you use a 2D image as if it were a solid texture,
                 * similar to 3D procedural textures like marble and granite.
                 *
                 * "u" is the value for the horizontal direction of the texture
                 * turbulence.
                 *
                 * "v" is an optional argument.
                 * "v" is the value for the vertical direction of the texture turbulence.
                 *
                 * "w" is an optional argument.
                 * "w" is a value used for the depth of the texture turbulence.
                 *
                 * By default, the turbulence for every texture map used in a material is
                 * uvw = (0,0,0).  This means that no turbulence will be applied and the 2D
                 * texture will behave normally.
                 *
                 * Only when you raise the turbulence values above zero will you see the
                 * effects of turbulence.
                 */
                glm::vec3 turbulence{0.0f, 0.0f, 0.0f};
                /**
                 * The option specifies the resolution of texture created when an
                 * image is used.  The default texture size is the largest power of two
                 * that does not exceed the original image size.
                 *
                 * If the source image is an exact power of 2, the texture cannot be built
                 * any larger.  If the source image size is not an exact power of 2, you
                 * can specify that the texture be built at the next power of 2 greater
                 * than the source image size.
                 *
                 * The original image should be square, otherwise, it will be scaled to
                 * fit the closest square size that is not larger than the original.
                 * Scaling reduces sharpness.
                 */
                int resolution{0};
                /**
                 * The -clamp option turns clamping on or off.  When clamping is on,
                 * textures are restricted to 0-1 in the uvw range.  The default is off.
                 *
                 *  When clamping is turned on, one copy of the texture is mapped onto the
                 * surface, rather than repeating copies of the original texture across the
                 * surface of a polygon, which is the default.  Outside of the origin
                 * texture, the underlying material is unchanged.
                 *
                 *  A postage stamp on an envelope or a label on a can of soup is an
                 * example of a texture with clamping turned on.  A tile floor or a
                 * sidewalk is an example of a texture with clamping turned off.
                 *
                 * Two-dimensional textures are clamped in the u and v dimensions; 3D
                 * procedural textures are clamped in the u, v, and w dimensions.
                 **/
                bool clamp{false};
                /**
                 *  The option specifies a bump multiplier. You can use it only with
                 *  the "bump" statement.  Values stored with the texture or procedural
                 *  texture file are multiplied by this value before they are applied to the
                 *  surface.
                 *
                 *  The value can be positive or negative. Extreme bump multipliers may cause odd visual results
                 * because only the surface normal is perturbed and the surface position does not change. For
                 * best results, use values between 0 and 1.
                 */
                float bumpIntensity{1.0f};
                /**
                 * The imfchan option specifies the channel used to create a scalar or
                 * bump texture.  Scalar textures are applied to:
                 *
                 * transparency
                 * specular exponent
                 * decal
                 * displacement
                 *
                 * The channel choices are:
                 *
                 * r specifies the red channel.
                 * g specifies the green channel.
                 * b specifies the blue channel.
                 * m specifies the matte channel.
                 * l specifies the luminance channel.
                 * z specifies the z-depth channel.
                 *
                 * The default for bump and scalar textures is "l" (luminance), unless you
                 * are building a decal.  In that case, the default is "m" (matte).
                 **/
                char imfchan{0};

                std::string type;
            };

            struct ColorOption
            {
                enum class Type
                {
                    RGB,
                    XYZ
                };
                Type type{Type::RGB};
                glm::vec3 value{1.0f, 1.0f, 1.0f};
            };

            // OBJ Material in accoring MTL file format specification
            struct Material
            {
                /**
                 * @brief Material name
                 */
                std::string name;

                /**
                 * @brief Specifies ambient color, to account for light that is scattered about
                 * the entire scene using values between 0 and 1 for the RGB components
                 */
                ColorOption Ka;
                /**
                 * @brief Specifies diffuse color, which typically contributes most of the color
                 * to an object
                 */
                ColorOption Kd;
                // Specifies specular color, the color seen where the surface is shiny and mirror-like
                ColorOption Ks;
                /**
                 * @brief Specifies diffuse color, which typically contributes most of the color Defines the
                 * focus of specular highlights in the material. Ns values normally range from 0 to 1000, with a
                 * high value resulting in a tight, concentrated highlight.
                 */
                float Ns{10.0f};
                /**
                 * @brief defines the optical density (aka index of refraction) in the current material.
                 * The values can range from 0.001 to 10. A value of 1.0 means that light does not
                 * bend as it passes through an object.
                 */
                float Ni;
                /**
                 * @brief specifies a factor for dissolve, how much this material dissolves into the background.
                 * A factor of 1.0 is fully opaque. A factor of 0.0 is completely transparent.
                 */
                float d{1.0};
                /**
                 * @brief specifies the transparency of the material as a value between 0.0 and 1.0.
                 * Tr euqals: 1.0 - d
                 */
                float Tr{0.0f};

                /**
                 * The Tf statement specifies the transmission filter using RGB/XYZ values.
                 *
                 * "r g b" are the values for the red, green, and blue components of the
                 * atmosphere.  The g and b arguments are optional.  If only r is
                 * specified, then g, and b are assumed to be equal to r.  The r g b values
                 * are normally in the range of 0.0 to 1.0.  Values outside this range
                 * increase or decrease the relectivity accordingly.
                 *
                 */
                ColorOption Tf;
                /**
                 * Specifies an illumination model, using a numeric value.
                 * The value 0 represents the simplest illumination model,
                 * relying on the Kd for the material modified by a texture map specified in a map_Kd statement
                 * if present. The compilers of this resource believe that the choice of illumination model is
                 * irrelevant for 3D printing use and is ignored on import by some software applications. For
                 * example, the MTL Loader in the threejs Javascript library appears to ignore illum statements.
                 *
                 * Possible values:
                 * 0. Color on and Ambient off
                 * 1. Color on and Ambient on
                 * 2. Highlight on
                 * 3. Reflection on and Ray trace on
                 * 4. Transparency: Glass on, Reflection: Ray trace on
                 * 5. Reflection: Fresnel on and Ray trace on
                 * 6. Transparency: Refraction on, Reflection: Fresnel off and Ray trace on
                 * 7. Transparency: Refraction on, Reflection: Fresnel on and Ray trace on
                 * 8. Reflection on and Ray trace off
                 * 9. Transparency: Glass on, Reflection: Ray trace off
                 * 10. Casts shadows onto invisible surfaces
                 */
                int illum;

                /// PBR Workflow
                float Pr;
                float Pm;
                float Ps;
                float Ke;
                float Pc;
                float Pcr;
                float aniso;
                float anisor;

                /// Textures

                TextureOption map_Ka;
                TextureOption map_Kd;
                TextureOption map_Ks;
                TextureOption map_Ns;
                TextureOption map_d;
                TextureOption map_Tr;
                TextureOption map_bump;
                TextureOption disp;
                TextureOption decal;
                TextureOption refl;

                /// PBR Workflow
                TextureOption map_Pr;
                TextureOption map_Pm;
                TextureOption map_Ps;
                TextureOption map_Ke;
                TextureOption norm;
            };

            struct ParseIndexed
            {
                oneapi::tbb::concurrent_vector<Line<glm::vec3>> v;
                oneapi::tbb::concurrent_vector<Line<glm::vec2>> vt;
                oneapi::tbb::concurrent_vector<Line<glm::vec3>> vn;
                oneapi::tbb::concurrent_vector<Line<astl::vector<glm::ivec3> *>> f;
                oneapi::tbb::concurrent_vector<Line<std::string>> g;
                std::string mtllib;
                oneapi::tbb::concurrent_vector<Line<std::string>> useMtl;
            };

            /**
             * @brief Parses a line of a .obj file
             * @param line The line to parse
             * @param i The line number
             */
            void parseLine(ParseIndexed &buffer, const std::string_view &line, int i);

            /**
             * @brief Parses a line from a MTL file and updates the list of materials.
             *
             * @param line The line to parse.
             * @param materials The list of materials to update.
             * @param matIndex The current material index.
             * @param lineIndex The index of the line being parsed.
             */
            void parseMTLline(const std::string_view &line, astl::vector<Material> &materials, int &matIndex,
                              int lineIndex);

            /**
             * @brief Process MTL color option.
             *
             * This function processes a color option from a Material Template Library (MTL) file.
             * It takes a token pointer and a reference to a ColorOption struct, and returns a boolean
             * indicating whether the processing was successful.
             *
             * @param token A pointer to the token string.
             * @param dst   A reference to the ColorOption struct to store the processed color option.
             *
             * @return True if the processing was successful, false otherwise.
             */
            bool processMTLColorOption(const char *&token, ColorOption &dst);

            /**
             * @brief Process the MTL texture option.
             *
             * @param token A pointer to the token string.
             * @param dst The destination texture option.
             * @return true if the processing is successful, false otherwise.
             */
            bool processMTLTextureOption(const char *&token, TextureOption &dst);

            /**
             * @brief Processes an on/off flag from a token and updates a boolean value.
             *
             * @param token A pointer to the token string.
             * @param dst The destination boolean value.
             * @return true if the flag was successfully processed, false otherwise.
             */
            bool processOnOffFlag(const char *&token, bool &dst);

            /**
             * Converts the materials in the given list to PBR materials.
             *
             * @param basePath The base path for loading material textures.
             * @param mtlMatList Parsed MTL material list.
             * @param materials The Dst materials list.
             * @param textures The Dst texture list.
             */
            void convertToMaterials(std::filesystem::path basePath, const astl::vector<Material> &mtlMatList,
                                    astl::vector<assets::MaterialInfo> &materials,
                                    astl::vector<assets::Image2D> &textures);

            /**
             * @brief Load the scene
             * @return Read state result
             **/
            class APPLIB_API Importer : public ILoader
            {
            public:
                Importer(const std::filesystem::path &filename) : ILoader(filename){};

                /**
                 * @brief Load the scene
                 * @return True if the scene was loaded
                 **/
                io::file::ReadState load(events::Manager &e) override;
            };
        } // namespace obj
    } // namespace scene
} // namespace ecl