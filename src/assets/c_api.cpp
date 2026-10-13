#include "assets/assets.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
thread_local std::string lastError;

template <typename Result, typename Function>
Result guarded(Result failure, Function function) {
    try {
        return function();
    } catch (const std::exception& error) {
        lastError = error.what();
        return failure;
    }
}

chase::assets::Bytes bytesOf(const std::uint8_t* data, std::size_t size) {
    return chase::assets::Bytes(data, data + size);
}

struct Texture {
    chase::assets::Image image;
};
struct SoundHandle {
    chase::assets::Sound sound;
};
}

extern "C" {
const char* chase_last_error() { return lastError.c_str(); }

int chase_skin_matrices(const std::uint8_t* skeletonData, std::size_t skeletonSize, const std::uint8_t* animationData,
                        std::size_t animationSize, double frame, float* out, std::size_t capacity) {
    try {
        using namespace chase::assets;
        Skeleton skeleton = parseSkeleton(Bytes(skeletonData, skeletonData + skeletonSize));
        Animation animation = parseAnimation(Bytes(animationData, animationData + animationSize));
        std::vector<Matrix> skin = skinMatrices(skeleton, animation, frame);
        if (skin.size() * 16 > capacity) throw std::runtime_error("output buffer too small");
        for (std::size_t i = 0; i < skin.size(); ++i) std::memcpy(out + 16 * i, skin[i].data(), sizeof(Matrix));
        return int(skin.size());
    } catch (const std::exception& error) {
        lastError = error.what();
        return -1;
    }
}

void* chase_fib_open(const char* path) {
    return guarded<void*>(nullptr, [&] { return new chase::assets::FuseArchive(path); });
}

void chase_fib_close(void* archive) { delete static_cast<chase::assets::FuseArchive*>(archive); }

std::size_t chase_fib_count(const void* archive) {
    return static_cast<const chase::assets::FuseArchive*>(archive)->entries().size();
}

int chase_fib_entry(const void* archive, std::size_t index, std::uint32_t* out) {
    const auto& entries = static_cast<const chase::assets::FuseArchive*>(archive)->entries();
    if (index >= entries.size()) return -1;
    out[0] = entries[index].crc;
    out[1] = entries[index].offset;
    out[2] = entries[index].size;
    out[3] = entries[index].flags;
    return 0;
}

int chase_fib_read(void* archive, std::size_t index, std::uint8_t* out, std::size_t capacity,
                   std::uint64_t* storedSize) {
    return guarded(-1, [&] {
        auto* fib = static_cast<chase::assets::FuseArchive*>(archive);
        if (index >= fib->entries().size()) throw std::runtime_error("entry index out of range");
        chase::assets::Bytes data = fib->read(fib->entries()[index], storedSize);
        if (data.size() > capacity) throw std::runtime_error("output buffer too small");
        std::memcpy(out, data.data(), data.size());
        return 0;
    });
}

void* chase_texture_decode(const std::uint8_t* data, std::size_t size) {
    return guarded<void*>(nullptr, [&] { return new Texture{chase::assets::decodeTexture(bytesOf(data, size))}; });
}

void chase_texture_info(const void* texture, std::int32_t* out) {
    const auto& image = static_cast<const Texture*>(texture)->image;
    out[0] = image.width;
    out[1] = image.height;
    out[2] = image.format;
    out[3] = image.faces;
    out[4] = image.levels;
}

const std::uint8_t* chase_texture_rgba(const void* texture) {
    return static_cast<const Texture*>(texture)->image.rgba.data();
}

void chase_texture_free(void* texture) { delete static_cast<Texture*>(texture); }

void* chase_sound_decode(const std::uint8_t* data, std::size_t size) {
    return guarded<void*>(nullptr, [&] { return new SoundHandle{chase::assets::decodeSound(bytesOf(data, size))}; });
}

void chase_sound_info(const void* handle, std::uint32_t* out) {
    const auto& sound = static_cast<const SoundHandle*>(handle)->sound;
    out[0] = std::uint32_t(sound.rate);
    out[1] = sound.loop ? 1 : 0;
    out[2] = sound.loopStart;
    out[3] = sound.frames;
    out[4] = std::uint32_t(sound.channels.size());
    out[5] = std::uint32_t(sound.encodings.size());
    out[6] = sound.version;
}

int chase_sound_encoding(const void* handle, std::size_t part) {
    const auto& encodings = static_cast<const SoundHandle*>(handle)->sound.encodings;
    return part < encodings.size() ? encodings[part] : -1;
}

const std::int16_t* chase_sound_channel(const void* handle, std::size_t channel) {
    const auto& channels = static_cast<const SoundHandle*>(handle)->sound.channels;
    return channel < channels.size() ? channels[channel].data() : nullptr;
}

void chase_sound_free(void* handle) { delete static_cast<SoundHandle*>(handle); }

std::uint32_t chase_text_key(const char* name) { return chase::assets::textKey(name); }

typedef void (*chase_text_entry_fn)(std::uint32_t key, const char* utf8, void* user);

int chase_loc_decode(const std::uint8_t* data, std::size_t size, const std::uint8_t* extended,
                     std::size_t extendedSize, chase_text_entry_fn callback, void* user) {
    return guarded(-1, [&] {
        auto characters = chase::assets::extendedCharacters(bytesOf(extended, extendedSize));
        for (const auto& [key, raw] : chase::assets::parseLocalisation(bytesOf(data, size)))
            callback(key, chase::assets::decodeText(raw, characters).c_str(), user);
        return 0;
    });
}

int chase_font_layout(const std::uint8_t* font, std::size_t fontSize, const char* utf8, const std::uint8_t* extended,
                      std::size_t extendedSize, std::int32_t* out, std::size_t capacity) {
    return guarded(-1, [&] {
        chase::assets::Font parsed = chase::assets::parseFont(bytesOf(font, fontSize));
        auto characters = chase::assets::extendedCharacters(bytesOf(extended, extendedSize));
        auto placed = chase::assets::layoutText(parsed, utf8, characters);
        if (placed.size() * 5 + 1 > capacity) throw std::runtime_error("output buffer too small");
        out[0] = parsed.lineHeight;
        for (std::size_t i = 0; i < placed.size(); ++i) {
            out[1 + 5 * i] = placed[i].x;
            out[2 + 5 * i] = placed[i].glyph.x;
            out[3 + 5 * i] = placed[i].glyph.y;
            out[4 + 5 * i] = placed[i].glyph.width;
            out[5 + 5 * i] = placed[i].glyph.height;
        }
        return int(placed.size());
    });
}

typedef void (*chase_level_object_fn)(const char* className, const char* instance, const char* room, void* user);
typedef void (*chase_level_param_fn)(std::uint32_t hash, std::uint16_t type, std::uint32_t value, std::size_t blockCount,
                                     const std::uint8_t* const* blocks, const std::size_t* sizes, void* user);

int chase_level_walk(const std::uint8_t* data, std::size_t size, chase_level_object_fn onObject,
                     chase_level_param_fn onParam, void* user) {
    return guarded(-1, [&] {
        chase::assets::Level level = chase::assets::parseLevel(bytesOf(data, size));
        for (const auto& object : level.objects) {
            onObject(object.className.c_str(), object.instance.c_str(), object.room.c_str(), user);
            for (const auto& param : object.params) {
                std::vector<const std::uint8_t*> blocks;
                std::vector<std::size_t> sizes;
                for (const auto& block : param.blocks) {
                    blocks.push_back(block.data());
                    sizes.push_back(block.size());
                }
                onParam(param.hash, param.type, param.value, blocks.size(), blocks.data(), sizes.data(), user);
            }
        }
        return int(level.objects.size());
    });
}
}
