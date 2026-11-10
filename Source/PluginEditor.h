#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>

class LTAFlatLook final : public juce::LookAndFeel_V4 {
public:
    LTAFlatLook();
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
};

class LTANoteLane final : public juce::Component {
public:
    void setNotes(const std::vector<lta::Note>& newNotes, int numberOfBars, juce::Colour noteColour);
    void setPlaybackPosition(double beat, bool playing);
    void paint(juce::Graphics&) override;
private:
    std::vector<lta::Note> notes;
    int bars = 8;
    double playBeat = 0;
    bool playing = false;
    juce::Colour accent = juce::Colour(0xfffa75a1);
};

class LTAMidiDragPad final : public juce::Component {
public:
    void setText(const juce::String& title) { caption = title; repaint(); }
    std::function<void(juce::Component*)> onDrag;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void paint(juce::Graphics&) override;
private:
    juce::String caption;
    bool pressed = false, started = false;
};

class LTAHarmonyAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer {
public:
    explicit LTAHarmonyAudioProcessorEditor(LTAHarmonyAudioProcessor&);
    ~LTAHarmonyAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void onSettingsChanged();
    void refreshFromProcessor(bool refreshInputs);
    void startMidiDrag(lta::Lane, juce::Component*);
    void saveLane(lta::Lane);
    void saveAll();
    void setStatus(const juce::String& status);

    LTAHarmonyAudioProcessor& processor;
    LTAFlatLook look;
    juce::ComboBox keyBox, scaleBox, moodBox, genreBox, barsBox;
    juce::Slider bpmSlider, densitySlider, complexitySlider, swingSlider;
    juce::TextButton newIdeaButton, saveAllButton, auditionButton;
    std::array<juce::TextButton, 3> regenerateButtons, lockButtons, saveButtons;
    std::array<LTAMidiDragPad, 3> dragPads;
    std::array<LTANoteLane, 3> noteLanes;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String statusText = "READY / DRAG MIDI INTO YOUR DAW";
    uint64_t displayedIdea = 0;
    bool syncInProgress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LTAHarmonyAudioProcessorEditor)
};
