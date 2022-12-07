// *** ADDED BY HEADER FIXUP ***
#include <istream>
// *** END ***
#include "uraster.hpp"
#include "uraster_color.hpp"

#include <iostream>

uraster::color::color()
{

}

uraster::color::color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
    c = 0;
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        // Pack to 8-bit R3 G3 B2
        c |= r & 0xE0;
        c |= (g >> 3) & 0x1C;
        c |= (b >> 6) & 0x03;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        // Pack to 16-bit R5 G6 B5
        c |= (uint16_t(r) << 8) & 0xF800;
        c |= (uint16_t(g) << 3) & 0x07E0;
        c |= (b >> 3) & 0x001F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        c |= (uint16_t(r) << 8) & 0xF000;
        c |= (uint16_t(g) << 4) & 0x0F00;
        c |= b & 0x00F0;
        c |= (a >> 4) & 0x000F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        c |= uint32_t(r) << 24;
        c |= uint32_t(g) << 16;
        c |= uint32_t(b) << 8;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        c |= uint32_t(r) << 24;
        c |= uint32_t(g) << 16;
        c |= uint32_t(b) << 8;
        c |= uint32_t(a);
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        c |= uint32_t(a) << 24;
        c |= uint32_t(b) << 16;
        c |= uint32_t(g) << 8;
        c |= uint32_t(r);
    }
}

uraster::color::color(const int r, const int g, const int b, const int a)
{
    *this = uraster::color(uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a));
}

uraster::color::color(const float r, const float g, const float b, const float a)
{
    *this = uraster::color(uint8_t(r * 255.0 + 0.5), uint8_t(g * 255.0 + 0.5), uint8_t(b * 255.0 + 0.5), uint8_t(a * 255.0 + 0.5));
}

uraster::color::color(const uint32_t p)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888) c = p;
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888) c = (p | 0x000000FF);
    else *this = uraster::color(uint8_t(p >> 24), uint8_t(p >> 16), uint8_t(p >> 8), uint8_t(p));
}

uraster::color::color(const _ur_color_internal p)
{
  c = p;
}

uraster::color::color(const uraster::color& _c)
{
    c = _c.c;
}

const uint8_t uraster::color::r()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c >> 5) & 0x07;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c >> 11) & 0x1F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c >> 12) & 0x0F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 24) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 24) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c) & 0xFF;
    }
}

const uint8_t uraster::color::g()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c >> 2) & 0x07;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c >> 5) & 0x3F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c >> 8) & 0x0F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 16) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 16) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 8) & 0xFF;
    }
}

const uint8_t uraster::color::b()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c) & 0x03;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c) & 0x1F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c >> 4) & 0x0F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 8) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 8) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 16) & 0xFF;
    }
}

const uint8_t uraster::color::a()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c) & 0x0F;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 24) & 0xFF;
    }
}

const uint8_t uraster::color::r8()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c & 0xE0) ? ((c & 0xE0) | 0x1F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c & 0xF800) ? (((c & 0xF800) >> 8) | 0x07) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c & 0xF000) ? ((c >> 8) | 0x0F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 24) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 24) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c) & 0xFF;
    }
}

const uint8_t uraster::color::g8()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c & 0x1C) ? (((c & 0x1C) << 3) | 0x1F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c & 0x07E0) ? (((c & 0x07E0) >> 3) | 0x03) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c & 0x0F00) ? (((c & 0x0F00) >> 4) | 0x0F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 16) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 16) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 8) & 0xFF;
    }
}

const uint8_t uraster::color::b8()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return (c & 0x03) ? (((c & 0x03) << 6) | 0x3F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return (c & 0x001F) ? (((c & 0x001F) << 3) | 0x07) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c & 0x00F0) ? ((c & 0x00F0) | 0x0F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return (c >> 8) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c >> 8) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 16) & 0xFF;
    }
}

const uint8_t uraster::color::a8()
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        return (c & 0x000F) ? ((c << 4) | 0x0F) : 0;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        return 255;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        return (c) & 0xFF;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        return (c >> 24) & 0xFF;
    }
}

const float uraster::color::rf()
{
    return r8() / 255.0f;
}

