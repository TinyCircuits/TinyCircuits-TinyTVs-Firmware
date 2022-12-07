#include "uraster.hpp"

#include "pico/stdlib.h"   // stdlib 
#include "hardware/vreg.h"

_ur_color_internal defaultPixelShader(uint16_t px, uint16_t py, int32_t t, void* in, const size_t n_in)
{
    return uraster::color((255*t) >> 16, (255*(0x10000-t)) >> 16, 255).c;
}

uraster::renderer2d::renderer2d()
{
    target = nullptr;
    source = nullptr;
    activePixelShader = &defaultPixelShader;
}

uraster::renderer2d::renderer2d(uraster::frame& t)
{
    target = &t;
    source = nullptr;
    activePixelShader = &defaultPixelShader;
}

uraster::renderer2d::renderer2d(uraster::frame& t, uraster::frame& s)
{
    target = &t;
    source = &s;
    activePixelShader = &defaultPixelShader;
}

void uraster::renderer2d::drawPixel(const uint16_t x, const uint16_t y, const color c)
{
    *target->getBufPtr(x, y) = c.c;
}

void uraster::renderer2d::drawPixel(const uint16_t x, const uint16_t y, const uint16_t u, const uint16_t v)
{
    *target->getBufPtr(x, y) = *source->getBufPtr(u, v);
}

void uraster::renderer2d::drawPixel(const uint16_t x, const uint16_t y, int32_t* in, const size_t n_in)
{
    *target->getBufPtr(x, y) = activePixelShader(x, y, 0x10000, in, n_in);
}

void uraster::renderer2d::drawPixels(const uint16_t* xybuf, const size_t n, const color c)
{
    for(size_t i = 0; i < (n << 1); i+=2)
    {
        *target->getBufPtr(xybuf[i], xybuf[i+1]) = c.c;
    }
}

void uraster::renderer2d::drawPixels(const uint16_t* xybuf, const size_t n, const color* cbuf)
{
    for(size_t i = 0; i < (n << 1); i+=2)
    {
        *target->getBufPtr(xybuf[i], xybuf[i+1]) = cbuf[i >> 1].c;
    }
}

void uraster::renderer2d::drawPixels(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf)
{
    for(size_t i = 0; i < (n << 1); i+=2)
    {
        *target->getBufPtr(xybuf[i], xybuf[i+1]) = *source->getBufPtr(uvbuf[i], uvbuf[i+1]);
    }
}

void uraster::renderer2d::drawPixels(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in)
{
    for(size_t i = 0; i < (n << 1); i+=2)
    {
        *target->getBufPtr(xybuf[i], xybuf[i+1]) = activePixelShader(xybuf[i], xybuf[i+1], 0x10000, in, n_in);
    }
}

void uraster::renderer2d::drawPixels(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in)
{
    for(size_t i = 0; i < (n << 1); i+=2)
    {
        *target->getBufPtr(xybuf[i], xybuf[i+1]) = activePixelShader(xybuf[i], xybuf[i+1], 0x10000, in[i >> 1], n_in[i >> 1]);
    }
}

constexpr auto abs(const auto& x) {return (x>=0) ? x:-x;}

constexpr void swap(auto* a, auto* b) {*a ^= *b; *b ^= *a; *a ^= *b;}

// This function computes (1/d)*2^32 using only 32-bit division so multiplication by reciprocal is easy
constexpr int32_t inv32(const int32_t d) {return (d > 0) ?  ((uint32_t(0xFFFFFFFF - d + 1) / d)) : -((uint32_t(0xFFFFFFFF + d + 1) / (-d)));}

