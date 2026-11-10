#include "MusicEngine.h"
#include "MidiWriter.h"
#include <cassert>
#include <iostream>
#include <fstream>

int main() {
    for (int scale = 0; scale < 7; ++scale) {
        for (int mood = 0; mood < 12; ++mood) {
            for (int genre = 0; genre < 14; ++genre) {
                lta::Settings s;
                s.root = (scale * 3 + mood) % 12;
                s.scale = scale;
                s.mood = mood;
                s.genre = genre;
                s.bars = mood % 3 == 0 ? 4 : mood % 3 == 1 ? 8 : 16;
                s.density = 13 + mood * 7;
                s.complexity = 5 + genre * 6;
                auto p = lta::generate(s, {456, static_cast<uint64_t>(700 + mood), static_cast<uint64_t>(900 + genre)});
                assert(static_cast<int>(p.chords.size()) == s.bars);
                assert(!p.harmony.empty() && !p.topline.empty() && !p.bass.empty());
                for (const auto* lane : {&p.harmony, &p.topline, &p.bass})
                    for (const auto& note : *lane) {
                        assert(note.pitch >= 0 && note.pitch <= 127);
                        assert(note.beat >= 0 && note.beat < s.bars * 4.0);
                        assert(note.length > 0 && note.beat + note.length < s.bars * 4.0 + 0.0001);
                    }
                for (auto lane : {lta::Lane::Chords, lta::Lane::Topline, lta::Lane::Bass, lta::Lane::All}) {
                    auto bytes = lta::makeMidi(p, lane);
                    assert(bytes.size() > 60);
                    assert(bytes[0] == 'M' && bytes[1] == 'T' && bytes[2] == 'h' && bytes[3] == 'd');
                    assert(bytes[8] == 0 && bytes[9] == (lane == lta::Lane::All ? 1 : 0));
                    assert(bytes[11] == (lane == lta::Lane::All ? 4 : 1));
                    assert(bytes[13] == 0xE0); // PPQ 480
                }
                auto repeat = lta::generate(s, {456, static_cast<uint64_t>(700 + mood), static_cast<uint64_t>(900 + genre)});
                assert(lta::makeMidi(p, lta::Lane::All) == lta::makeMidi(repeat, lta::Lane::All));
            }
        }
    }
    std::cout << "PASS: 1176 genre/scale/mood combinations x 4 MIDI files; deterministic generation.\n";
}
