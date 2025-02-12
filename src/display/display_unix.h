#ifndef DISPLAY_UNIX_H
#define DISPLAY_UNIX_H

#include <stdint.h>
#include <SDL2/SDL.h>

class DisplayUnix{
    public:
        DisplayUnix(uint16_t width, uint16_t height);
        ~DisplayUnix();

        void write(uint16_t *buffer, uint32_t buffer_len);
        void update();
    private:
        uint16_t width;
        uint16_t height;

        uint16_t *buffer;
        SDL_Window   *window;
        SDL_Renderer *window_renderer;      // https://dev.to/noah11012/using-sdl2-2d-accelerated-renderering-1kcb
        SDL_Texture  *window_frame_buffer;  // https://gamedev.stackexchange.com/questions/157604/how-to-get-access-to-framebuffer-as-a-uint32-t-in-sdl2
};

#endif