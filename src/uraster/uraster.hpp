#ifndef URASTER_HPP_INCLUDED
#define URASTER_HPP_INCLUDED

#include "uraster_color.hpp"
#include <stddef.h>

#ifndef PROGMEM
#define PROGMEM
#endif // PROGMEM
typedef struct
{
  const uint8_t width;
  const uint16_t offset;

} FONT_CHAR_INFO;
typedef struct
{
  const unsigned char height;
  const char startCh;
  const char endCh;
  const FONT_CHAR_INFO* charDesc;
  const unsigned char* bitmap;

} FONT_INFO;
#include "font.h"

namespace uraster
{
    struct color
    {
        _ur_color_internal c;
        color();
        /**
         * @brief Constructor for internal color type given 8-bit truecolor channel intensities.
         * @param[in] r truecolor red intensity between 0 and 255.
         * @param[in] g truecolor green intensity between 0 and 255.
         * @param[in] b truecolor blue intensity between 0 and 255.
         * @param[in] a truecolor alpha intensity between 0 and 255; Defaults to 255.
         */
        color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255);

        color(const int r, const int g, const int b, const int a = 255);

        /**
         * @brief Constructor for internal color type given 8-bit floating-point channel intensities.
         * @param[in] r floating-point red intensity between 0 and 1.
         * @param[in] g floating-point green intensity between 0 and 1.
         * @param[in] b floating-point blue intensity between 0 and 1.
         * @param[in] a floating-point alpha intensity between 0 and 1; Defaults to 1.
         */
        color(const float r, const float g, const float b, const float a = 1.0f);

        /**
         * @brief Constructor for internal color type given 32-bit packed RGBA.
         * @param[in] p packed 32-bit RGBA pixel with the alpha channel at the little end.
         */
        color(const uint32_t p);

        /**
         * @brief Constructor for internal color type given a copy.
         * @param[in] _c color to make a copy of.
         */

        color(const _ur_color_internal p);

        /**
         * @brief Constructor for internal color type given a copy.
         * @param[in] _c color to make a copy of.
         */
        color(const color& _c);

        /**
         * @brief Accessor for internal red channel value.
         * @return internal red intensity.
         */
        const uint8_t r();

        /**
         * @brief Accessor for internal green channel value.
         * @return internal green intensity.
         */
        const uint8_t g();

        /**
         * @brief Accessor for internal blue channel value.
         * @return internal blue intensity.
         */
        const uint8_t b();

        /**
         * @brief Accessor for internal alpha channel value.
         * @return internal alpha intensity.
         */
        const uint8_t a();

        /**
         * @brief Accessor for internal red channel value.
         * @return truecolor red intensity between 0 and 255.
         */
        const uint8_t r8();

        /**
         * @brief Accessor for internal green channel value.
         * @return truecolor green intensity between 0 and 255.
         */
        const uint8_t g8();

        /**
         * @brief Accessor for internal blue channel value.
         * @return truecolor blue intensity between 0 and 255.
         */
        const uint8_t b8();

        /**
         * @brief Accessor for internal alpha channel value.
         * @return truecolor alpha intensity between 0 and 255.
         */
        const uint8_t a8();

        /**
         * @brief Accessor for internal red channel value.
         * @return floating-point red intensity between 0 and 1.
         */
        const float rf();

        /**
         * @brief Accessor for internal green channel value.
         * @return floating-point green intensity between 0 and 1.
         */
        const float gf();

        /**
         * @brief Accessor for internal blue channel value.
         * @return floating-point blue intensity between 0 and 1.
         */
        const float bf();

        /**
         * @brief Accessor for internal alpha channel value.
         * @return floating-point alpha intensity between 0 and 1.
         */
        const float af();

        /**
         * @brief Mutator for internal red channel value.
         * @param[in] r truecolor red intensity between 0 and 255.
         */
        void setr(const uint8_t r);

        /**
         * @brief Mutator for internal green channel value.
         * @param[in] g truecolor green intensity between 0 and 255.
         */
        void setg(const uint8_t g);

        /**
         * @brief Mutator for internal blue channel value.
         * @param[in] b truecolor blue intensity between 0 and 255.
         */
        void setb(const uint8_t b);

        /**
         * @brief Mutator for internal alpha channel value.
         * @param[in] a truecolor alpha intensity between 0 and 255.
         */
        void seta(const uint8_t a);

        /**
         * @brief Mutator for internal red channel value.
         * @param[in] r floating-point red intensity between 0 and 1.
         */
        void setr(const float r);

        /**
         * @brief Mutator for internal green channel value.
         * @param[in] g floating-point green intensity between 0 and 1.
         */
        void setg(const float g);

        /**
         * @brief Mutator for internal blue channel value.
         * @param[in] b floating-point blue intensity between 0 and 1.
         */
        void setb(const float b);

