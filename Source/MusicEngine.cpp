#include "MusicEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <tuple>

namespace lta {
namespace {
using Rng = std::mt19937_64;
constexpr std::array<std::array<int, 7>, 7> scaleTable {{
    {{0, 2, 4, 5, 7, 9, 11}},  // major
    {{0, 2, 3, 5, 7, 8, 10}}, // natural minor
    {{0, 2, 3, 5, 7, 8, 11}}, // harmonic minor
    {{0, 2, 3, 5, 7, 9, 10}}, // dorian
    {{0, 1, 3, 5, 7, 8, 10}}, // phrygian
    {{0, 2, 4, 6, 7, 9, 11}}, // lydian
    {{0, 2, 4, 5, 7, 9, 10}}  // mixolydian
}};

int randInt(Rng& rng, int min, int max) { return std::uniform_int_distribution<int>(min, max)(rng); }
bool chance(Rng& rng, int percent) { return randInt(rng, 0, 99) < std::clamp(percent, 0, 100); }
int wrap(int n, int mod) { return (n % mod + mod) % mod; }
const std::array<int, 7>& notesOf(const Settings& s) { return scaleTable[static_cast<size_t>(std::clamp(s.scale, 0, 6))]; }
int degreePc(const Settings& s, int degree) { return wrap(s.root + notesOf(s)[static_cast<size_t>(wrap(degree, 7))], 12); }
int closestPitchClass(int pitchClass, int preferred) {
    int note = preferred + wrap(pitchClass - preferred, 12);
    if (note - preferred > 6) note -= 12;
    return std::clamp(note, 0, 127);
}
std::string chordName(const Settings& s, int degree, bool seventh, bool ninth) {
    const auto& sc = notesOf(s);
    const int a = sc[static_cast<size_t>(wrap(degree, 7))];
    const int b = sc[static_cast<size_t>(wrap(degree + 2, 7))] + ((degree + 2 >= 7) ? 12 : 0);
    const int c = sc[static_cast<size_t>(wrap(degree + 4, 7))] + ((degree + 4 >= 7) ? 12 : 0);
    const int third = b - a, fifth = c - a;
    std::string quality = third == 3 ? "m" : "";
    if (fifth == 6) quality = "dim";
    if (fifth == 8) quality = "aug";
    if (seventh) {
        const int d = sc[static_cast<size_t>(wrap(degree + 6, 7))] + ((degree + 6 >= 7) ? 12 : 0);
        const int seventhInterval = d - a;
        if (third == 4 && seventhInterval == 11) quality = "maj7";
        else if (third == 3 && seventhInterval == 11) quality = "m(maj7)";
        else if (fifth == 6) quality = "m7b5";
        else quality += "7";
    }
    if (ninth) quality += "(9)";
    return roots()[static_cast<size_t>(degreePc(s, degree))] + quality;
}

std::vector<int> makeVoicing(const Settings& s, int degree, bool addSeventh, bool addNinth,
                             const std::vector<int>& previous, Rng& rng) {
    std::vector<int> intervals {0, 2, 4};
    if (addSeventh) intervals.push_back(6);
    if (addNinth) intervals.push_back(8);
    std::vector<int> raw;
    for (int index : intervals) {
        const int absoluteDegree = degree + index;
        raw.push_back(48 + s.root + notesOf(s)[static_cast<size_t>(absoluteDegree % 7)]
                      + 12 * (absoluteDegree / 7));
    }
    std::vector<int> best;
    double bestScore = std::numeric_limits<double>::infinity();
    for (int inversion = 0; inversion < 3; ++inversion) {
        for (int octave = -1; octave <= 1; ++octave) {
            auto test = raw;
            for (int k = 0; k < inversion; ++k) test[static_cast<size_t>(k)] += 12;
            for (int& pitch : test) pitch += octave * 12;
            std::sort(test.begin(), test.end());
            if (test.front() < 45 || test.back() > 90) continue;
            double score = std::abs(test.front() - 56) * 1.5;
            if (!previous.empty()) {
                for (int p : test) {
                    int minDist = 99;
                    for (int last : previous) minDist = std::min(minDist, std::abs(p - last));
                    score += minDist * 1.8;
                }
                score += std::abs(test.front() - previous.front()) * 0.5;
            }
            score += randInt(rng, 0, 20) * 0.15;
            if (score < bestScore) { bestScore = score; best = test; }
        }
    }
    if (best.empty()) best = raw;
    return best;
}

std::array<int, 4> chooseProgression(const Settings& s, Rng& rng) {
    static const std::array<std::array<int,4>, 11> major {{
        {{0, 4, 5, 3}}, {{0, 5, 3, 4}}, {{5, 3, 0, 4}}, {{0, 3, 4, 0}},
        {{0, 2, 5, 3}}, {{3, 0, 4, 5}}, {{5, 1, 4, 0}}, {{0, 3, 1, 4}},
        {{0, 4, 3, 3}}, {{0, 2, 3, 4}}, {{3, 4, 0, 5}}
    }};
    static const std::array<std::array<int,4>, 12> minor {{
        {{0, 5, 2, 6}}, {{0, 3, 6, 2}}, {{0, 6, 5, 6}}, {{0, 4, 5, 3}},
        {{0, 2, 5, 6}}, {{5, 6, 0, 0}}, {{0, 5, 3, 6}}, {{0, 3, 5, 4}},
        {{0, 6, 2, 5}}, {{0, 4, 3, 6}}, {{5, 3, 0, 6}}, {{0, 1, 6, 4}}
    }};
    bool majorish = s.scale == 0 || s.scale == 5 || s.scale == 6;
    // Gritty / dark moods tend to avoid the straightforward major I-V-vi-IV.
    int index = majorish ? randInt(rng, 0, static_cast<int>(major.size()) - 1)
                         : randInt(rng, 0, static_cast<int>(minor.size()) - 1);
    if ((s.mood == 0 || s.mood == 1 || s.mood == 9) && !majorish) index = randInt(rng, 0, 7);
    if (s.mood == 4 && majorish) index = randInt(rng, 0, 4);
    return majorish ? major[static_cast<size_t>(index)] : minor[static_cast<size_t>(index)];
}

void applySwing(Note& note, const Settings& s, Rng& rng) {
    const double eighth = note.beat * 2.0;
    if (std::abs(eighth - std::round(eighth)) < 0.005 && static_cast<int>(std::round(eighth)) % 2 == 1)
        note.beat += (s.swing / 100.0) * 0.23;
    // velocity humanisation, no random timing drift that could cause negative beats
    note.velocity = std::clamp(note.velocity + randInt(rng, -5, 5), 35, 125);
}

void pushNote(std::vector<Note>& lane, int pitch, double beat, double duration, int velocity,
              const Settings& s, Rng& rng) {
    Note n {std::clamp(pitch, 0, 127), std::max(0.0, beat), std::max(0.07, duration), velocity};
    applySwing(n, s, rng);
    n.length = std::min(n.length, static_cast<double>(s.bars * 4) - n.beat - 0.015);
    if (n.length > 0.06) lane.push_back(n);
}

// Pick a chord tone / scale tone near the preceding note with an optional high-energy leap.
int melodyPitch(const Settings& s, const Chord& chord, int previous, bool strong, Rng& rng) {
    const int target = (s.genre == 1 || s.mood == 4) ? 81 :
                       ((s.mood == 1 || s.mood == 3 || s.genre == 2) ? 73 : 76);
    std::vector<int> candidates;
    for (int oct = 5; oct <= 7; ++oct) {
        for (int d = 0; d < 7; ++d) {
            const int p = 12 * oct + degreePc(s, d);
            if (p < 62 || p > 96) continue;
            bool chordTone = false;
            for (int c : chord.pitches) if (wrap(c, 12) == wrap(p, 12)) chordTone = true;
            if (strong && !chordTone) continue;
            candidates.push_back(p);
        }
    }
    if (candidates.empty()) return 76;
    double best = -1.e9;
    int answer = candidates.front();
    for (int c : candidates) {
        const int movement = std::abs(c - previous);
        double score = -std::abs(c - target) * 0.55 - movement * 1.1 + randInt(rng, 0, 65) * 0.34;
        if (movement == 0) score += 4;
        if (movement >= 2 && movement <= 5) score += 6;
        if (movement > 9) score -= 16;
        if (s.genre == 1 && movement >= 7) score += 8;
        if (score > best) { best = score; answer = c; }
    }
    return answer;
}

void generateMelody(Pattern& p, const Settings& s, Rng& rng) {
    const int density = std::clamp(s.density, 0, 100);
    const int notesPerBar = std::clamp(2 + density / 18 + (s.genre == 1 ? 2 : 0)
                                    - (s.genre == 10 ? 2 : 0), 1, 10);
    std::array<std::vector<double>, 4> rhythmMotif;
    for (int m = 0; m < 4; ++m) {
        const double pulse = s.genre == 6 || s.genre == 7 ? 0.5 :
                             (s.genre == 1 || s.genre == 11) ? 0.25 : 0.5;
        for (double pos = 0; pos < 4.0; pos += pulse) {
            int probability = std::min(91, static_cast<int>(notesPerBar * pulse * 24));
            if (pos == 0.0) probability += 27;
            if (std::fmod(pos, 1.0) > 0.0 && s.genre == 2) probability -= 13;
            if (chance(rng, probability)) rhythmMotif[static_cast<size_t>(m)].push_back(pos);
        }
        if (rhythmMotif[static_cast<size_t>(m)].empty()) rhythmMotif[static_cast<size_t>(m)].push_back(0);
    }
    int lastPitch = 75;
    std::array<int, 4> motifDelta {{0, 0, 0, 0}};
    for (int m = 0; m < 4; ++m) motifDelta[static_cast<size_t>(m)] = randInt(rng, -2, 2);
    for (int bar = 0; bar < p.bars; ++bar) {
        const auto& chord = p.chords[static_cast<size_t>(bar)];
        const auto& motif = rhythmMotif[static_cast<size_t>(bar % 4)];
        for (size_t i = 0; i < motif.size(); ++i) {
            const double pos = motif[i];
            const bool strong = std::fmod(pos, 1.0) < 0.01 || chance(rng, 40);
            int pitch = melodyPitch(s, chord, lastPitch, strong, rng);
            if (bar >= 4 && chance(rng, 65)) {
                // Reuse short ideas, but diatonically re-harmonise to the active chord.
                pitch = melodyPitch(s, chord, lastPitch + motifDelta[static_cast<size_t>(bar % 4)], strong, rng);
            }
            double gap = (i + 1 < motif.size()) ? motif[i+1] - pos : 4.0 - pos;
            double length = std::clamp(gap * (s.genre == 10 ? 0.98 : 0.80), 0.13, 2.7);
            if (s.mood == 1 || s.mood == 2 || s.genre == 10) length = std::min(3.1, gap * 0.97);
            if (chance(rng, 12 + s.complexity / 8)) length *= 0.52;
            pushNote(p.topline, pitch, bar * 4.0 + pos, length,
                     81 + (strong ? 9 : 0) + randInt(rng, -8, 8), s, rng);
            lastPitch = pitch;
        }
    }
}

void generateBass(Pattern& p, const Settings& s, Rng& rng) {
    for (int bar = 0; bar < p.bars; ++bar) {
        const int root = p.chords[static_cast<size_t>(bar)].root;
        const int desired = (s.genre == 7 || s.genre == 10) ? 44 : 36;
        int fundamental = closestPitchClass(root, desired);
        if (fundamental < 28) fundamental += 12;
        const bool bounce = s.genre == 1 || s.genre == 4 || s.genre == 5 || s.genre == 11;
        const bool house = s.genre == 8;
        const bool legato = s.genre == 2 || s.genre == 10 || s.mood == 2;
        std::vector<double> onsets {0.0};
        if (house) onsets = {0.0, 1.0, 2.0, 3.0};
        else if (bounce) {
            if (chance(rng, 85)) onsets.push_back(1.5);
            if (chance(rng, 75)) onsets.push_back(2.0);
            if (chance(rng, 70)) onsets.push_back(3.5);
            if (s.density > 60 && chance(rng, 55)) onsets.push_back(2.75);
        } else if (legato) {
            if (chance(rng, 35)) onsets.push_back(2.5);
        } else {
            if (chance(rng, 75)) onsets.push_back(2.0);
            if (s.density > 45 && chance(rng, 55)) onsets.push_back(3.0);
        }
        std::sort(onsets.begin(), onsets.end());
        for (size_t i = 0; i < onsets.size(); ++i) {
            double pos = onsets[i];
            double gap = i + 1 < onsets.size() ? onsets[i+1] - pos : 4.0 - pos;
            int pitch = fundamental;
            if (i > 0 && bounce && chance(rng, 20 + s.complexity / 4)) pitch += 12;
            else if (i > 0 && s.genre == 7 && chance(rng, 30)) {
                bool hasPerfectFifth = false;
                for (int note : p.chords[static_cast<size_t>(bar)].pitches)
                    if (wrap(note - root, 12) == 7) hasPerfectFifth = true;
                if (hasPerfectFifth) pitch += 7;
            }
            double gate = house ? 0.68 : (legato ? 0.98 : 0.83);
            pushNote(p.bass, pitch, bar * 4.0 + pos, std::max(0.12, gap * gate),
                     93 + randInt(rng, -12, 6), s, rng);
        }
    }
}
} // namespace

const std::vector<std::string>& roots() { static const std::vector<std::string> v {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return v; }
const std::vector<std::string>& scales() { static const std::vector<std::string> v {
    "Major", "Natural minor", "Harmonic minor", "Dorian", "Phrygian", "Lydian", "Mixolydian" }; return v; }
const std::vector<std::string>& moods() { static const std::vector<std::string> v {
    "Dark", "Melancholic", "Dreamy", "Nostalgic", "Euphoric", "Tense",
    "Romantic", "Gritty", "Hopeful", "Mysterious", "Peaceful", "Playful" }; return v; }
const std::vector<std::string>& genres() { static const std::vector<std::string> v {
    "UK underground", "Rage", "Cloud rap", "Emo trap", "Trap", "Drill", "R&B",
    "Neo soul", "House", "Pop", "Ambient", "Detroit", "Indie", "Lo-fi" }; return v; }

uint64_t newSeed() {
    static std::random_device device;
    const uint64_t a = static_cast<uint64_t>(device());
    const uint64_t b = static_cast<uint64_t>(device());
    return (a << 32) ^ b ^ 0x9e3779b97f4a7c15ULL;
}

Pattern generate(const Settings& original, const Seeds& seeds) {
    Settings s = original;
    s.root = std::clamp(s.root, 0, 11);
    s.scale = std::clamp(s.scale, 0, 6);
    s.genre = std::clamp(s.genre, 0, static_cast<int>(genres().size()) - 1);
    s.mood = std::clamp(s.mood, 0, static_cast<int>(moods().size()) - 1);
    s.bars = s.bars == 4 || s.bars == 8 || s.bars == 16 ? s.bars : 8;
    s.density = std::clamp(s.density, 0, 100);
    s.complexity = std::clamp(s.complexity, 0, 100);
    s.swing = std::clamp(s.swing, 0, 60);
    s.tempo = std::clamp(s.tempo, 50, 240);
    Pattern p;
    p.bars = s.bars;
    p.tempo = s.tempo;
    p.ideaId = seeds.chords ^ (seeds.topline << 1) ^ (seeds.bass >> 1);
    Rng chordsRng(seeds.chords), melodyRng(seeds.topline), bassRng(seeds.bass);
    auto progression = chooseProgression(s, chordsRng);
    std::vector<int> last;
    for (int bar = 0; bar < s.bars; ++bar) {
        int degree = progression[static_cast<size_t>(bar % 4)];
        // Second/third/fourth group of four repeats motifs, with occasional turnaround.
        if (bar >= 4 && bar % 4 == 3 && chance(chordsRng, 30 + s.complexity / 3))
            degree = chance(chordsRng, 50) ? 4 : 6;
        bool add7 = (s.genre == 6 || s.genre == 7 || s.genre == 9 || s.genre == 13)
                        ? chance(chordsRng, 48 + s.complexity / 3)
                        : chance(chordsRng, 12 + s.complexity / 2);
        bool add9 = add7 && chance(chordsRng, s.complexity / 3 + (s.genre == 7 ? 25 : 0));
        Chord c;
        c.degree = degree;
        c.root = degreePc(s, degree);
        c.name = chordName(s, degree, add7, add9);
        c.pitches = makeVoicing(s, degree, add7, add9, last, chordsRng);
        last = c.pitches;
        for (int pitch : c.pitches) {
            pushNote(p.harmony, pitch, bar * 4.0, 3.95,
                     81 + randInt(chordsRng, -10, 9), s, chordsRng);
        }
        p.chords.push_back(std::move(c));
    }
    generateMelody(p, s, melodyRng);
    generateBass(p, s, bassRng);
    return p;
}

} // namespace lta
