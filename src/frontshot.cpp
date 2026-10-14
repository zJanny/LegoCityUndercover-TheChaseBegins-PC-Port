// chase-frontshot: runs the front end offscreen with a scripted pad and saves screenshots (400x480, the two screens
// stacked as the emulator's screenshots). No window, no sound. Script grammar: frontend/script.hpp.
#include "frontend/frontend.hpp"
#include "frontend/movie_host.hpp"
#include "frontend/script.hpp"
#include "strict_parse.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::frontend;

struct Options {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string movies = "artifacts/romfs/romfs/cutscenes";
    std::string script, output = ".";
    int language = 1;
    bool firstRun = true;
};

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() { return strict::nextValue(argc, argv, i); };
        strict::forOption(arg, [&] {
            if (arg == "--fib") options.fib = value();
            else if (arg == "--names") options.names = value();
            else if (arg == "--movies") options.movies = value();
            else if (arg == "--script") {
                options.script = value();
                parseFrontScript(options.script);
            }
            else if (arg == "--output") options.output = value();
            else if (arg == "--language") options.language = int(strict::integer(value(), 1, 9));
            else if (arg == "--returning") options.firstRun = false;
            else strict::reject("unknown option", arg);
        });
    }
    if (options.script.empty()) throw std::runtime_error("--script is required");
    return options;
}

int run(const Options& options) {
    assets::FuseArchive archive(options.fib);
    archive.loadNames(options.names);
    Resources resources(archive);
    MoviePictureHost movies(options.movies);
    FlowConfig config;
    config.language = options.language;
    config.firstRun = options.firstRun;
    FrontEnd front(resources, &movies, config);
    Canvas top(kTopWidth, kScreenHeight), bottom(kBottomWidth, kScreenHeight);
    ScriptHooks hooks;
    hooks.shot = [&](const std::string& name) {
        front.draw(top, bottom);
        composeScreens(top, bottom).writeBmp(options.output + "/" + name + ".bmp");
        std::cout << name << ": screen " << screenName(front.flow().screen()) << " prompt " << int(front.flow().prompt()) << '\n';
    };
    const unsigned updates = runScript(front, options.script, hooks);
    std::cout << "frontshot: " << updates << " updates, screen " << screenName(front.flow().screen()) << ", handoff slot "
              << front.flow().handoff().slot << (front.flow().done() ? " (done)" : "") << '\n';
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        return run(parse(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "chase-frontshot: " << error.what() << '\n';
        return 1;
    }
}
