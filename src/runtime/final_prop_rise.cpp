#include "runtime/final_prop_rise.hpp"

namespace chase::runtime {

bool FinalPropRise::writes(float rise) {
    const bool write = rise != 0.0f || last_ != 0.0f;
    last_ = rise;
    return write;
}

}
