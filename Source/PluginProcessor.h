#pragma once
#include <JuceHeader.h>
#include "MusicEngine.h"
#include "MidiWriter.h"
#include <atomic>
#include <memory>

class LTAHarmonyAudioProcessor final : public juce::AudioProcessor {
public:
    LTAHarmonyAudioProcessor();
    ~LTAHarmonyAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    lta::Settings getSettings() const;
    void setSettings(const lta::Settings& newSettings);
    void regenerate(lta::Lane lane, bool respectLocks = true);
    std::shared_ptr<const lta::Pattern> getPattern() const;
    bool exportMidi(const juce::File&, lta::Lane) const;
    bool getLocked(lta::Lane lane) const;
    void setLocked(lta::Lane lane, bool locked);

    void setAudition(bool on) { audition.store(on); }
    bool getAudition() const { return audition.load(); }
    double getDisplayBeat() const { return displayBeat.load(); }
    bool getHostPlaying() const { return hostPlaying.load(); }

private:
    void rebuildPattern(); // caller holds settingsLock
    void issueAllNotesOff(juce::MidiBuffer& midi);

    mutable juce::CriticalSection settingsLock;
    lta::Settings settings;
    lta::Seeds seeds;
    bool locks[3] = {false, false, false};
    std::shared_ptr<const lta::Pattern> currentPattern;
    std::atomic<bool> audition {true};
    std::atomic<bool> allNotesOffPending {false};
    std::atomic<double> displayBeat {0.0};
    std::atomic<bool> hostPlaying {false};
    bool previouslyPlaying = false;
    double previousExpectedPpq = 0.0;
    double fallbackPpq = 0.0;
    double currentSampleRate = 44100.0;
    juce::Synthesiser previewSynth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTAHarmonyAudioProcessor)
};