void uraster::renderer2d::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const uraster::color c)
{
    int32_t dy = (int32_t(y2) - y1);
    int32_t dx = (int32_t(x2) - x1);
    if(abs(dx) > abs(dy))
    {
        if(x1 > x2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            dx = -dx;
            dy = -dy;
        }
        dy = (dy << 16) / dx;
        int32_t cy = int32_t(y1) << 16;
        size_t pos = (cy >> 16) * target->xs;
        for(uint16_t x = x1; x < x2; x++)
        {
            target->bufPtr[pos + x] = c.c;
            if((cy + dy) >> 16 > cy >> 16) pos += target->xs;
            else if((cy+dy) >> 16 < cy >> 16) pos -= target->xs;
            cy += dy;
        }
    }
    else
    {
        if(y1 > y2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            dx = -dx;
            dy = -dy;
        }
        dx = (dx << 16) / dy;
        int32_t cx = int32_t(x1) << 16;
        size_t pos = y1 * target->xs;
        for(uint16_t y = y1; y < y2; y++)
        {
            target->bufPtr[pos + (cx >> 16)] = c.c;
            pos += target->xs;
            cx += dx;
        }
    }
}

void uraster::renderer2d::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2)
{
    int32_t dy = (int32_t(y2) - y1);
    int32_t dx = (int32_t(x2) - x1);
    int32_t du = (int32_t(u2) - u1);
    int32_t dv = (int32_t(v2) - v1);
    if(abs(dx) > abs(dy))
    {
        if(x1 > x2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            swap(&u1, &u2);
            swap(&v1, &v2);
            dx = -dx;
            dy = -dy;
            du = -du;
            dv = -dv;
        }
        const int32_t idx = inv32(dx);
        int32_t cy = int32_t(y1) << 16;
        int32_t cu = int32_t(u1) << 16;
        int32_t cv = int32_t(v1) << 16;
        dy = (int64_t(dy) * idx) >> 16;
        du = (int64_t(du) * idx) >> 16;
        dv = (int64_t(dv) * idx) >> 16;
        size_t pos = (cy >> 16) * target->xs;
        for(uint16_t x = x1; x < x2; x++)
        {
            target->bufPtr[pos + x] = *source->getBufPtr(cu >> 16, cv >> 16);
            if((cy + dy) >> 16 > cy >> 16) pos += target->xs;
            else if((cy+dy) >> 16 < cy >> 16) pos -= target->xs;
            cy += dy;
            cu += du;
            cv += dv;
        }
    }
    else
    {
        if(y1 > y2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            swap(&u1, &u2);
            swap(&v1, &v2);
            dx = -dx;
            dy = -dy;
            du = -du;
            dv = -dv;
        }
        const int32_t idy = inv32(dy);
        int32_t cx = int32_t(x1) << 16;
        int32_t cu = int32_t(u1) << 16;
        int32_t cv = int32_t(v1) << 16;
        dx = (int64_t(dx) * idy) >> 16;
        du = (int64_t(du) * idy) >> 16;
        dv = (int64_t(dv) * idy) >> 16;
        size_t pos = y1 * target->xs;
        for(uint16_t y = y1; y < y2; y++)
        {
            target->bufPtr[pos + (cx >> 16)] = *source->getBufPtr(cu >> 16, cv >> 16);
            pos += target->xs;
            cx += dx;
            cu += du;
            cv += dv;
        }
    }
}

void uraster::renderer2d::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int32_t* in, const size_t n_in)
{
    int32_t dy = (int32_t(y2) - y1);
    int32_t dx = (int32_t(x2) - x1);
    int32_t dt = 0x10000;
    if(abs(dx) > abs(dy))
    {
        if(x1 > x2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            dx = -dx;
            dy = -dy;
            dt = -dt;
        }
        dy = (dy << 16) / dx;
        dt = dt / dx;
        int32_t ct = (dt > 0) ? 0 : 0x10000;
        int32_t cy = int32_t(y1) << 16;
        size_t pos = (cy >> 16) * target->xs;
        for(uint16_t x = x1; x < x2; x++)
        {
            target->bufPtr[pos + x] = activePixelShader(x, cy>>16, ct, in, n_in);
            if((cy + dy) >> 16 > cy >> 16) pos += target->xs;
            else if((cy+dy) >> 16 < cy >> 16) pos -= target->xs;
            cy += dy;
            ct += dt;
        }
    }
    else
    {
        if(y1 > y2)
        {
            swap(&x1, &x2);
            swap(&y1, &y2);
            dx = -dx;
            dy = -dy;
            dt = -dt;
        }
        dx = (dx << 16) / dy;
        dt = dt / dy;
        int32_t ct = (dt > 0) ? 0 : 0x10000;
        int32_t cx = int32_t(x1) << 16;
        size_t pos = y1 * target->xs;
        for(uint16_t y = y1; y < y2; y++)
        {
            //*target->getBufPtr(cx >> 16, y) = activePixelShader(cx >> 16, y, ct, in, n_in);
            target->bufPtr[pos + (cx >> 16)] = activePixelShader(cx >> 16, y, ct, in, n_in);
            pos += target->xs;
            cx += dx;
            ct += dt;
        }
    }
}

