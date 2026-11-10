#pragma once
#include "MusicEngine.h"
#include <cstdint>
#include <string>
#include <vector>

namespace lta {
enum class Lane { Chords, Topline, Bass, All };
std::vector<uint8_t> makeMidi(const Pattern& pattern, Lane lane);
bool saveMidi(const Pattern& pattern, Lane lane, const std::string& filePath);
}
