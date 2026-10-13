#pragma once
// Which archive files have a C++ decoder. One table for the progress report (tools/progress_report.py).
// No SDL dependency.
#include <string>

namespace chase::assets {

// Name of the chase_assets function that decodes the archive file at `archivePath` (for example "decodeTexture"), or an
// empty string when no decoder exists. Names are matched case-insensitively.
std::string fileDecoder(const std::string& archivePath);

}