void uraster::renderer2d::drawLines(const uint16_t* xybuf, const size_t n, const color c)
{
    for(size_t i = 0; i < (n << 2); i+=4)
    {
        drawLine(xybuf[i], xybuf[i+1], xybuf[i+2], xybuf[i+3], c);
    }
}

void uraster::renderer2d::drawLines(const uint16_t* xybuf, const size_t n, const color* cbuf)
{
    for(size_t i = 0; i < (n << 2); i+=4)
    {
        drawLine(xybuf[i], xybuf[i+1], xybuf[i+2], xybuf[i+3], cbuf[i >> 2]);
    }
}

void uraster::renderer2d::drawLines(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf)
{
    for(size_t i = 0; i < (n << 2); i+=4)
    {
        drawLine(xybuf[i], xybuf[i+1], xybuf[i+2], xybuf[i+3], uvbuf[i], uvbuf[i+1], uvbuf[i+2], uvbuf[i+3]);
    }
}

void uraster::renderer2d::drawLines(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in)
{
    for(size_t i = 0; i < (n << 2); i+=4)
    {
        drawLine(xybuf[i], xybuf[i+1], xybuf[i+2], xybuf[i+3], in, n_in);
    }
}

void uraster::renderer2d::drawLines(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in)
{
    for(size_t i = 0; i < (n << 2); i+=4)
    {
        drawLine(xybuf[i], xybuf[i+1], xybuf[i+2], xybuf[i+3], in[i >> 2], n_in[i >> 2]);
    }
}

void uraster::renderer2d::drawLinesStrip(const uint16_t* xybuf, const size_t n, const color c)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[i-3], xybuf[i-2], xybuf[i-1], xybuf[i], c);
    }
}

void uraster::renderer2d::drawLinesStrip(const uint16_t* xybuf, const size_t n, const color* cbuf)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[i-3], xybuf[i-2], xybuf[i-1], xybuf[i], cbuf[(i >> 1) - 1]);
    }
}

void uraster::renderer2d::drawLinesStrip(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[i-3], xybuf[i-2], xybuf[i-1], xybuf[i], uvbuf[i-3], uvbuf[i-2], uvbuf[i-1], uvbuf[i]);
    }
}

void uraster::renderer2d::drawLinesStrip(const uint16_t* xybuf, const size_t n, int32_t* in, const size_t n_in)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[i-3], xybuf[i-2], xybuf[i-1], xybuf[i], in, n_in);
    }
}

void uraster::renderer2d::drawLinesStrip(const uint16_t* xybuf, const size_t n, int32_t** in, const size_t* n_in)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[i-3], xybuf[i-2], xybuf[i-1], xybuf[i], in[(i >> 1) - 1], n_in[(i >> 1) - 1]);
    }
}

void uraster::renderer2d::drawLinesFan(const uint16_t* xybuf, const size_t n, const color c)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[0], xybuf[1], xybuf[i-1], xybuf[i], c);
    }
}

void uraster::renderer2d::drawLinesFan(const uint16_t* xybuf, const size_t n, const color* cbuf)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[0], xybuf[1], xybuf[i-1], xybuf[i], cbuf[(i >> 1) - 1]);
    }
}

void uraster::renderer2d::drawLinesFan(const uint16_t* xybuf, const size_t n, const uint16_t* uvbuf)
{
    for(size_t i = 3; i < (n << 1)+2; i+=2)
    {
        drawLine(xybuf[0], xybuf[1], xybuf[i-1], xybuf[i], uvbuf[0], uvbuf[1], uvbuf[i-1], uvbuf[i]);
    }
}

