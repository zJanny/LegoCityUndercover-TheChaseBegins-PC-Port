#include "assets/assets.hpp"
#include "assets/path_name_hash.hpp"

#include <cstdio>

using namespace chase::assets;

namespace {
int failures = 0;
void require(bool ok, const char* what) {
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}
}

int main() {
    // 0x23895c, read by 20261107-traffic-D (llvm-objdump): the part after the last '.', the whole name when there is no '.' or the only '.' is first.
    require(pathLookupName("lvl.Path01") == "Path01", "one dot");
    require(pathLookupName("a.b.Path01") == "Path01", "last dot");
    require(pathLookupName("Path01") == "Path01", "no dot");
    require(pathLookupName(".Path01") == ".Path01", "leading dot only");
    require(pathLookupName("a.") == "", "trailing dot");
    require(pathLookupName("") == "", "empty");
    require(pathNameHash("scene.Path01") == nameHash("Path01"), "hash of the suffix");
    require(pathNameHash("Path01") == nameHash("Path01"), "hash without dot");
    require(pathNameHash("PATH01") == nameHash("path01"), "case folded like nameHash");
    return failures == 0 ? 0 : 1;
}