const float uraster::color::gf()
{
    return g8() / 255.0f;
}

const float uraster::color::bf()
{
    return b8() / 255.0f;
}

const float uraster::color::af()
{
    return a8() / 255.0f;
}

void uraster::color::setr(const uint8_t r)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        c &= 0x1F;
        c |= (r & 0xE0);
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        c &= 0x07FF;
        c |= uint16_t(r & 0xF8) << 8;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        c &= 0x0FFF;
        c |= uint16_t(r & 0xF0) << 8;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        c &= 0x00FFFFFF;
        c |= uint32_t(r) << 24;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        c &= 0x00FFFFFF;
        c |= uint32_t(r) << 24;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        c &= 0xFFFFFF00;
        c |= uint32_t(r);
    }
}

void uraster::color::setg(const uint8_t g)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        c &= 0xE3;
        c |= (g & 0xE0) >> 3;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        c &= 0xF81F;
        c |= uint16_t(g & 0xFC) << 3;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        c &= 0xF0FF;
        c |= uint16_t(g & 0xF0) << 4;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        c &= 0xFF00FFFF;
        c |= uint32_t(g) << 16;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        c &= 0xFF00FFFF;
        c |= uint32_t(g) << 16;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        c &= 0xFFFF00FF;
        c |= uint32_t(g) << 8;
    }
}

void uraster::color::setb(const uint8_t b)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB332)
    {
        c &= 0xFC;
        c |= (b & 0xC0) >> 6;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB565)
    {
        c &= 0xFFE0;
        c |= uint16_t(b & 0xF8) >> 3;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        c &= 0xFF0F;
        c |= uint16_t(b & 0xF0);
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGB888)
    {
        c &= 0xFFFF00FF;
        c |= uint32_t(b) << 8;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        c &= 0xFFFF00FF;
        c |= uint32_t(b) << 8;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        c &= 0xFF00FFFF;
        c |= uint32_t(b) << 16;
    }
}

void uraster::color::seta(const uint8_t a)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444)
    {
        c &= 0xFFF0;
        c |= uint16_t(a & 0xF0) >> 4;
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888)
    {
        c &= 0xFFFFFF00;
        c |= uint32_t(a);
    }
    else if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        c &= 0x00FFFFFF;
        c |= uint32_t(a) << 24;
    }
}

void uraster::color::setr(const float r)
{
    setr(uint8_t(r * 255.0 + 0.5));
}

void uraster::color::setg(const float g)
{
    setg(uint8_t(g * 255.0 + 0.5));
}

void uraster::color::setb(const float b)
{
    setb(uint8_t(b * 255.0 + 0.5));
}

void uraster::color::seta(const float a)
{
    seta(uint8_t(a * 255.0 + 0.5));
}

const uraster::color& uraster::color::operator=(const uraster::color& _rhs)
{
    c = _rhs.c;
    return *this;
}

const uraster::color uraster::color::operator+(uraster::color& _rhs)
{
    if(_UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA4444 || _UR_INTERNAL_CFORMAT == _UR_CFORMAT_RGBA8888 || _UR_INTERNAL_CFORMAT == _UR_CFORMAT_ABGR8888)
    {
        uint8_t r1 = (uint16_t(r8()) * (0x100 - _rhs.a8())) >> 8;
        uint8_t r2 = (uint16_t(_rhs.r8()) * _rhs.a8()) >> 8;
        uint8_t g1 = (uint16_t(g8()) * (0x100 - _rhs.a8())) >> 8;
        uint8_t g2 = (uint16_t(_rhs.g8()) * _rhs.a8()) >> 8;
        uint8_t b1 = (uint16_t(b8()) * (0x100 - _rhs.a8())) >> 8;
        uint8_t b2 = (uint16_t(_rhs.b8()) * _rhs.a8()) >> 8;
        return uraster::color(uint8_t(r1 + r2), uint8_t(g1 + g2), uint8_t(b1 + b2), (uint16_t(a8()) + _rhs.a8() <= 0xFF) ? uint8_t(a8() + _rhs.a8()) : uint8_t(255));
    }
    else
    {
        return _rhs;
    }
}
