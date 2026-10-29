#pragma once

#include "input.hpp"
#include "runtime/pad_events.hpp"

#include <SDL.h>

class PadDevice {
public:
    PadDevice() {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return;
        openFirst();
    }
    PadDevice(const PadDevice&) = delete;
    PadDevice& operator=(const PadDevice&) = delete;
    ~PadDevice() { close(); }

    bool handle(const SDL_Event& event, chase::InputState& input) {
        switch (event.type) {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            if (event.cbutton.which != instance_) return false;
            return chase::runtime::feedPadButton(input, event.cbutton.button, event.type == SDL_CONTROLLERBUTTONDOWN);
        case SDL_CONTROLLERDEVICEADDED:
            if (!controller_) openFirst();
            return true;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (controller_ && event.cdevice.which == instance_) {
                close();
                input.reset();
                openFirst();
            }
            return true;
        default:
            return false;
        }
    }

private:
    void openFirst() {
        for (int index = 0; index < SDL_NumJoysticks(); ++index) {
            if (!SDL_IsGameController(index)) continue;
            controller_ = SDL_GameControllerOpen(index);
            if (!controller_) continue;
            instance_ = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_));
            return;
        }
    }

    void close() {
        if (controller_ && SDL_WasInit(SDL_INIT_GAMECONTROLLER)) SDL_GameControllerClose(controller_);
        controller_ = nullptr;
        instance_ = -1;
    }

    SDL_GameController* controller_ = nullptr;
    SDL_JoystickID instance_ = -1;
};
