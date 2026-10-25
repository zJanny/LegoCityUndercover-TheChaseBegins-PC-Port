// Front end against the real game data: texts, wrapping, exact logo pixels, page renders and the two movies.
// Skips (exit 77) when the extracted RomFS is missing. Usage: frontend-real-test FIB CSV CUTSCENE_DIR
#include "frontend/frontend.hpp"
#include "frontend/layout.hpp"
#include "frontend/movie_host.hpp"
#include "frontend/script.hpp"
#include "movie/movie.hpp"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {
using namespace chase;
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

template <typename T, typename U> void requireEqual(const T& actual, const U& expected, const std::string& what) {
    if (!(actual == expected)) {
        std::cerr << "FAIL: " << what << ": got " << actual << ", expected " << expected << '\n';
        ++failures;
    }
}

std::uint32_t fnv1a(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t hash = 2166136261u;
    for (const std::uint8_t byte : bytes) hash = (hash ^ byte) * 16777619u;
    return hash;
}

void texts(Resources& resources) {
    // Strings read on the oracle screens (docs/re/front-end.md) and from the decompiled page builders.
    const std::string game = "localisation/gametext_uk.loc", system = "localisation/systemtext_uk.loc";
    requireEqual(resources.textForKey(game, 0x4617a2ca), std::string("Activate StreetPass for\nLEGO\xC2\xAE City Undercover: The Chase Begins?"),
                 "StreetPass question (oracle screenshot the save state at the StreetPass question)");
    requireEqual(resources.textForKey(game, 0x65213108), std::string("Pressing the HOME Button will allow you to return to the HOME Menu."), "HOME notice");
    requireEqual(resources.textForKey(system, 0xe2117397), std::string("Select Slot"), "slot header");
    requireEqual(resources.textForKey(system, 0xcbafe6a8), std::string("Saving Game..."), "saving header");
    requireEqual(resources.textForKey("localisation/hinttext_uk.loc", 0x42ef3429), std::string("Doughnut Disaster"), "mission title");
    requireEqual(resources.textForKey(game, kLanguageRowKeys[0]), std::string("English"), "first language row");
    requireEqual(resources.textForKey(game, kLanguageRowKeys[8]), std::string("Portugu\xC3\xAA" "s"), "last language row");

    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    // The autosave notice wraps into three lines on the oracle screen (the save state at the autosave notice) at the 296 px limit.
    const std::vector<std::string> lines = wrapText(font, extended, resources.textForKey(game, 0xa5e2f63f), 296, 0);
    requireEqual(lines.size(), std::size_t(3), "autosave notice lines");
    if (lines.size() == 3) {
        requireEqual(lines[0], std::string("This game uses an autosave feature."), "autosave line 1");
        requireEqual(lines[1], std::string("While you see this icon, do not power off"), "autosave line 2");
        requireEqual(lines[2], std::string("the system."), "autosave line 3");
    }
}

// Provenance: line breaks of the intro cues in oracle pictures intro00 (Excellent...),
// intro03/04 (I'm sure...), intro06 (We're out of doughnuts...) and intro09 (Part two...); box heights 48 (one line) and 72 (two lines).
void subtitleBreaks(Resources& resources) {
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    const auto cues = assets::parseSubtitles(resources.bytes("cutscenes/01_gameintro_us.srt"), extended);
    const auto wrapped = [&](const std::string& begin) {
        for (const auto& cue : cues)
            if (!cue.lines.empty() && cue.lines[0].rfind(begin, 0) == 0)
                return assets::wrapSubtitleLines(cue.lines, assets::subtitleWrapWidthStandIn(kBottomWidth),
                                                 [&](const std::string& text) { return textWidth(font, extended, text, 0); });
        return std::vector<std::string>{};
    };
    using Lines = std::vector<std::string>;
    require(wrapped("Excellent, we're") == Lines{"Excellent, we're all very happy to have", "you here."}, "subtitle break: Excellent");
    require(wrapped("I'm sure he has") == Lines{"I'm sure he has some important work", "for you."}, "subtitle break: I'm sure");
    require(wrapped("We're out of") == Lines{"We're out of doughnuts. Get me some", "more."}, "subtitle break: doughnuts");
    require(wrapped("Part two.") == Lines{"Part two. I think I've made a bad", "career choice."}, "subtitle break: Part two");
    require(wrapped("Thrilled.") == Lines{"Thrilled."}, "subtitle one line");
}

void logoPixels(Resources& resources) {
    FrontEnd front(resources, nullptr, {});
    Canvas top(kTopWidth, kScreenHeight), bottom(kBottomWidth, kScreenHeight);
    runScript(front, "until logos 20; run 1", {});
    front.draw(top, bottom);
    const assets::Image& up = resources.image("sprites/splashscreens/splash2top.btga");
    const assets::Image& down = resources.image("sprites/splashscreens/splash2bottom_uk.btga");
    bool same = true;
    for (int y = 0; y < kScreenHeight && same; ++y)
        for (int x = 0; x < kTopWidth && same; ++x)
            for (int c = 0; c < 3; ++c) same = same && top.pixel(x, y)[c] == up.rgba[(std::size_t(y) * up.width + x) * 4 + c];
    require(same, "top logo screen equals splash2top pixel for pixel");
    same = true;
    for (int y = 0; y < kScreenHeight && same; ++y)
        for (int x = 0; x < kBottomWidth && same; ++x)
            for (int c = 0; c < 3; ++c) same = same && bottom.pixel(x, y)[c] == down.rgba[(std::size_t(y) * down.width + x) * 4 + c];
    require(same, "bottom logo screen equals splash2bottom_uk pixel for pixel");
}