inline void scanline(uraster::frame* fb, uint16_t x1, uint16_t x2, uint16_t y, const _ur_color_internal c)
{
    if(x1 > x2) swap(&x1, &x2);
    size_t pos = size_t(y) * fb->xs + x1;
    for(uint16_t x = x1; x < x2; x++) fb->bufPtr[pos++] = c;
}

/*
inline void scanlineuv(uraster::frame* fb, uraster::frame* tex, uint16_t x1, uint16_t x2, uint16_t y, int32_t u1, int32_t v1, int32_t u2, int32_t v2)
{
    if(x1 == x2) return;
    if(x1 > x2)
    {
        swap(&x1, &x2);
        swap(&u1, &u2);
        swap(&v1, &v2);
    }
    uint32_t cu = u1;
    uint32_t cv = v1;
    int32_t idx = inv32(int32_t(x2) - x1);
    //int32_t du = (u2 - u1) / (x2 - x1);
    //int32_t dv = (v2 - v1) / (x2 - x1) * tex->xs;
    int32_t du = (int64_t(u2 - u1) * idx) >> 32;
    int32_t dv = ((int64_t(v2 - v1) * idx) >> 32);
    //std::cout<<du<<", "<<dv<<std::endl;
    size_t pos = size_t(y) * fb->xs + x1;
    int32_t tptr = (v1 * tex->xs + u1);
    int32_t dtptr = (dv * tex->xs + du);
    for(uint16_t x = x1; x < x2; x++)
    {
        fb->bufPtr[pos++] = tex->bufPtr[(((cv & 0xFFFF0000) * tex->xs + cu) >> 16)];
        cu += du;
        cv += dv;
    }
}
*/

inline void scanlineuv(uraster::frame* fb, uraster::frame* tex, uint16_t x1, uint16_t x2, uint16_t y, int32_t u1, int32_t v1, int32_t u2, int32_t v2)
{
    if(x1 == x2) return;
    if(x1 > x2)
    {
        swap(&x1, &x2);
        swap(&u1, &u2);
        swap(&v1, &v2);
    }
    uint32_t cu = u1;
    uint32_t cv = v1;
    int32_t idx = inv32(int32_t(x2) - x1);
    int32_t du = (int64_t(u2 - u1) * idx) >> 32;
    int32_t dv = ((int64_t(v2 - v1) * idx) >> 32);
    size_t pos = size_t(y) * fb->xs + x1;
    size_t tpos = (((cv & 0xFFFF0000) * tex->xs + cu) >> 16);
    for(uint16_t x = x1; x < x2; x++)
    {
        fb->bufPtr[pos++] = tex->bufPtr[(((cv & 0xFFFF0000) * tex->xs + cu) >> 16)];
        //fb->bufPtr[pos++] = tex->bufPtr[tpos >> 16];
        cu += du;
        cv += dv;
        //tpos = (((cv & 0xFFFF0000) * tex->xs + cu));
        //tpos += tex->xs * (((0xFFFF0000 & cv) + dv)-(0xFFFF0000 & cv)) + du;
    }
}

inline void scanlineuv(uraster::frame* fb, uraster::frame* tex, uint16_t x1, uint16_t x2, uint16_t y, int32_t u1, int32_t v1, int32_t u2, int32_t v2, const uraster::color _key)
{
    if(x1 == x2) return;
    if(x1 > x2)
    {
        swap(&x1, &x2);
        swap(&u1, &u2);
        swap(&v1, &v2);
    }
    uint32_t cu = u1;
    uint32_t cv = v1;
    int32_t idx = inv32(int32_t(x2) - x1);
    int32_t du = (int64_t(u2 - u1) * idx) >> 32;
    int32_t dv = ((int64_t(v2 - v1) * idx) >> 32);
    size_t pos = size_t(y) * fb->xs + x1;
    size_t tpos = (((cv & 0xFFFF0000) * tex->xs + cu) >> 16);
    for(uint16_t x = x1; x < x2; x++)
    {
        if(tex->bufPtr[(((cv & 0xFFFF0000) * tex->xs + cu) >> 16)] != _key.c) fb->bufPtr[pos++] = tex->bufPtr[(((cv & 0xFFFF0000) * tex->xs + cu) >> 16)];
        else pos++;
        //fb->bufPtr[pos++] = tex->bufPtr[tpos >> 16];
        cu += du;
        cv += dv;
        //tpos = (((cv & 0xFFFF0000) * tex->xs + cu));
        //tpos += tex->xs * (((0xFFFF0000 & cv) + dv)-(0xFFFF0000 & cv)) + du;
    }
}

