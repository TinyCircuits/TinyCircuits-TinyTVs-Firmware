#include "display_unix.h"
#include "../debug/debug.h"


DisplayUnix::DisplayUnix(uint16_t width, uint16_t height){
    debug_println("Display Unix");

    // Store these
    width = width;
    height = height;

    // https://dev.to/noah11012/using-sdl2-opening-a-window-79c
    // Init SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        debug_println("Failed to initialize the SDL2 library");
    }

    // Create window
    window = SDL_CreateWindow("TV Window",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height,
        SDL_WINDOW_SHOWN);
    
    // Check window created without error
    if(!window){
        debug_println("Failed to create window");
    }

    // Create window renderer and a framebuffer
    window_renderer     = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    window_frame_buffer = SDL_CreateTexture(window_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width, height);
    buffer = (uint16_t*)malloc(width*height*sizeof(uint16_t));

    // Increase window size
    SDL_SetWindowSize(window, width*3, height*3);

    // Update the screen once
    update();
}


DisplayUnix::~DisplayUnix(){
    // Clean up and free everything allocated
    if (buffer){
        free(buffer);
        buffer = NULL;
    }

    if (window_frame_buffer){
        SDL_DestroyTexture(window_frame_buffer);
        window_frame_buffer = NULL;
    }

    if (window_renderer){
        SDL_DestroyRenderer(window_renderer);
        window_renderer = NULL;
    }

    if (window){
        SDL_DestroyWindow(window);
        window = NULL;
    }
}


void DisplayUnix::write(uint16_t *buffer, uint32_t buffer_len){
    
}


void DisplayUnix::update(){
    SDL_UpdateTexture(window_frame_buffer , NULL, buffer, width*sizeof(uint16_t));
    SDL_RenderClear(window_renderer);
    SDL_RenderCopy(window_renderer, window_frame_buffer, NULL, NULL);
    SDL_RenderPresent(window_renderer);
}