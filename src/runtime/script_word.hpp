#pragma once
// Natives that test their argument as a 32-bit word (`ldr r0, [r0]; cmp r0, #0`, for example ShowDialogBG 0x1bfb6c and
// BlackoutMap 0x1b23fc) see -0.0 as set. No SDL dependency.
#include <cstdint>
#include <cstring>

namespace chase::runtime {

inline bool scriptWordIsSet(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits != 0;
}

}
