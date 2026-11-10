#include "MidiWriter.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <tuple>

namespace lta {
namespace {
constexpr int ppq = 480;
void be16(std::vector<uint8_t>& x, uint32_t n) { x.push_back(static_cast<uint8_t>(n >> 8)); x.push_back(static_cast<uint8_t>(n)); }
void be32(std::vector<uint8_t>& x, uint32_t n) { for (int k = 24; k >= 0; k -= 8) x.push_back(static_cast<uint8_t>(n >> k)); }
void vlq(std::vector<uint8_t>& x, uint32_t v) {
    uint8_t bytes[5]; int n = 0;
    bytes[n++] = static_cast<uint8_t>(v & 0x7F);
    while ((v >>= 7) != 0) bytes[n++] = static_cast<uint8_t>((v & 0x7F) | 0x80);
    while (n) x.push_back(bytes[--n]);
}
void addChunk(std::vector<uint8_t>& result, const std::vector<uint8_t>& content) {
    result.insert(result.end(), {'M', 'T', 'r', 'k'});
    be32(result, static_cast<uint32_t>(content.size()));
    result.insert(result.end(), content.begin(), content.end());
}
void meta(std::vector<uint8_t>& track, uint32_t delta, uint8_t type, const std::vector<uint8_t>& data) {
    vlq(track, delta); track.push_back(0xFF); track.push_back(type);
    vlq(track, static_cast<uint32_t>(data.size()));
    track.insert(track.end(), data.begin(), data.end());
}
std::vector<uint8_t> tempoTrack(int bpm) {
    std::vector<uint8_t> out;
    int micro = 60000000 / std::max(1, bpm);
    meta(out, 0, 0x03, {'T','e','m','p','o'});
    meta(out, 0, 0x51, {static_cast<uint8_t>(micro >> 16), static_cast<uint8_t>(micro >> 8), static_cast<uint8_t>(micro)});
    meta(out, 0, 0x58, {4, 2, 24, 8});
    meta(out, 0, 0x2F, {});
    return out;
}
struct Event { uint32_t tick; uint8_t status; uint8_t pitch; uint8_t velocity; };
std::vector<uint8_t> noteTrack(const std::vector<Note>& notes, int channel, const char* name, int maxBars, int bpm = 0) {
    std::vector<Event> events;
    events.reserve(notes.size() * 2);
    for (const auto& n : notes) {
        const uint32_t start = static_cast<uint32_t>(std::max(0.0, std::round(n.beat * ppq)));
        const uint32_t end = static_cast<uint32_t>(std::max(static_cast<double>(start + 1),
                               std::round(std::min(n.beat + n.length, maxBars * 4.0) * ppq)));
        events.push_back({start, static_cast<uint8_t>(0x90 | channel), static_cast<uint8_t>(n.pitch), static_cast<uint8_t>(n.velocity)});
        events.push_back({end, static_cast<uint8_t>(0x80 | channel), static_cast<uint8_t>(n.pitch), 0});
    }
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return (a.status & 0xF0) < (b.status & 0xF0); // note-off before note-on
    });
    std::vector<uint8_t> out;
    const std::string title(name);
    meta(out, 0, 0x03, std::vector<uint8_t>(title.begin(), title.end()));
    if (bpm > 0) {
        const int micro = 60000000 / bpm;
        meta(out, 0, 0x51, {static_cast<uint8_t>(micro >> 16), static_cast<uint8_t>(micro >> 8), static_cast<uint8_t>(micro)});
        meta(out, 0, 0x58, {4, 2, 24, 8});
    }
    uint32_t last = 0;
    for (const auto& e : events) {
        vlq(out, e.tick - last);
        out.push_back(e.status); out.push_back(e.pitch); out.push_back(e.velocity);
        last = e.tick;
    }
    meta(out, 0, 0x2F, {});
    return out;
}
}
std::vector<uint8_t> makeMidi(const Pattern& p, Lane lane) {
    const bool all = lane == Lane::All;
    std::vector<uint8_t> out;
    out.insert(out.end(), {'M','T','h','d'});
    be32(out, 6);
    be16(out, all ? 1 : 0); // individual: type 0; combined: type 1
    be16(out, all ? 4 : 1);
    be16(out, ppq);
    if (all) {
        addChunk(out, tempoTrack(p.tempo));
        addChunk(out, noteTrack(p.harmony, 0, "LTA Chords", p.bars));
        addChunk(out, noteTrack(p.topline, 1, "LTA Topline", p.bars));
        addChunk(out, noteTrack(p.bass, 2, "LTA Bass", p.bars));
    } else if (lane == Lane::Chords) {
        addChunk(out, noteTrack(p.harmony, 0, "LTA Chords", p.bars, p.tempo));
    } else if (lane == Lane::Topline) {
        addChunk(out, noteTrack(p.topline, 0, "LTA Topline", p.bars, p.tempo));
    } else if (lane == Lane::Bass) {
        addChunk(out, noteTrack(p.bass, 0, "LTA Bass", p.bars, p.tempo));
    }
    return out;
}
bool saveMidi(const Pattern& p, Lane lane, const std::string& filePath) {
    const auto data = makeMidi(p, lane);
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return file.good();
}
}
