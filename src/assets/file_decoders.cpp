#include "assets/file_decoders.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::assets {
namespace {
struct Rule {
    const char* suffix;
    const char* decoder;
};

constexpr Rule kRules[] = {
    {".btga", "decodeTexture"},
    {".bpng", "decodeTexture"},
    {".bdds", "decodeTexture"},
    {".bfnmdl", "parseModel"},
    {".bfnanm", "parseAnimation"},
    {".bfnskl", "parseSkeleton"},
    {".bfnnav", "parseNav"},
    {".bwav", "decodeSound"},
    {".blvl", "parseLevel"},
    {".loc", "parseLocalisation"},
    {".bstxt", "parseManifest"},
    {".bxaml", "parseLayout"},
    {".bxls", "parseTable"},
    {".srt", "parseSubtitles"},
    {".tnf", "parseFont"},
    {".bprt", "parseParticleDefs"},
    {".cam", "parseConfigText"},
    {"localisation/extended_chars_utf8.txt", "extendedCharacters"},
    {"options3ds.txt", "parseConfigText"},
    {"levels/triggers.txt", "parseConfigText"},
    {"levels/letriggers.txt", "parseConfigText"},
};

bool endsWith(const std::string& text, const char* suffix) {
    const std::size_t length = std::strlen(suffix);
    return text.size() >= length && text.compare(text.size() - length, length, suffix) == 0;
}
}

std::string fileDecoder(const std::string& archivePath) {
    std::string path = archivePath;
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    for (const Rule& rule : kRules)
        if (endsWith(path, rule.suffix)) return rule.decoder;
    return {};
}

}
