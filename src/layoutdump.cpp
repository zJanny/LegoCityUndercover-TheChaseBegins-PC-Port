// chase-layoutdump: prints the element tree and storyboards of .bxaml UI layouts, or lists every layout of the archive.
#include "assets/assets.hpp"
#include "strict_parse.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::assets;

const char* typeName(UiElementType type) {
    switch (type) {
    case UiElementType::Container: return "box";
    case UiElementType::Image: return "image";
    case UiElementType::Reference: return "ref";
    }
    return "?";
}

void printElement(const UiElement& e, int depth) {
    std::cout << std::string(std::size_t(depth) * 2, ' ') << typeName(e.type) << ' ' << e.name;
    if (!e.source.empty()) std::cout << " <" << e.source << '>';
    std::cout << " pos " << e.position[0] << ',' << e.position[1] << " size " << e.size[0] << ',' << e.size[1];
    if (e.opacity != 1) std::cout << " opacity " << e.opacity;
    if (e.scale[0] != 1 || e.scale[1] != 1) std::cout << " scale " << e.scale[0] << ',' << e.scale[1];
    if (e.origin[0] != 0 || e.origin[1] != 0) std::cout << " origin " << e.origin[0] << ',' << e.origin[1];
    if (e.flags) std::cout << " flags 0x" << std::hex << e.flags << std::dec;
    if (e.fontSize) std::cout << " font " << unsigned(e.fontSize);
    if (e.alignment) std::cout << " align " << unsigned(e.alignment);
    std::cout << '\n';
    for (const UiElement& child : e.children) printElement(child, depth + 1);
}

void printLayout(const std::string& path, const UiLayout& layout, bool tracks) {
    std::cout << "# " << path << '\n';
    printElement(layout.root, 0);
    for (const UiStoryboard& board : layout.storyboards) {
        float length = 0;
        for (const UiTrack& track : board.tracks)
            length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
        std::cout << "storyboard " << board.name << " length " << length << " tracks " << board.tracks.size() << '\n';
        if (!tracks) continue;
        for (const UiTrack& track : board.tracks) {
            std::cout << "  track " << track.target << " property " << unsigned(track.property) << " repeat " << track.repeat << " keys";
            for (const UiKeyframe& key : track.keyframes) std::cout << ' ' << key.time << '=' << key.value;
            std::cout << '\n';
        }
    }
}

void printSummary(const std::string& path, const UiLayout& layout) {
    std::vector<std::string> references;
    std::size_t elements = 0;
    std::vector<const UiElement*> stack{&layout.root};
    while (!stack.empty()) {
        const UiElement* e = stack.back();
        stack.pop_back();
        ++elements;
        if (e->type == UiElementType::Reference) references.push_back(e->source);
        for (const UiElement& child : e->children) stack.push_back(&child);
    }
    std::sort(references.begin(), references.end());
    references.erase(std::unique(references.begin(), references.end()), references.end());
    std::cout << path << "\tsize " << layout.root.size[0] << 'x' << layout.root.size[1] << "\telements " << elements
              << "\tstoryboards";
    for (const UiStoryboard& board : layout.storyboards) std::cout << ' ' << board.name;
    std::cout << "\trefs";
    for (const std::string& reference : references) std::cout << ' ' << reference;
    std::cout << '\n';
}

int run(int argc, char** argv) {
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    bool list = false, tracks = false;
    std::vector<std::string> paths;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        strict::forOption(arg, [&] {
            if (arg == "--fib") fib = strict::nextValue(argc, argv, i);
            else if (arg == "--names") names = strict::nextValue(argc, argv, i);
            else if (arg == "--list") list = true;
            else if (arg == "--tracks") tracks = true;
            else if (!arg.empty() && arg[0] == '-') strict::reject("unknown option", arg);
            else paths.push_back(arg);
        });
    }
    if (list == !paths.empty())
        throw std::runtime_error("usage: chase-layoutdump [--fib F] [--names CSV] (--list | [--tracks] sprites/<path>.bxaml...)");
    FuseArchive archive(fib);
    archive.loadNames(names);
    if (list) {
        for (const std::string& name : archive.names())
            if (name.size() > 6 && name.compare(name.size() - 6, 6, ".bxaml") == 0) {
                const FuseEntry* entry = archive.find(name);
                if (!entry) throw std::runtime_error("not in archive: " + name);
                printSummary(name, parseLayout(archive.read(*entry)));
            }
        return 0;
    }
    for (const std::string& path : paths) {
        const FuseEntry* entry = archive.find(path);
        if (!entry) throw std::runtime_error("not in archive: " + path);
        printLayout(path, parseLayout(archive.read(*entry)), tracks);
    }
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-layoutdump: " << error.what() << '\n';
        return 1;
    }
}