void uraster::renderer2d::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, const color c)
{
    if(y1 > y2)
    {
        swap(&x1, &x2);
        swap(&y1, &y2);
    }
    if(y2 > y3)
    {
        swap(&x2, &x3);
        swap(&y2, &y3);
    }
    if(y1 > y3)
    {
        swap(&x1, &x3);
        swap(&y1, &y3);
    }
    int32_t cx2;
    int32_t cx1 = cx2 = int32_t(x1) << 16;
    int32_t dx1 = ((int32_t(x2) - x1) << 16) / (y2 - y1);
    int32_t dx2 = ((int32_t(x3) - x1) << 16) / (y3 - y1);
    for(uint16_t y = y1; y < y2; y++)
    {
        scanline(target, cx1 >> 16, cx2 >> 16, y, c.c);
        cx1 += dx1;
        cx2 += dx2;
    }
    dx1 = ((int32_t(x3) - x2) << 16) / (y3 - y2);
    for(uint16_t y = y2; y < y3; y++)
    {
        scanline(target, cx1 >> 16, cx2 >> 16, y, c.c);
        cx1 += dx1;
        cx2 += dx2;
    }
}

void uraster::renderer2d::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2, uint16_t u3, uint16_t v3)
{
    //vreg_set_voltage(VREG_VOLTAGE_1_25);
    //if(!set_sys_clock_khz(270000, false)) return;
    if(y1 > y2)
    {
        swap(&x1, &x2);
        swap(&y1, &y2);
        swap(&u1, &u2);
        swap(&v1, &v2);
    }
    if(y1 > y3)
    {
        swap(&x1, &x3);
        swap(&y1, &y3);
        swap(&u1, &u3);
        swap(&v1, &v3);
    }
    if(y2 > y3)
    {
        swap(&x2, &x3);
        swap(&y2, &y3);
        swap(&u2, &u3);
        swap(&v2, &v3);
    }
    int32_t cx1, cx2, cu1, cu2, cv1, cv2, dx1, dx2, du1, du2, dv1, dv2;
    if(y2 != y1)
    {
        int32_t idy21 = inv32(y2 - y1);
        dx1 = ((int64_t(x2) - x1) * idy21) >> 16;
        du1 = ((int64_t(u2) - u1) * idy21) >> 16;
        dv1 = ((int64_t(v2) - v1) * idy21) >> 16;
    }
    if(y3 != y1)
    {
        int32_t idy31 = inv32(y3 - y1);
        dx2 = ((int64_t(x3) - x1) * idy31) >> 16;
        du2 = ((int64_t(u3) - u1) * idy31) >> 16;
        dv2 = ((int64_t(v3) - v1) * idy31) >> 16;
    }
    cx1 = cx2 = int32_t(x1) << 16;
    cu1 = cu2 = int32_t(u1) << 16;
    cv1 = cv2 = int32_t(v1) << 16;
    for(uint16_t y = y1; y < y2; y++)
    {
        scanlineuv(target, source, cx1 >> 16, cx2 >> 16, y, cu1, cv1, cu2, cv2);
        cx1 += dx1;
        cx2 += dx2;
        cu1 += du1;
        cv1 += dv1;
        cu2 += du2;
        cv2 += dv2;
    }
    cx1 = int32_t(x2) << 16;
    cu1 = int32_t(u2) << 16;
    cv1 = int32_t(v2) << 16;
    if(y3 != y2)
    {
        int32_t idy32 = inv32(y3 - y2);
        /*
        dx1 = ((int32_t(x3) - x2) << 16) / (y3 - y2);
        du1 = ((int32_t(u3) - u2) << 16) / (y3 - y2);
        dv1 = ((int32_t(v3) - v2) << 16) / (y3 - y2);
        */
        dx1 = ((int64_t(x3) - x2) * idy32) >> 16;
        du1 = ((int64_t(u3) - u2) * idy32) >> 16;
        dv1 = ((int64_t(v3) - v2) * idy32) >> 16;
    }
    for(uint16_t y = y2; y < y3; y++)
    {
        scanlineuv(target, source, cx1 >> 16, cx2 >> 16, y, cu1, cv1, cu2, cv2);
        cx1 += dx1;
        cx2 += dx2;
        cu1 += du1;
        cv1 += dv1;
        cu2 += du2;
        cv2 += dv2;
    }
    //vreg_set_voltage(VREG_VOLTAGE_1_10);
    //set_sys_clock_khz(133000, false);
}