void pageRenders(Resources& resources) {
    FrontEnd front(resources, nullptr, {});
    Canvas top(kTopWidth, kScreenHeight), bottom(kBottomWidth, kScreenHeight);
    std::map<std::string, std::uint32_t> hashes;
    ScriptHooks hooks;
    hooks.shot = [&](const std::string& name) {
        front.draw(top, bottom);
        hashes[name + "-top"] = fnv1a(top.rgba());
        hashes[name + "-bottom"] = fnv1a(bottom.rgba());
    };
    runScript(front,
              "until language 40; run 5; shot language; press A; run 5; shot streetpass; press B; run 5; shot home; press A; run 5; shot autosave;"
              "press A; run 5; shot slots; press DOWN; run 5; shot slots2",
              hooks);
    // Recorded with chase-frontshot at the commit that added this test; the renders agree with the oracle screenshots
    // of the same screens (mean abs diff over both screens 0.12 language, 0.36 StreetPass, 0.17 HOME, 0.33 autosave, 1.4 slot list).
    const std::map<std::string, std::uint32_t> expected = {
        {"language-bottom", 0xe9c9af04}, {"streetpass-bottom", 0xb104cab2}, {"home-bottom", 0x2792c7d1}, {"autosave-bottom", 0xce41d9c1},
        {"slots-bottom", 0x7eda0668}, {"slots-top", 0x6e54e1b5}, {"slots2-bottom", 0x2cf9403}};
    for (const auto& [name, value] : expected) {
        const auto found = hashes.find(name);
        require(found != hashes.end(), "render " + name);
        if (found != hashes.end()) requireEqual(found->second, value, "render hash " + name);
    }
    require(hashes["streetpass-top"] == hashes["home-top"], "prompt pages leave the top screen black");
}

void movies(const std::string& directory) {
    // docs/re/fmv.md: splash5 has 191 video frames in 6.36 s (30 fps); ffprobe: 01_gameintro has 4088 frames at 59.94 fps = 68.2 s.
    const std::pair<const char*, std::pair<unsigned, unsigned>> cases[] = {{"splash5", {189, 194}}, {"01_gameintro", {2040, 2052}}};
    for (const auto& [name, range] : cases) {
        MoviePictureHost host(directory);
        require(host.open(name, "uk"), std::string("open ") + name);
        unsigned updates = 0;
        while (host.advance(1.0 / 30.0) && updates < 5000) ++updates;
        require(updates >= range.first && updates <= range.second,
                std::string(name) + " plays in " + std::to_string(updates) + " updates at 30 per second, expected " +
                    std::to_string(range.first) + ".." + std::to_string(range.second));
        require(host.frame().width == 400 && host.frame().height == 240, std::string(name) + " is 400x240");
        require(!host.takeAudio().empty(), std::string(name) + " has audio");
    }
}

void eyePairs(const std::string& directory) {
    // docs/re/fmv.md 2026-10-13T20:58:38: layout byte 0 = left/right eye frames alternate, the original shows the first of
    // each pair (oracle intro captures match even frames); ffprobe: 01_gameintro 4088 frames, splash5 191 (layout 6).
    const std::tuple<const char*, bool, std::size_t, double> cases[] = {{"splash5", false, 191, 1.0 / 30},
                                                                        {"01_gameintro", true, 2044, 2002.0 / 60000}};
    for (const auto& [name, pairs, frames, period] : cases) {
        movie::Decoder decoder(directory + "/" + name + ".moflex", movie::PixelFormat::Yuv420);
        require(decoder.eyePairs() == pairs, std::string(name) + " eye-pair layout");
        std::deque<movie::VideoFrame> video;
        std::vector<std::uint8_t> audio;
        while (decoder.readMore(video, audio)) audio.clear();
        require(video.size() == frames, std::string(name) + " shows " + std::to_string(video.size()) + " pictures");
        require(std::abs(video[1].time - video[0].time - period) < 1e-6, std::string(name) + " picture period");
    }
}

}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: frontend-real-test FIB CSV CUTSCENE_DIR\n";
        return EXIT_FAILURE;
    }
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
        std::cout << "skipped: game data not extracted\n";
        return 77;
    }
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    Resources resources(archive);
    texts(resources);
    subtitleBreaks(resources);
    logoPixels(resources);
    pageRenders(resources);
    if (std::ifstream(std::string(argv[3]) + "/splash5.moflex").good()) {
        movies(argv[3]);
        eyePairs(argv[3]);
    }
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "frontend real data ok\n";
    return EXIT_SUCCESS;
}
