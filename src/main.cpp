#include "input.hpp"
#include "strict_parse.hpp"
#include <SDL.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
struct Lab {
    chase::InputState input;
    chase::Viewport viewport{0, 0, 0, 0};
    chase::TouchPoint touch{};
    bool touching = false;
    bool running = true;
    explicit Lab(std::vector<chase::Binding> bindings) : input(std::move(bindings)) {}
    void resize(int width, int height) {
        viewport = chase::fitViewport(20, height / 2, width - 40, height / 2 - 20);
        touching = false;
    }
    void event(const SDL_Event& event) {
        switch (event.type) {
        case SDL_QUIT: running = false; break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            input.event(chase::Source::Key, event.key.keysym.sym,
                event.type == SDL_KEYDOWN, event.key.repeat != 0);
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            input.event(chase::Source::Mouse, event.button.button,
                event.type == SDL_MOUSEBUTTONDOWN);
            if (event.button.button == SDL_BUTTON_LEFT)
                touching = event.type == SDL_MOUSEBUTTONDOWN &&
                    chase::mapTouch(viewport, event.button.x, event.button.y, touch);
            break;
        case SDL_MOUSEMOTION:
            if (touching) touching = chase::mapTouch(viewport,
                event.motion.x, event.motion.y, touch);
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                input.reset();
                touching = false;
            } else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                resize(event.window.data1, event.window.data2);
            break;
        default: break;
        }
    }
};
void pump(Lab& lab) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) lab.event(event);
}
void push(SDL_Event event) {
    if (SDL_PushEvent(&event) < 0) throw std::runtime_error(SDL_GetError());
}
SDL_Event key(Uint32 type, SDL_Keycode code, bool repeat = false) {
    SDL_Event event{};
    event.type = type;
    event.key.keysym.sym = code;
    event.key.repeat = repeat ? 1 : 0;
    return event;
}
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void smoke(Lab& lab) {
    // Use fixed smoke bindings independently of user edits.
    lab.input = chase::InputState({
        {chase::Action::Up, chase::Source::Key, SDLK_w, chase::Mode::Hold},
        {chase::Action::A, chase::Source::Key, SDLK_SPACE, chase::Mode::Hold},
        {chase::Action::A, chase::Source::Mouse, SDL_BUTTON_LEFT, chase::Mode::Hold},
        {chase::Action::B, chase::Source::Key, SDLK_q, chase::Mode::Toggle},
        {chase::Action::X, chase::Source::Key, SDLK_e, chase::Mode::Hold},
        {chase::Action::Walk, chase::Source::Key, SDLK_LALT, chase::Mode::Hold}
    });
    lab.input.reset();
    push(key(SDL_KEYDOWN, SDLK_w));
    push(key(SDL_KEYDOWN, SDLK_SPACE));
    SDL_Event mouse{};
    mouse.type = SDL_MOUSEBUTTONDOWN;
    mouse.button.button = SDL_BUTTON_LEFT;
    mouse.button.x = lab.viewport.x + lab.viewport.width / 2;
    mouse.button.y = lab.viewport.y + lab.viewport.height / 2;
    push(mouse);
    push(key(SDL_KEYUP, SDLK_SPACE));
    push(key(SDL_KEYDOWN, SDLK_q));
    push(key(SDL_KEYDOWN, SDLK_q, true));
    push(key(SDL_KEYDOWN, SDLK_LALT));
    pump(lab);
    require(lab.input.active(chase::Action::Up), "SDL key integration failed");
    require(lab.input.active(chase::Action::A), "SDL multi-binding integration failed");
    require(lab.input.active(chase::Action::B), "SDL toggle repeat integration failed");
    require(lab.input.active(chase::Action::Walk) && !lab.input.active(chase::Action::X), "SDL Walk integration failed");
    chase::TouchPoint expected{};
    require(chase::mapTouch(lab.viewport, mouse.button.x, mouse.button.y, expected),
        "Smoke test point outside viewport");
    require(lab.touching && lab.touch.x == expected.x && lab.touch.y == expected.y,
        "SDL mouse touch integration failed");
    SDL_Event focus{};
    focus.type = SDL_WINDOWEVENT;
    focus.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    push(focus);
    pump(lab);
    require(!lab.input.active(chase::Action::Up) && !lab.input.active(chase::Action::A) &&
        !lab.input.active(chase::Action::B) && !lab.input.active(chase::Action::Walk) && !lab.touching, "SDL focus reset failed");
    std::cout << "SDL dummy input smoke test passed\n";
}
void render(SDL_Window* window, SDL_Renderer* renderer, const Lab& lab, int width) {
    SDL_SetRenderDrawColor(renderer, 15, 19, 30, 255);
    SDL_RenderClear(renderer);
    std::ostringstream title;
    title << "The Chase Begins input laboratory (game not implemented) | ";
    for (std::size_t i = 0; i < chase::actionCount; ++i) {
        auto action = static_cast<chase::Action>(i);
        bool active = lab.input.active(action);
        title << chase::actionName(action) << ':' << (active ? "ON " : "- ");
        int cell = std::max(1, (width - 40) / int(chase::actionCount));
        SDL_Rect bar{20 + int(i) * cell, 45, std::max(1, cell - 4), active ? 95 : 20};
        SDL_SetRenderDrawColor(renderer, active ? 50 : 65, active ? 210 : 75, 115, 255);
        SDL_RenderFillRect(renderer, &bar);
    }
    title << "| axis " << lab.input.horizontal() << ',' << lab.input.vertical();
    if (lab.touching) title << " | touch " << lab.touch.x << ',' << lab.touch.y;
    title << " | Escape quits";
    SDL_SetWindowTitle(window, title.str().c_str());
    SDL_Rect lower{lab.viewport.x, lab.viewport.y, lab.viewport.width, lab.viewport.height};
    SDL_SetRenderDrawColor(renderer, 35, 48, 69, 255);
    SDL_RenderFillRect(renderer, &lower);
    SDL_SetRenderDrawColor(renderer, 130, 160, 195, 255);
    SDL_RenderDrawRect(renderer, &lower);
    if (lab.touching) {
        int x = lab.viewport.x + lab.touch.x * lab.viewport.width / 320;
        int y = lab.viewport.y + lab.touch.y * lab.viewport.height / 240;
        SDL_SetRenderDrawColor(renderer, 255, 205, 75, 255);
        SDL_RenderDrawLine(renderer, x - 8, y, x + 8, y);
        SDL_RenderDrawLine(renderer, x, y - 8, x, y + 8);
    }
    SDL_RenderPresent(renderer);
}
}

