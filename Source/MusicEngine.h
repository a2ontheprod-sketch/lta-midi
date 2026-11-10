#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lta {

struct Note {
    int pitch = 60; // MIDI note 0..127
    double beat = 0.0; // quarter notes, relative to pattern start
    double length = 1.0;
    int velocity = 90;
};

struct Chord {
    int root = 0;              // pitch class
    int degree = 0;            // 0..6
    std::string name;
    std::vector<int> pitches;  // actual MIDI voicing
};

struct Pattern {
    std::vector<Chord> chords;
    std::vector<Note> harmony;
    std::vector<Note> topline;
    std::vector<Note> bass;
    int bars = 8;
    int tempo = 140;
    uint64_t ideaId = 0;
};

struct Settings {
    int root = 0;             // C=0, ... B=11
    int scale = 1;            // 0 Major; 1 Natural Minor; 2 Harmonic Minor; etc
    int mood = 0;             // see moods()
    int genre = 0;            // see genres()
    int bars = 8;             // 4, 8, 16
    int tempo = 140;
    int density = 58;         // 0..100
    int complexity = 50;      // 0..100
    int swing = 14;          // 0..60 (percentage of 8th-note shift)
};

struct Seeds {
    uint64_t chords = 1;
    uint64_t topline = 2;
    uint64_t bass = 3;
};

const std::vector<std::string>& roots();
const std::vector<std::string>& scales();
const std::vector<std::string>& moods();
const std::vector<std::string>& genres();
uint64_t newSeed();
Pattern generate(const Settings&, const Seeds&);

} // namespace lta