void uraster::renderer2d::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t u1, uint16_t v1, uint16_t u2, uint16_t v2, uint16_t u3, uint16_t v3, const uraster::color _key)
{
    //vreg_set_voltage(VREG_VOLTAGE_1_25);
    //if(!set_sys_clock_khz(270000, false)) return;
    if(y1 > y2)
    {
        swap(&x1, &x2);
        swap(&y1, &y2);
        swap(&u1, &u2);
        swap(&v1, &v2);
    }
    if(y1 > y3)
    {
        swap(&x1, &x3);
        swap(&y1, &y3);
        swap(&u1, &u3);
        swap(&v1, &v3);
    }
    if(y2 > y3)
    {
        swap(&x2, &x3);
        swap(&y2, &y3);
        swap(&u2, &u3);
        swap(&v2, &v3);
    }
    int32_t cx1, cx2, cu1, cu2, cv1, cv2, dx1, dx2, du1, du2, dv1, dv2;
    if(y2 != y1)
    {
        int32_t idy21 = inv32(y2 - y1);
        dx1 = ((int64_t(x2) - x1) * idy21) >> 16;
        du1 = ((int64_t(u2) - u1) * idy21) >> 16;
        dv1 = ((int64_t(v2) - v1) * idy21) >> 16;
    }
    if(y3 != y1)
    {
        int32_t idy31 = inv32(y3 - y1);
        dx2 = ((int64_t(x3) - x1) * idy31) >> 16;
        du2 = ((int64_t(u3) - u1) * idy31) >> 16;
        dv2 = ((int64_t(v3) - v1) * idy31) >> 16;
    }
    cx1 = cx2 = int32_t(x1) << 16;
    cu1 = cu2 = int32_t(u1) << 16;
    cv1 = cv2 = int32_t(v1) << 16;
    for(uint16_t y = y1; y < y2; y++)
    {
        scanlineuv(target, source, cx1 >> 16, cx2 >> 16, y, cu1, cv1, cu2, cv2, _key);
        cx1 += dx1;
        cx2 += dx2;
        cu1 += du1;
        cv1 += dv1;
        cu2 += du2;
        cv2 += dv2;
    }
    cx1 = int32_t(x2) << 16;
    cu1 = int32_t(u2) << 16;
    cv1 = int32_t(v2) << 16;
    if(y3 != y2)
    {
        int32_t idy32 = inv32(y3 - y2);
        /*
        dx1 = ((int32_t(x3) - x2) << 16) / (y3 - y2);
        du1 = ((int32_t(u3) - u2) << 16) / (y3 - y2);
        dv1 = ((int32_t(v3) - v2) << 16) / (y3 - y2);
        */
        dx1 = ((int64_t(x3) - x2) * idy32) >> 16;
        du1 = ((int64_t(u3) - u2) * idy32) >> 16;
        dv1 = ((int64_t(v3) - v2) * idy32) >> 16;
    }
    for(uint16_t y = y2; y < y3; y++)
    {
        scanlineuv(target, source, cx1 >> 16, cx2 >> 16, y, cu1, cv1, cu2, cv2, _key);
        cx1 += dx1;
        cx2 += dx2;
        cu1 += du1;
        cv1 += dv1;
        cu2 += du2;
        cv2 += dv2;
    }
    //vreg_set_voltage(VREG_VOLTAGE_1_10);
    //set_sys_clock_khz(133000, false);
}