        /**
         * @brief Mutator for internal alpha channel value.
         * @param[in] a floating-point alpha intensity between 0 and 1.
         */
        void seta(const float a);

        /**
         * @brief Assignment operator overloaded for chaining.
         */
        const color& operator=(const color&);

        /**
         * @brief Addition operator overloaded for alpha blending.
         */
        const color operator+(color&);
    };
    struct frame
    {
        _ur_color_internal* bufPtr;
        uint16_t xs, ys;

        frame();
        /**
         * @brief Constructor for a frame object.
         * @param[in] u16 _xs, size of new frame in the x-direction.
         * @param[in] u16 _ys, size of new frame in the y-direction.
         */
        frame(const uint16_t _xs, const uint16_t _ys);

        /**
         * @brief Constructor for a frame object.
         * @param[in] const frame& c, frame to copy into new frame.
         */
        frame(const frame& c);

        /**
         * @brief Get pointer to pixel at (x, y).
         * @note Returns the precise system pointer to (x, y) in the frame's buffer.
         * @param[in] u16 x, x-position of pixel.
         * @param[in] u16 y, y-position of pixel.
         * @return _ur_color_internal*, pointer to pixel at (x, y).
         */
        constexpr _ur_color_internal* getBufPtr(const uint16_t x, const uint16_t y)
        {
            return bufPtr+(y * xs)+x;
        }

        /**
         * @brief Get color of pixel at (x, y).
         * @param[in] x x-position of pixel.
         * @param[in] y y-position of pixel.
         * @return color of pixel at (x, y).
         */
        const color getColor(const uint16_t x, const uint16_t y);

        /**
         * @brief Fills buffer with specified color.
         * @param[in] c color to fill buffer with.
         */
        void fillBuf(const color c);

        /**
         * @brief Set color of pixel at (x, y).
         * @param[in] x x-position of pixel.
         * @param[in] y y-position of pixel.
         * @param[in] c color to set pixel to.
         */
        void setColor(const uint16_t x, const uint16_t y, const color c);

        /**
         * @brief Blit other buffer somewhere on this frame.
         * @param[in] x x-position to start blitting buffer to.
         * @param[in] y y-position to start blitting buffer to.
         * @param[in] xoff x-position offset from which to start reading blit buffer.
         * @param[in] yoff y-position offset from which to start reading blit buffer.
         * @param[in] xend size in the x-direction of blit rectangle.
         * @param[in] yend size in the y-direction of blit rectangle.
         */
        void blitBuf(frame& f, const uint16_t x, const uint16_t y, const uint16_t xoff, const uint16_t yoff, const uint16_t xend, const uint16_t yend);

        void blitBuf(frame& f, const uint16_t x, const uint16_t y, const uint16_t xoff, const uint16_t yoff, const uint16_t xend, const uint16_t yend, const color);
    };
    struct renderer2d
    {
        frame* target;
        frame* source;

        renderer2d();

        /**
         * @brief Constructs a renderer2d instance given a target but no source/texture buffer.
         * @note calls to the affine-textured rendering functions cause crashes without a source buffer.
         * @param[in] t reference to target frame for rendering.
         */
        renderer2d(frame& t);

        /**
         * @brief Constructs a renderer2d instance given a target and source buffer.
         * @param[in] t reference to target frame for rendering.
         * @param[in] s reference to source frame for rendering.
         */
        renderer2d(frame& t, frame& s);

        _ur_color_internal (*activePixelShader)(uint16_t, uint16_t, int32_t, void*, const size_t);

        /**
         * @brief Draw a point/pixel primitive given a color.
         * @param[in] x x-coordinate of pixel.
         * @param[in] y y-coordinate of pixel.
         * @param[in] c color of pixel.
         */
        void drawPixel(const uint16_t x, const uint16_t y, const color c = color(255, 255, 255));

        /**
         * @brief Draw a point/pixel primitive given a pair of UV source/texture coordinates.
         * @param[in] x x-coordinate of pixel.
         * @param[in] y y-coordinate of pixel.
         * @param[in] u u-coordinate of texel.
         * @param[in] v v-coordinate of texel.
         */
        void drawPixel(const uint16_t x, const uint16_t y, const uint16_t u, const uint16_t v);

        /**
         * @brief Draw a point/pixel primitive given a set of pixel shader inputs.
         * @param[in] x x-coordinate of pixel.
         * @param[in] y y-coordinate of pixel.
         * @param[in] in pointer to pixel shader input array.
         * @param[in] n_in number of elements in pixel shader input array.
         */
        void drawPixel(const uint16_t x, const uint16_t y, int32_t* in, const size_t n_in);

        /**
         * @brief Draw multiple point/pixel primitives given a set of vertices and a single color.
         * @param[in] xybuf pointer to array of pixel coordinates (x0, y0, x1, y1... xn, yn).
         * @param[in] n number of pixels.
         * @param[in] c color of pixels.
         */
        void drawPixels(const uint16_t* xybuf, const size_t n, const color c = color(255, 255, 255));

        /**
         * @brief Draw multiple point/pixel primitives given a set of vertices and a set of colors.
         * @note cbuf is expected to have exactly as many entries as there are pixels.
         * @param[in] xybuf pointer to array of pixel coordinates (x0, y0, x1, y1... xn, yn).
         * @param[in] n number of pixels.
         * @param[in] cbuf pointer to array of colors for each pixel.
         */
        void drawPixels(const uint16_t* xybuf, const size_t n, const color* cbuf);

        /**
         * @brief Draw multiple point/pixel primitives given a set of vertices and a set of texture coordinate pairs.
         * @note uvbuf is expected to have exactly as many pairs of coordinates as there are pixels.
         * @param[in] xybuf pointer to array of pixel coordinates (x0, y0, x1, y1... xn, yn).
         * @param[in] n number of pixels.
         * @param[in] uvbuf pointer to array of texel coordinates (u0, v0, u1, v1... un, vn).
         */
        void drawPixels(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf);

        /**
         * @brief Draw multiple point/pixel primitives given a set of vertices and some pixel shader inputs.
         * @param[in] xybuf pointer to array of pixel coordinates (x0, y0, x1, y1... xn, yn).
         * @param[in] n number of pixels.
         * @param[in] in pointer to pixel shader input array.
         * @param[in] n_in number of pixel shader inputs.
         */
        void drawPixels(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in);

        /**
         * @brief Draw multiple point/pixel primitives given a set of vertices and a set of pixel shader input arrays.
         * @note in is expected to have exactly as many sets of inputs as there are pixels.
         * @param[in] xybuf pointer to array of pixel coordinates (x0, y0, x1, y1... xn, yn).
         * @param[in] n number of pixels.
         * @param[in] in pointer to pixel shader input array... array?
         * @param[in] n_in number of pixel shader inputs... array.
         */
        void drawPixels(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in);

        /**
         * @brief Draw flat-color line.
         * @param[in] x1 x-coordinate of one segment endpoint.
         * @param[in] y1 y-coordinate of one segment endpoint.
         * @param[in] x2 x-coordinate of another segment endpoint.
         * @param[in] y2 y-coordinate of another segment endpoint.
         * @param[in] c color of line segment.
         */
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const color c = color(255, 255, 255));

        /**
         * @brief Draw affine textured line.
         * @param[in] x1 x-coordinate of one segment endpoint.
         * @param[in] y1 y-coordinate of one segment endpoint.
         * @param[in] x2 x-coordinate of another segment endpoint.
         * @param[in] y2 y-coordinate of another segment endpoint.
         * @param[in] u1 u-coordinate of one endpoint texel.
         * @param[in] v1 v-coordinate of one endpoint texel.
         * @param[in] u2 u-coordinate of another endpoint texel.
         * @param[in] v2 v-coordinate of another endpoint texel.
         */
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2);
        void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int32_t* in, const size_t n_in);

        void drawLines(const uint16_t* xybuf, const size_t n, const color c = color(255, 255, 255));
        void drawLines(const uint16_t* xybuf, const size_t n, const color* cbuf);
        void drawLines(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf);
        void drawLines(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in);
        void drawLines(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in);

        void drawLinesStrip(const uint16_t* xybuf, const size_t n, const color c = color(255, 255, 255));
        void drawLinesStrip(const uint16_t* xybuf, const size_t n, const color* cbuf);
        void drawLinesStrip(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf);
        void drawLinesStrip(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in);
        void drawLinesStrip(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in);

        void drawLinesFan(const uint16_t* xybuf, const size_t n, const color c = color(255, 255, 255));
        void drawLinesFan(const uint16_t* xybuf, const size_t n, const color* cbuf);
        void drawLinesFan(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf);
        void drawLinesFan(const uint16_t* xybuf, const size_t n, const int32_t* in, const size_t n_in);
        void drawLinesFan(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t n_in);

        void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, const color c = color(255, 255, 255));
        void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2, uint16_t u3, uint16_t v3);
        void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2, uint16_t u3, uint16_t v3, const color);

        void drawChar(const uint16_t x, const uint16_t y, const char a, const color c = color(255, 255, 255), const FONT_INFO& fi = liberationSans_10ptFontInfo);
        void drawStr(const uint16_t x, const uint16_t y, const char* s, const color c = color(255, 255, 255), const FONT_INFO& fi = liberationSans_10ptFontInfo);

        void drawXShear(const uint16_t x, const uint16_t y, const uint16_t xskip, const int16_t xoff);
        void drawYShear(const uint16_t x, const uint16_t y, const uint16_t yskip, const int16_t yoff);
    };
}

#endif // URASTER_HPP_INCLUDED
