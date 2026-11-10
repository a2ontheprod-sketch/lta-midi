#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
class PreviewSound final : public juce::SynthesiserSound {
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class PreviewVoice final : public juce::SynthesiserVoice {
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override {
        return dynamic_cast<PreviewSound*>(sound) != nullptr;
    }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
        frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        gain = static_cast<double>(velocity) * 0.115;
        phase = 0;
        auto rate = std::max(1.0, getSampleRate());
        phaseDelta = 2.0 * juce::MathConstants<double>::pi * frequency / rate;
        adsr.setSampleRate(rate);
        adsr.setParameters(shape);
        adsr.reset();
        adsr.noteOn();
    }
    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) adsr.noteOff();
        else { adsr.reset(); clearCurrentNote(); }
    }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override {
        if (!isVoiceActive()) return;
        for (int i = 0; i < numSamples; ++i) {
            const auto envelope = adsr.getNextSample();
            if (!adsr.isActive()) { clearCurrentNote(); break; }
            const auto wave = std::sin(phase) * 0.83 + std::sin(phase * 2.0) * 0.13
                            + std::sin(phase * 3.0) * 0.04;
            const float sample = static_cast<float>(wave * envelope * gain);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, startSample + i, sample);
            phase += phaseDelta;
            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;
        }
    }
private:
    double frequency = 440.0, phase = 0.0, phaseDelta = 0.0, gain = 0.0;
    juce::ADSR adsr;
    juce::ADSR::Parameters shape;
public:
    PreviewVoice() {
        shape.attack = 0.006f; shape.decay = 0.23f;
        shape.sustain = 0.38f; shape.release = 0.16f;
    }
};

uint64_t parseSeed(const juce::String& str, uint64_t fallback) {
    try { return std::stoull(str.toStdString()); }
    catch (...) { return fallback; }
}
}

LTAHarmonyAudioProcessor::LTAHarmonyAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    seeds = {lta::newSeed(), lta::newSeed(), lta::newSeed()};
    for (int i = 0; i < 40; ++i) previewSynth.addVoice(new PreviewVoice());
    previewSynth.addSound(new PreviewSound());
    rebuildPattern();
}

void LTAHarmonyAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;
    previewSynth.setCurrentPlaybackSampleRate(sampleRate);
    previouslyPlaying = false;
    previousExpectedPpq = 0;
    fallbackPpq = 0;
    allNotesOffPending.store(true);
}

void LTAHarmonyAudioProcessor::releaseResources() {
    previewSynth.allNotesOff(0, false);
    allNotesOffPending.store(true);
}

bool LTAHarmonyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void LTAHarmonyAudioProcessor::issueAllNotesOff(juce::MidiBuffer& out) {
    for (int ch = 1; ch <= 3; ++ch) {
        out.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
        out.addEvent(juce::MidiMessage::allSoundOff(ch), 0);
    }
}

void LTAHarmonyAudioProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    midi.clear();
    auto pattern = getPattern();
    if (!pattern || currentSampleRate <= 0) return;

    bool playing = false;
    double startPpq = 0.0, bpm = pattern->tempo;
    if (auto* playHead = getPlayHead()) {
        if (auto info = playHead->getPosition()) {
            playing = info->getIsPlaying();
            if (auto tempo = info->getBpm()) bpm = *tempo;
            if (auto position = info->getPpqPosition()) startPpq = *position;
            else startPpq = fallbackPpq; // host provides playback but not musical position
        }
    }
    if (!std::isfinite(bpm) || bpm <= 0) bpm = pattern->tempo;
    if (!std::isfinite(startPpq)) playing = false;
    hostPlaying.store(playing, std::memory_order_relaxed);

    if (allNotesOffPending.exchange(false) || (previouslyPlaying && !playing)
        || (previouslyPlaying && playing && std::abs(startPpq - previousExpectedPpq) > 0.18)) {
        issueAllNotesOff(midi);
        previewSynth.allNotesOff(0, false);
    }
    previouslyPlaying = playing;
    if (playing) {
        const double beatsPerSample = bpm / (60.0 * currentSampleRate);
        const double finishPpq = startPpq + audio.getNumSamples() * beatsPerSample;
        const double period = static_cast<double>(pattern->bars * 4);
        displayBeat.store(std::fmod(std::fmod(startPpq, period) + period, period), std::memory_order_relaxed);

        const auto schedule = [&](const std::vector<lta::Note>& notes, int channel) {
            for (const auto& note : notes) {
                const auto emit = [&](double atBeat, bool on) {
                    const long long first = static_cast<long long>(std::floor((startPpq - atBeat) / period)) - 1;
                    const long long last = static_cast<long long>(std::floor((finishPpq - atBeat) / period)) + 1;
                    for (long long cycle = first; cycle <= last && cycle < first + 5; ++cycle) {
                        double absolute = atBeat + cycle * period;
                        if (absolute < startPpq - 0.0000001 || absolute >= finishPpq - 0.0000001) continue;
                        int sample = juce::jlimit(0, audio.getNumSamples() - 1,
                                          static_cast<int>(std::round((absolute - startPpq) / beatsPerSample)));
                        midi.addEvent(on ? juce::MidiMessage::noteOn(channel, note.pitch,
                                                static_cast<juce::uint8>(note.velocity))
                                         : juce::MidiMessage::noteOff(channel, note.pitch), sample);
                    }
                };
                emit(note.beat, true);
                emit(note.beat + note.length, false);
            }
        };
        schedule(pattern->harmony, 1);
        schedule(pattern->topline, 2);
        schedule(pattern->bass, 3);
        previousExpectedPpq = finishPpq;
        fallbackPpq = finishPpq;
    } else {
        fallbackPpq = 0.0;
        displayBeat.store(0.0, std::memory_order_relaxed);
    }

    if (audition.load(std::memory_order_relaxed))
        previewSynth.renderNextBlock(audio, midi, 0, audio.getNumSamples());
    else previewSynth.allNotesOff(0, false);
}

