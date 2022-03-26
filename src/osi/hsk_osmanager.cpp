#include "hsk_osmanager.hpp"
#include <sdl2/SDL.h>

namespace hsk{
    void OsManager::Init(){
        SDL_Init(SDL_INIT_EVERYTHING);
    }

    void OsManager::Cleanup(){
        SDL_Quit();
    }

    Event::ptr OsManager::PollEvent(){
        SDL_Event rawevent;
        if (SDL_PollEvent(&rawevent) == 0){
            return Event::ptr();
        }
        else{
            // TODO: Translate events
            return Event::ptr();
        }
    }
}