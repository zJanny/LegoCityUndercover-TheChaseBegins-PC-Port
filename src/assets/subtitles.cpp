#include "assets.hpp"
#include <stdexcept>

namespace chase::assets {
namespace {
std::vector<std::string> splitLines(const Bytes& data) {
    std::vector<std::string> lines;
    std::string current;
    std::size_t start = data.size() >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf ? 3 : 0;
    auto flush = [&] {
        if (!current.empty() && current.back() == '\r') current.pop_back();
        lines.push_back(current);
        current.clear();
    };
    for (std::size_t i = start; i < data.size(); ++i) {
        if (data[i] == '\n') flush();
        else current += char(data[i]);
    }
    if (!current.empty()) flush();
    return lines;
}

bool digits(const std::string& text, std::size_t begin, std::size_t count) {
    if (begin + count > text.size()) return false;
    for (std::size_t i = begin; i < begin + count; ++i)
        if (text[i] < '0' || text[i] > '9') return false;
    return true;
}

std::uint32_t number(const std::string& text, std::size_t begin, std::size_t count) {
    std::uint32_t value = 0;
    for (std::size_t i = begin; i < begin + count; ++i) value = value * 10 + std::uint32_t(text[i] - '0');
    return value;
}

std::uint32_t timestamp(const std::string& text, std::size_t at) {
    if (!digits(text, at, 2) || !digits(text, at + 3, 2) || !digits(text, at + 6, 2) || !digits(text, at + 9, 3) ||
        text[at + 2] != ':' || text[at + 5] != ':' || text[at + 8] != ',')
        throw std::runtime_error("malformed subtitle timestamp: " + text);
    std::uint32_t minutes = number(text, at + 3, 2), seconds = number(text, at + 6, 2);
    if (minutes > 59 || seconds > 59) throw std::runtime_error("subtitle timestamp out of range: " + text);
    return ((number(text, at, 2) * 60 + minutes) * 60 + seconds) * 1000 + number(text, at + 9, 3);
}
}

std::vector<SubtitleCue> parseSubtitles(const Bytes& data, const std::vector<std::string>& extended) {
    std::vector<std::string> lines = splitLines(data);
    std::vector<SubtitleCue> cues;
    std::size_t i = 0;
    while (i < lines.size()) {
        if (lines[i].empty()) {
            ++i;
            continue;
        }
        const std::string& indexLine = lines[i];
        if (indexLine.size() > 9 || !digits(indexLine, 0, indexLine.size()))
            throw std::runtime_error("malformed subtitle index: " + indexLine);
        SubtitleCue cue;
        cue.index = unsigned(std::stoul(indexLine));
        if (++i >= lines.size()) throw std::runtime_error("subtitle cue without timestamps");
        const std::string& times = lines[i];
        if (times.size() != 29 || times.compare(12, 5, " --> ") != 0)
            throw std::runtime_error("malformed subtitle time line: " + times);
        cue.startMs = timestamp(times, 0);
        cue.endMs = timestamp(times, 17);
        if (cue.endMs < cue.startMs) throw std::runtime_error("subtitle ends before it starts: " + times);
        for (++i; i < lines.size() && !lines[i].empty(); ++i) cue.lines.push_back(decodeText(lines[i], extended));
        if (cue.lines.empty()) throw std::runtime_error("subtitle cue without text");
        cues.push_back(std::move(cue));
    }
    return cues;
}
}

namespace chase::assets {

std::vector<std::string> wrapSubtitleLines(const std::vector<std::string>& lines, int width,
                                           const std::function<int(const std::string&)>& measure) {
    std::vector<std::string> wrapped;
    for (const std::string& paragraph : lines) {
        std::string line;
        std::size_t lastSpace = std::string::npos;
        for (std::size_t i = 0; i < paragraph.size();) {
            std::size_t next = i + 1;
            while (next < paragraph.size() && (static_cast<unsigned char>(paragraph[next]) & 0xc0) == 0x80) ++next;
            const std::string glyph = paragraph.substr(i, next - i);
            const bool space = glyph == " ";
            if (!space && measure(line + glyph) > width && !line.empty()) {
                if (lastSpace != std::string::npos) {
                    wrapped.push_back(line.substr(0, lastSpace));
                    line = line.substr(lastSpace + 1);
                } else {
                    wrapped.push_back(line);
                    line.clear();
                }
                lastSpace = std::string::npos;
                for (std::size_t j = 0; j < line.size(); ++j)
                    if (line[j] == ' ') lastSpace = j;
            }
            if (space) lastSpace = line.size();
            line += glyph;
            i = next;
        }
        wrapped.push_back(line);
    }
    return wrapped;
}

}