juce::AudioProcessorEditor* LTAHarmonyAudioProcessor::createEditor() {
    return new LTAHarmonyAudioProcessorEditor(*this);
}

lta::Settings LTAHarmonyAudioProcessor::getSettings() const {
    const juce::ScopedLock lock(settingsLock);
    return settings;
}

void LTAHarmonyAudioProcessor::setSettings(const lta::Settings& next) {
    const juce::ScopedLock lock(settingsLock);
    settings = next;
    rebuildPattern();
}

void LTAHarmonyAudioProcessor::regenerate(lta::Lane lane, bool respectLocks) {
    const juce::ScopedLock lock(settingsLock);
    if ((lane == lta::Lane::Chords || lane == lta::Lane::All) && (!respectLocks || !locks[0])) seeds.chords = lta::newSeed();
    if ((lane == lta::Lane::Topline || lane == lta::Lane::All) && (!respectLocks || !locks[1])) seeds.topline = lta::newSeed();
    if ((lane == lta::Lane::Bass || lane == lta::Lane::All) && (!respectLocks || !locks[2])) seeds.bass = lta::newSeed();
    rebuildPattern();
}

void LTAHarmonyAudioProcessor::rebuildPattern() {
    std::shared_ptr<const lta::Pattern> next = std::make_shared<const lta::Pattern>(lta::generate(settings, seeds));
    std::atomic_store_explicit(&currentPattern, std::move(next), std::memory_order_release);
    allNotesOffPending.store(true);
}

std::shared_ptr<const lta::Pattern> LTAHarmonyAudioProcessor::getPattern() const {
    return std::atomic_load_explicit(&currentPattern, std::memory_order_acquire);
}

bool LTAHarmonyAudioProcessor::exportMidi(const juce::File& file, lta::Lane lane) const {
    auto pattern = getPattern();
    return pattern && lta::saveMidi(*pattern, lane, file.getFullPathName().toStdString());
}

bool LTAHarmonyAudioProcessor::getLocked(lta::Lane lane) const {
    const auto index = static_cast<int>(lane);
    const juce::ScopedLock lock(settingsLock);
    return index >= 0 && index < 3 ? locks[index] : false;
}

void LTAHarmonyAudioProcessor::setLocked(lta::Lane lane, bool value) {
    const int index = static_cast<int>(lane);
    if (index >= 0 && index < 3) {
        const juce::ScopedLock lock(settingsLock);
        locks[index] = value;
    }
}

void LTAHarmonyAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    const juce::ScopedLock lock(settingsLock);
    juce::XmlElement xml("LTAHarmonyState");
    xml.setAttribute("root", settings.root);
    xml.setAttribute("scale", settings.scale);
    xml.setAttribute("mood", settings.mood);
    xml.setAttribute("genre", settings.genre);
    xml.setAttribute("bars", settings.bars);
    xml.setAttribute("tempo", settings.tempo);
    xml.setAttribute("density", settings.density);
    xml.setAttribute("complexity", settings.complexity);
    xml.setAttribute("swing", settings.swing);
    xml.setAttribute("chordSeed", juce::String(std::to_string(seeds.chords)));
    xml.setAttribute("melodySeed", juce::String(std::to_string(seeds.topline)));
    xml.setAttribute("bassSeed", juce::String(std::to_string(seeds.bass)));
    for (int i = 0; i < 3; ++i) xml.setAttribute("lock" + juce::String(i), locks[i]);
    xml.setAttribute("audition", audition.load());
    copyXmlToBinary(xml, dest);
}

void LTAHarmonyAudioProcessor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || !xml->hasTagName("LTAHarmonyState")) return;
    const juce::ScopedLock lock(settingsLock);
    settings.root = xml->getIntAttribute("root", 0);
    settings.scale = xml->getIntAttribute("scale", 1);
    settings.mood = xml->getIntAttribute("mood", 0);
    settings.genre = xml->getIntAttribute("genre", 0);
    settings.bars = xml->getIntAttribute("bars", 8);
    settings.tempo = xml->getIntAttribute("tempo", 140);
    settings.density = xml->getIntAttribute("density", 58);
    settings.complexity = xml->getIntAttribute("complexity", 50);
    settings.swing = xml->getIntAttribute("swing", 14);
    seeds.chords = parseSeed(xml->getStringAttribute("chordSeed"), 1);
    seeds.topline = parseSeed(xml->getStringAttribute("melodySeed"), 2);
    seeds.bass = parseSeed(xml->getStringAttribute("bassSeed"), 3);
    for (int i = 0; i < 3; ++i) locks[i] = xml->getBoolAttribute("lock" + juce::String(i), false);
    audition.store(xml->getBoolAttribute("audition", true));
    rebuildPattern();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new LTAHarmonyAudioProcessor();
}