namespace {
struct Arguments {
    bool smokeTest = false;
    std::string configuration = "config/controls.cfg";
};

Arguments parseArguments(int argc, char** argv) {
    Arguments arguments;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        strict::forOption(argument, [&] {
            if (argument == "--smoke-test") arguments.smokeTest = true;
            else if (argument == "--config") arguments.configuration = strict::nextValue(argc, argv, i);
            else strict::reject("unknown option", argument);
        });
    }
    return arguments;
}
}

int main(int argc, char** argv) {
    Arguments arguments;
    try {
        arguments = parseArguments(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-input-lab: " << error.what() << '\n';
        return 1;
    }
    const bool smokeTest = arguments.smokeTest;
    const std::string configuration = arguments.configuration;
    try {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
            throw std::runtime_error(SDL_GetError());
        struct Cleanup { ~Cleanup() { SDL_Quit(); } } cleanup;
        std::ifstream controls(configuration);
        if (!controls) throw std::runtime_error("Cannot open controls: " + configuration);
        auto bindings = chase::readBindings(controls, [](const std::string& keyName) {
            return int(SDL_GetKeyFromName(keyName.c_str()));
        });
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
            SDL_CreateWindow("The Chase Begins input laboratory (game not implemented)",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
                smokeTest ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE), SDL_DestroyWindow);
        if (!window) throw std::runtime_error(SDL_GetError());
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE), SDL_DestroyRenderer);
        if (!renderer) throw std::runtime_error(SDL_GetError());
        Lab lab(std::move(bindings));
        pump(lab); // Discard initial window lifecycle before synthetic assertions.
        lab.resize(800, 600);
        if (smokeTest) {
            smoke(lab);
            render(window.get(), renderer.get(), lab, 800);
            return 0;
        }
        while (lab.running) {
            pump(lab);
            int width = 0, height = 0;
            SDL_GetWindowSize(window.get(), &width, &height);
            render(window.get(), renderer.get(), lab, width);
            SDL_Delay(16);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Input laboratory: " << error.what() << '\n';
        return 1;
    }
}