// Draws 4 horizontal pixels according to the 4 least significant bits of d, but fast
void draw4(_ur_color_internal* buf, const size_t pos, const uint8_t d, const uraster::color c)
{
    switch(d & 0x0F)
    {
    case 0:
        break;
    case 1:
        buf[pos+3] = c.c;
        break;
    case 2:
        buf[pos+2] = c.c;
        break;
    case 3:
        buf[pos+2] = c.c; buf[pos+3] = c.c;
        break;
    case 4:
        buf[pos+1] = c.c;
        break;
    case 5:
        buf[pos+1] = c.c; buf[pos+3] = c.c;
        break;
    case 6:
        buf[pos+1] = c.c; buf[pos+2] = c.c;
        break;
    case 7:
        buf[pos+1] = c.c; buf[pos+2] = c.c; buf[pos+3] = c.c;
        break;
    case 8:
        buf[pos] = c.c;
        break;
    case 9:
        buf[pos] = c.c; buf[pos+3] = c.c;
        break;
    case 10:
        buf[pos] = c.c; buf[pos+2] = c.c;
        break;
    case 11:
        buf[pos] = c.c; buf[pos+2] = c.c; buf[pos+3] = c.c;
        break;
    case 12:
        buf[pos] = c.c; buf[pos+1] = c.c;
        break;
    case 13:
        buf[pos] = c.c; buf[pos+1] = c.c; buf[pos+3] = c.c;
        break;
    case 14:
        buf[pos] = c.c; buf[pos+1] = c.c; buf[pos+2] = c.c;
        break;
    case 15:
        buf[pos] = c.c; buf[pos+1] = c.c; buf[pos+2] = c.c; buf[pos+3] = c.c;
        break;
    }
}

void uraster::renderer2d::drawChar(const uint16_t x, const uint16_t y, const char a, const color c, const FONT_INFO& fi)
{
    FONT_CHAR_INFO charInfo = fi.charDesc[a-32];
    size_t pos = y * target->xs + x;
    size_t offset = charInfo.offset;
    int sizeLeft = charInfo.width;
    while(sizeLeft > 0)
    {
        for(int i = 0; i < fi.height; i++)
        {
            uint16_t b = fi.bitmap[(offset)+((fi.height-1)-i)];
            draw4(target->bufPtr, pos, b >> 4, c);
            draw4(target->bufPtr, pos+4, b, c);
            pos += target->xs;
        }
        pos -= target->xs * fi.height;
        pos += 8;
        offset += fi.height;
        sizeLeft -= 8;
    }
}

void uraster::renderer2d::drawStr(uint16_t x, uint16_t y, const char* s, const color c, const FONT_INFO& fi)
{
    size_t strPos = 0;
    while(s[strPos] != 0)
    {
        drawChar(x, y, s[strPos], c, fi);
        x += fi.charDesc[s[strPos]-32].width+1;
        strPos++;
    }
}

void uraster::renderer2d::drawXShear(const uint16_t x, const uint16_t y, const uint16_t xskip, const int16_t xoff)
{
    size_t pos = y * target->xs + x;
    uint16_t ymod = 0;
    for(uint16_t cy = y; cy < y + source->ys; cy++)
    {
        if(ymod == xskip)
        {
            ymod = 0;
            pos += xoff;
        }
        for(uint16_t cx = x; cx < x + source->xs; cx++)
        {
            target->bufPtr[pos++] = *source->getBufPtr(cx - x, cy - y);
        }
        pos += target->xs - source->xs;
        ymod++;
    }
}

void uraster::renderer2d::drawYShear(const uint16_t x, const uint16_t y, const uint16_t yskip, const int16_t yoff)
{
    size_t pos = y * target->xs + x;
    uint16_t xmod;
    int16_t mul = yoff * target->xs;
    for(uint16_t cy = y; cy < y + source->ys; cy++)
    {
        xmod = 1;
        for(uint16_t cx = x; cx < x + source->xs; cx++)
        {
            target->bufPtr[pos++] = *source->getBufPtr(cx - x, cy - y);
            if(xmod == yskip)
            {
                xmod = 0;
                pos += mul;
            }
            xmod++;
        }
        pos = cy * target->xs + x;
    }
}
