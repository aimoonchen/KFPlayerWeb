/*
#include <SDL.h>
//#include <SDL_image.h>
#if __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#endif
#include <stdio.h>

int main(int argc, char* argv[]) {

   SDL_Window *window;                    // Declare a pointer

   SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
   SDL_SetHint(SDL_HINT_VIDEO_EXTERNAL_CONTEXT, "1");
   // Create an application window with the following settings:
   window = SDL_CreateWindow(
       "An SDL2 window",                  // window title
       SDL_WINDOWPOS_UNDEFINED,           // initial x position
       SDL_WINDOWPOS_UNDEFINED,           // initial y position
       640,                               // width, in pixels
       480,                               // height, in pixels
       SDL_WINDOW_OPENGL                  // flags - see below
   );

   // Check that the window was successfully created
   if (window == NULL) {
       // In the case that the window could not be made...
       printf("Could not create window: %s\n", SDL_GetError());
       return 1;
   }

   // The window is open: could enter program loop here (see SDL_PollEvent())
   // Setup renderer
   SDL_Renderer* renderer = NULL;
   renderer =  SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);

   // Set render color to red ( background will be rendered in this color )
   SDL_SetRenderDrawColor( renderer, 255, 0, 0, 255 );

   // Clear winow
   SDL_RenderClear( renderer );

   // bouyatest

   // Creat a rect at pos ( 50, 50 ) that's 50 pixels wide and 50 pixels high.
   SDL_Rect r;
   r.x = 50;
   r.y = 50;
   r.w = 500;
   r.h = 500;

   // Set render color to blue ( rect will be rendered in this color )
   SDL_SetRenderDrawColor( renderer, 0, 0, 255, 255 );

   // Render image
   // SDL_Surface *loadedImage = IMG_Load("res/hello.png");
   // SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, loadedImage);
   // SDL_FreeSurface(loadedImage);

   // SDL_RenderCopy(renderer, texture, NULL, &r);

   // Render the rect to the screen
   SDL_RenderPresent(renderer);

   SDL_Delay(8000);  // Pause execution for 3000 milliseconds, for example

   // Close and destroy the window
   SDL_DestroyWindow(window);

   // Clean up
   SDL_Quit();
   return 0;
}

*/

// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

// #include <stdio.h>
// #include <SDL/SDL.h>

// #ifdef __EMSCRIPTEN__
// #include <emscripten.h>
// #endif

// extern "C" int main(int argc, char** argv) {
//   printf("hello, world!\n");

//   SDL_Init(SDL_INIT_VIDEO);
//   SDL_Surface *screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);

// #ifdef TEST_SDL_LOCK_OPTS
//   EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
// #endif

//   if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
//   for (int i = 0; i < 256; i++) {
//     for (int j = 0; j < 256; j++) {
// #ifdef TEST_SDL_LOCK_OPTS
//       // Alpha behaves like in the browser, so write proper opaque pixels.
//       int alpha = 255;
// #else
//       // To emulate native behavior with blitting to screen, alpha component is ignored. Test that it is so by outputting
//       // data (and testing that it does get discarded)
//       int alpha = (i+j) % 255;
// #endif
//       *((Uint32*)screen->pixels + i * 256 + j) = SDL_MapRGBA(screen->format, i, j, 255-i, alpha);
//     }
//   }
//   if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
//   SDL_Flip(screen); 

//   printf("you should see a smoothly-colored square - no sharp lines but the square borders!\n");
//   printf("and here is some text that should be HTML-friendly: amp: |&| double-quote: |\"| quote: |'| less-than, greater-than, html-like tags: |<cheez></cheez>|\nanother line.\n");

//   SDL_Quit();

//   return 0;
// }
#include <cstdio>
#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "logo.h"
//#include "imgui/imgui.h"

namespace
{

    class ExampleHelloWorld : public entry::AppI
    {
    public:
        ExampleHelloWorld(const char* _name, const char* _description, const char* _url)
                : entry::AppI(_name, _description, _url)
        {
        }

        void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
        {
            Args args(_argc, _argv);

            m_width  = _width;
            m_height = _height;
            m_debug  = BGFX_DEBUG_TEXT;
            m_reset  = BGFX_RESET_VSYNC;

            bgfx::Init init;
            init.type     = args.m_type;
            init.vendorId = args.m_pciId;
            init.resolution.width  = m_width;
            init.resolution.height = m_height;
            init.resolution.reset  = m_reset;
            bgfx::init(init);
            // Enable debug text.
            bgfx::setDebug(m_debug);

            // Set view 0 clear state.
            bgfx::setViewClear(0
                    , BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
                    , 0x303030ff
                    , 1.0f
                    , 0
            );
//            imguiCreate();
        }

        virtual int shutdown() override
        {
//            imguiDestroy();

            // Shutdown bgfx.
            bgfx::shutdown();

            return 0;
        }

        bool update() override
        {
            if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
            {
//                imguiBeginFrame(m_mouseState.m_mx
//                        ,  m_mouseState.m_my
//                        , (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
//                          | (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
//                          | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
//                        ,  m_mouseState.m_mz
//                        , uint16_t(m_width)
//                        , uint16_t(m_height)
//                );
//
//                showExampleDialog(this);
//
//                imguiEndFrame();

                // Set view 0 default viewport.
                bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );

                // This dummy draw call is here to make sure that view 0 is cleared
                // if no other draw calls are submitted to view 0.
                bgfx::touch(0);

                // Use debug font to print information about this example.
                bgfx::dbgTextClear();
                bgfx::dbgTextImage(
                        bx::max<uint16_t>(uint16_t(m_width /2/8 ), 20)-20
                        , bx::max<uint16_t>(uint16_t(m_height/2/16),  6)-6
                        , 40
                        , 12
                        , s_logo
                        , 160
                );
                bgfx::dbgTextPrintf(0, 1, 0x0f, "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");

                bgfx::dbgTextPrintf(80, 1, 0x0f, "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    \x1b[; 7m    \x1b[0m");
                bgfx::dbgTextPrintf(80, 2, 0x0f, "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    \x1b[;15m    \x1b[0m");

                const bgfx::Stats* stats = bgfx::getStats();
                bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters."
                        , stats->width
                        , stats->height
                        , stats->textWidth
                        , stats->textHeight
                );

                // Advance to next frame. Rendering thread will be kicked to
                // process submitted rendering primitives.
                bgfx::frame();

                return true;
            }

            return false;
        }

        entry::MouseState m_mouseState;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;
    };

} // namespace

ENTRY_IMPLEMENT_MAIN(
        ExampleHelloWorld
, "00-helloworld"
, "Initialization and debug text."
, "https://bkaradzic.github.io/bgfx/examples.html#helloworld"
);
