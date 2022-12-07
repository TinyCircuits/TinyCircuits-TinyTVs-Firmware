#include "uraster.hpp"

uraster::frame::frame(void)
{

}

uraster::frame::frame(const uint16_t _xs, const uint16_t _ys)
{
    xs = _xs;
    ys = _ys;
    bufPtr = new _ur_color_internal[_xs * _ys];
}

uraster::frame::frame(const uraster::frame& c)
{
    xs = c.xs;
    ys = c.ys;
    bufPtr = new _ur_color_internal[xs * ys];
    for(size_t i = 0; i < size_t(xs) * ys; i++)
    {
        bufPtr[i] = c.bufPtr[i];
    }
}

const uraster::color uraster::frame::getColor(const uint16_t x, const uint16_t y)
{
    uraster::color c;
    c.c = *getBufPtr(x, y);
    return c;
}

void uraster::frame::fillBuf(const uraster::color c)
{
    for(size_t i = 0; i < size_t(xs) * ys; i++) bufPtr[i] = c.c;
}

void uraster::frame::setColor(const uint16_t x, const uint16_t y, const uraster::color c)
{
    *getBufPtr(x, y) = c.c;
}

void uraster::frame::blitBuf(uraster::frame& f, const uint16_t x, const uint16_t y, const uint16_t xoff, const uint16_t yoff, const uint16_t _xs, const uint16_t _ys)
{
    size_t pos = size_t(y) * xs;
    size_t pos2 = size_t(yoff) * f.xs;
    for(size_t cy = y; cy < y + _ys; cy++)
    {
        for(int cx = x; cx < x + _xs; cx++) bufPtr[pos + cx] = f.bufPtr[pos2 + (xoff + cx - x)];
        pos += xs;
        pos2 += f.xs;
    }
}

void uraster::frame::blitBuf(uraster::frame& f, const uint16_t x, const uint16_t y, const uint16_t xoff, const uint16_t yoff, const uint16_t _xs, const uint16_t _ys, const uraster::color _tkey)
{
    size_t pos = size_t(y) * xs;
    size_t pos2 = size_t(yoff) * f.xs;
    for(size_t cy = y; cy < y + _ys; cy++)
    {
        for(int cx = x; cx < x + _xs; cx++) if(f.bufPtr[pos2 + (xoff + cx - x)] != _tkey.c) bufPtr[pos + cx] = f.bufPtr[pos2 + (xoff + cx - x)];
        pos += xs;
        pos2 += f.xs;
    }
}
