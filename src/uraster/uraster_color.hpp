#ifndef URASTER_COLOR_HPP_INCLUDED
#define URASTER_COLOR_HPP_INCLUDED

#include <cstdint>

#define _UR_CFORMAT_RGB332 1
#define _UR_CFORMAT_RGB565 2
#define _UR_CFORMAT_RGB888 3
#define _UR_CFORMAT_RGBA4444 4
#define _UR_CFORMAT_RGBA8888 5
#define _UR_CFORMAT_ABGR8888 6

#define _UR_INTERNAL_CFORMAT 2

#if ((_UR_INTERNAL_CFORMAT==2) || (_UR_INTERNAL_CFORMAT==4))
typedef uint16_t _ur_color_internal;

#elif ((_UR_INTERNAL_CFORMAT==3) || (_UR_INTERNAL_CFORMAT==5) || (_UR_INTERNAL_CFORMAT==6))
typedef uint32_t _ur_color_internal;

#elif (_UR_INTERNAL_CFORMAT == 1)
typedef uint8_t _ur_color_internal;

#endif

#endif // URASTER_COLOR_HPP_INCLUDED
