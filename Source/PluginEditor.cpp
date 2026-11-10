#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour ink {0xff1a2030};
const juce::Colour muted {0xff788093};
const juce::Colour background {0xfff7f7fa};
const juce::Colour border {0xffe5e7ed};
const juce::Colour pink {0xffee6799};
const juce::Colour violet {0xff8c85e7};
const juce::Colour teal {0xff43b8b0};
const std::array<juce::Colour, 3> laneColours {pink, violet, teal};
const std::array<juce::String, 3> laneTitles {"01   CHORDS", "02   TOPLINE", "03   BASS"};
const std::array<juce::String, 3> fileNames {"LTA Harmony - Chords.mid", "LTA Harmony - Topline.mid", "LTA Harmony - Bass.mid"};

void paintLabel(juce::Graphics& g, const juce::String& text, int x, int y, int width,
                int height, juce::Colour colour, float size, bool bold = false) {
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)));
    g.drawFittedText(text, x, y, width, height, juce::Justification::centredLeft, 1);
}
}

LTAFlatLook::LTAFlatLook() {
    setColour(juce::ComboBox::backgroundColourId, juce::Colours::white);
    setColour(juce::ComboBox::textColourId, ink);
    setColour(juce::ComboBox::outlineColourId, border);
    setColour(juce::ComboBox::arrowColourId, muted);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colours::white);
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, pink.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId, ink);
    setColour(juce::Slider::thumbColourId, pink);
    setColour(juce::Slider::trackColourId, pink);
    setColour(juce::Slider::backgroundColourId, border);
    setColour(juce::Slider::textBoxTextColourId, ink);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
    setColour(juce::TextButton::textColourOffId, ink);
}

void LTAFlatLook::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                       const juce::Colour& baseColour, bool hover, bool down) {
    const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    auto fill = baseColour;
    if (down) fill = fill.darker(0.1f);
    else if (hover) fill = fill.interpolatedWith(juce::Colours::white, 0.12f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, 10.0f);
}

void LTAFlatLook::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                int, int, int, int, juce::ComboBox&) {
    const auto r = juce::Rectangle<float>(0, 0, static_cast<float>(width), static_cast<float>(height));
    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(r.reduced(0.6f), 9.0f);
    g.setColour(border);
    g.drawRoundedRectangle(r.reduced(0.6f), 9.0f, 1.1f);
    juce::Path arrow;
    arrow.addTriangle(width - 21.0f, height * 0.44f, width - 10.0f, height * 0.44f,
                      width - 15.5f, height * 0.58f);
    g.setColour(muted);
    g.fillPath(arrow);
}

void LTANoteLane::setNotes(const std::vector<lta::Note>& newNotes, int count, juce::Colour colour) {
    notes = newNotes;
    bars = std::max(1, count);
    accent = colour;
    repaint();
}

void LTANoteLane::setPlaybackPosition(double beat, bool active) {
    if (playing != active || std::abs(playBeat - beat) > 0.045) {
        playBeat = beat;
        playing = active;
        repaint();
    }
}

void LTANoteLane::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xfff4f5f9));
    g.fillRoundedRectangle(r, 8.0f);
    if (notes.empty()) return;
    int minNote = 127, maxNote = 0;
    for (auto& n : notes) { minNote = std::min(minNote, n.pitch); maxNote = std::max(maxNote, n.pitch); }
    minNote = std::max(0, minNote - 2);
    maxNote = std::min(127, maxNote + 2);
    if (maxNote - minNote < 8) maxNote = std::min(127, minNote + 8);
    const float left = 5.0f, top = 5.0f;
    const float usefulWidth = r.getWidth() - 10.0f;
    const float usefulHeight = r.getHeight() - 10.0f;
    const double totalBeats = bars * 4.0;
    for (int b = 0; b <= bars * 4; ++b) {
        const float x = left + usefulWidth * static_cast<float>(b / totalBeats);
        g.setColour((b % 4 == 0) ? juce::Colour(0xffdaddE7) : juce::Colour(0xffe9ebf1));
        g.drawVerticalLine(static_cast<int>(x), top, top + usefulHeight);
    }
    for (int pitch = minNote; pitch <= maxNote; ++pitch) {
        if (pitch % 2 != 0) continue;
        const float y = top + usefulHeight * (1.0f - static_cast<float>(pitch - minNote) / (maxNote - minNote + 1));
        g.setColour(juce::Colour(0xffe9ebf1));
        g.drawHorizontalLine(static_cast<int>(y), left, left + usefulWidth);
    }
    for (const auto& note : notes) {
        const float x = left + usefulWidth * static_cast<float>(note.beat / totalBeats);
        const float width = std::max(2.0f, usefulWidth * static_cast<float>(note.length / totalBeats) - 1.1f);
        const float y = top + usefulHeight * (1.0f - static_cast<float>(note.pitch - minNote + 0.5f) / (maxNote - minNote + 1));
        g.setColour(accent);
        g.fillRoundedRectangle(x, y - 2.8f, width, 5.6f, 2.0f);
    }
    if (playing) {
        const float x = left + usefulWidth * static_cast<float>(playBeat / totalBeats);
        g.setColour(ink.withAlpha(0.6f));
        g.drawVerticalLine(static_cast<int>(x), top, top + usefulHeight);
    }
}

void LTAMidiDragPad::mouseDown(const juce::MouseEvent&) { pressed = true; started = false; }
void LTAMidiDragPad::mouseDrag(const juce::MouseEvent& e) {
    if (pressed && !started && e.getDistanceFromDragStart() > 5) {
        started = true;
        if (onDrag) onDrag(this);
    }
}
void LTAMidiDragPad::mouseUp(const juce::MouseEvent&) { pressed = false; started = false; }
void LTAMidiDragPad::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xffffedf3));
    g.fillRoundedRectangle(r, 9.0f);
    g.setColour(pink.withAlpha(0.4f));
    g.drawRoundedRectangle(r.reduced(0.5f), 9.0f, 1.0f);
    paintLabel(g, ":::  " + caption, 13, 7, getWidth() - 26, getHeight() - 14,
               juce::Colour(0xffb33267), 13.0f, true);
}

LTAHarmonyAudioProcessorEditor::LTAHarmonyAudioProcessorEditor(LTAHarmonyAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&look);
    setOpaque(true);
    setSize(1140, 785);
    setResizable(true, true);
    setResizeLimits(1020, 720, 1600, 1040);

    auto initCombo = [this](juce::ComboBox& combo, const std::vector<std::string>& values) {
        for (int i = 0; i < static_cast<int>(values.size()); ++i)
            combo.addItem(juce::String(values[static_cast<size_t>(i)]), i + 1);
        combo.onChange = [this] { onSettingsChanged(); };
        addAndMakeVisible(combo);
    };
    initCombo(keyBox, lta::roots());
    initCombo(scaleBox, lta::scales());
    initCombo(moodBox, lta::moods());
    initCombo(genreBox, lta::genres());
    barsBox.addItem("4 bars", 1); barsBox.addItem("8 bars", 2); barsBox.addItem("16 bars", 3);
    barsBox.onChange = [this] { onSettingsChanged(); };
    addAndMakeVisible(barsBox);

    auto initSlider = [this](juce::Slider& s, double min, double max, double step) {
        s.setRange(min, max, step);
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 48, 22);
        s.onValueChange = [this] { onSettingsChanged(); };
        addAndMakeVisible(s);
    };
    initSlider(bpmSlider, 50, 240, 1);
    initSlider(densitySlider, 0, 100, 1);
    initSlider(complexitySlider, 0, 100, 1);
    initSlider(swingSlider, 0, 60, 1);

    newIdeaButton.setButtonText("+  GENERATE NEW IDEA");
    newIdeaButton.setColour(juce::TextButton::buttonColourId, pink);
    newIdeaButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    newIdeaButton.onClick = [this] { processor.regenerate(lta::Lane::All); refreshFromProcessor(false); };
    addAndMakeVisible(newIdeaButton);

    saveAllButton.setButtonText("EXPORT ALL 4 MIDI FILES");
    saveAllButton.setColour(juce::TextButton::buttonColourId, ink);
    saveAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    saveAllButton.onClick = [this] { saveAll(); };
    addAndMakeVisible(saveAllButton);

    auditionButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff343b4d));
    auditionButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    auditionButton.onClick = [this] {
        processor.setAudition(!processor.getAudition());
        refreshFromProcessor(false);
    };
    addAndMakeVisible(auditionButton);

    for (int i = 0; i < 3; ++i) {
        auto lane = static_cast<lta::Lane>(i);
        regenerateButtons[static_cast<size_t>(i)].setButtonText("REMAKE ↻");
        regenerateButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId, juce::Colour(0xfff2f2f7));
        regenerateButtons[static_cast<size_t>(i)].onClick = [this, lane] {
            processor.regenerate(lane, false);
            refreshFromProcessor(false);
        };
        addAndMakeVisible(regenerateButtons[static_cast<size_t>(i)]);

        lockButtons[static_cast<size_t>(i)].setButtonText("LOCK");
        lockButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId, juce::Colour(0xfff2f2f7));
        lockButtons[static_cast<size_t>(i)].onClick = [this, lane] {
            processor.setLocked(lane, !processor.getLocked(lane));
            refreshFromProcessor(false);
        };
        addAndMakeVisible(lockButtons[static_cast<size_t>(i)]);

        saveButtons[static_cast<size_t>(i)].setButtonText("OR CLICK TO SAVE .MID");
        saveButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId, juce::Colour(0xfff0f1f5));
        saveButtons[static_cast<size_t>(i)].onClick = [this, lane] { saveLane(lane); };
        addAndMakeVisible(saveButtons[static_cast<size_t>(i)]);

        dragPads[static_cast<size_t>(i)].setText("DRAG " + juce::String(i == 0 ? "CHORDS" : i == 1 ? "TOPLINE" : "BASS") + " MIDI");
        dragPads[static_cast<size_t>(i)].onDrag = [this, lane](juce::Component* source) { startMidiDrag(lane, source); };
        addAndMakeVisible(dragPads[static_cast<size_t>(i)]);
        addAndMakeVisible(noteLanes[static_cast<size_t>(i)]);
    }
    refreshFromProcessor(true);
    startTimerHz(16);
}

LTAHarmonyAudioProcessorEditor::~LTAHarmonyAudioProcessorEditor() {
    stopTimer();
    fileChooser.reset();
    setLookAndFeel(nullptr);
}

void LTAHarmonyAudioProcessorEditor::onSettingsChanged() {
    if (syncInProgress) return;
    auto s = processor.getSettings();
    s.root = std::max(0, keyBox.getSelectedId() - 1);
    s.scale = std::max(0, scaleBox.getSelectedId() - 1);
    s.mood = std::max(0, moodBox.getSelectedId() - 1);
    s.genre = std::max(0, genreBox.getSelectedId() - 1);
    const int barsId = barsBox.getSelectedId();
    s.bars = barsId == 1 ? 4 : barsId == 3 ? 16 : 8;
    s.tempo = static_cast<int>(bpmSlider.getValue());
    s.density = static_cast<int>(densitySlider.getValue());
    s.complexity = static_cast<int>(complexitySlider.getValue());
    s.swing = static_cast<int>(swingSlider.getValue());
    processor.setSettings(s);
    refreshFromProcessor(false);
}

void LTAHarmonyAudioProcessorEditor::refreshFromProcessor(bool refreshInputs) {
    const auto s = processor.getSettings();
    if (refreshInputs) {
        syncInProgress = true;
        keyBox.setSelectedId(s.root + 1, juce::dontSendNotification);
        scaleBox.setSelectedId(s.scale + 1, juce::dontSendNotification);
        moodBox.setSelectedId(s.mood + 1, juce::dontSendNotification);
        genreBox.setSelectedId(s.genre + 1, juce::dontSendNotification);
        barsBox.setSelectedId(s.bars == 4 ? 1 : s.bars == 16 ? 3 : 2, juce::dontSendNotification);
        bpmSlider.setValue(s.tempo, juce::dontSendNotification);
        densitySlider.setValue(s.density, juce::dontSendNotification);
        complexitySlider.setValue(s.complexity, juce::dontSendNotification);
        swingSlider.setValue(s.swing, juce::dontSendNotification);
        syncInProgress = false;
    }
    auto pattern = processor.getPattern();
    if (pattern) {
        displayedIdea = pattern->ideaId;
        noteLanes[0].setNotes(pattern->harmony, pattern->bars, laneColours[0]);
        noteLanes[1].setNotes(pattern->topline, pattern->bars, laneColours[1]);
        noteLanes[2].setNotes(pattern->bass, pattern->bars, laneColours[2]);
    }
    for (int i = 0; i < 3; ++i) {
        const bool locked = processor.getLocked(static_cast<lta::Lane>(i));
        lockButtons[static_cast<size_t>(i)].setButtonText(locked ? "LOCKED" : "LOCK");
        lockButtons[static_cast<size_t>(i)].setColour(juce::TextButton::buttonColourId,
                                                      locked ? pink.withAlpha(0.2f) : juce::Colour(0xfff2f2f7));
    }
    auditionButton.setButtonText(processor.getAudition() ? "AUDITION AUDIO: ON" : "AUDITION AUDIO: OFF");
    repaint();
}

void LTAHarmonyAudioProcessorEditor::setStatus(const juce::String& s) {
    statusText = s;
    repaint();
}

void LTAHarmonyAudioProcessorEditor::startMidiDrag(lta::Lane lane, juce::Component* source) {
    const juce::String label = lane == lta::Lane::Chords ? "chords" : lane == lta::Lane::Topline ? "topline" : "bass";
    const auto name = "lta-harmony-" + label + "-" + juce::String(juce::Time::getMillisecondCounter()) + ".mid";
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("LTA-Harmony-MIDI");
    if (!dir.createDirectory().wasOk()) {
        setStatus("COULD NOT PREPARE THE MIDI TEMP FOLDER"); return;
    }
    auto temp = dir.getChildFile(name);
    if (!processor.exportMidi(temp, lane)) { setStatus("MIDI FILE COULD NOT BE CREATED"); return; }
    if (juce::DragAndDropContainer::performExternalDragDropOfFiles(juce::StringArray {temp.getFullPathName()}, false, source))
        setStatus("DRAG MIDI INTO AN INSTRUMENT'S PIANO ROLL");
    else setStatus("DRAG NOT SUPPORTED BY HOST - USE SAVE .MID INSTEAD");
}

void LTAHarmonyAudioProcessorEditor::saveLane(lta::Lane lane) {
    const int index = static_cast<int>(lane);
    auto destination = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                      .getChildFile(fileNames[static_cast<size_t>(index)]);
    fileChooser = std::make_unique<juce::FileChooser>("Save LTA Harmony MIDI", destination, "*.mid");
    juce::Component::SafePointer<LTAHarmonyAudioProcessorEditor> weak(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [weak, lane](const juce::FileChooser& chooser) {
            if (auto* editor = weak.getComponent()) {
                auto file = chooser.getResult();
                if (file == juce::File()) return;
                if (!file.hasFileExtension("mid")) file = file.withFileExtension(".mid");
                editor->setStatus(editor->processor.exportMidi(file, lane) ? "MIDI SAVED: " + file.getFileName()
                                                                 : "COULD NOT SAVE MIDI");
            }
        });
}

void LTAHarmonyAudioProcessorEditor::saveAll() {
    fileChooser = std::make_unique<juce::FileChooser>("Choose a folder for all LTA Harmony MIDI files",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
    juce::Component::SafePointer<LTAHarmonyAudioProcessorEditor> weak(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [weak](const juce::FileChooser& chooser) {
            if (auto* editor = weak.getComponent()) {
                auto folder = chooser.getResult();
                if (!folder.isDirectory()) return;
                bool ok = true;
                for (int i = 0; i < 3; ++i)
                    ok = editor->processor.exportMidi(folder.getChildFile(fileNames[static_cast<size_t>(i)]), static_cast<lta::Lane>(i)) && ok;
                ok = editor->processor.exportMidi(folder.getChildFile("LTA Harmony - All Tracks.mid"), lta::Lane::All) && ok;
                editor->setStatus(ok ? "SAVED: CHORDS, TOPLINE, BASS AND ALL-TRACKS MIDI" : "SOME MIDI FILES FAILED TO SAVE");
            }
        });
}

void LTAHarmonyAudioProcessorEditor::timerCallback() {
    auto pattern = processor.getPattern();
    if (pattern && displayedIdea != pattern->ideaId) refreshFromProcessor(true);
    const auto playPosition = processor.getDisplayBeat();
    const bool playing = processor.getHostPlaying();
    for (auto& lane : noteLanes) lane.setPlaybackPosition(playPosition, playing);
}

void LTAHarmonyAudioProcessorEditor::resized() {
    // Layout is scaled as a unit, keeping text and drag areas aligned at smaller window sizes.
    const float sx = getWidth() / 1140.0f;
    const float sy = getHeight() / 785.0f;
    const auto put = [sx, sy](juce::Component& c, int x, int y, int w, int h) {
        c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy),
                    juce::roundToInt(w * sx), juce::roundToInt(h * sy));
    };
    put(newIdeaButton, 846, 38, 250, 48);
    put(auditionButton, 26, 438, 171, 35);
    put(keyBox, 254, 146, 88, 41);
    put(scaleBox, 351, 146, 147, 41);
    put(moodBox, 508, 146, 146, 41);
    put(genreBox, 664, 146, 187, 41);
    put(barsBox, 861, 146, 93, 41);
    put(bpmSlider, 965, 146, 134, 41);
    for (int i = 0; i < 3; ++i) {
        const int y = 336 + 126 * i;
        put(regenerateButtons[static_cast<size_t>(i)], 270, y + 53, 105, 31);
        put(lockButtons[static_cast<size_t>(i)], 384, y + 53, 65, 31);
        put(noteLanes[static_cast<size_t>(i)], 464, y + 12, 401, 83);
        put(dragPads[static_cast<size_t>(i)], 879, y + 9, 205, 49);
        put(saveButtons[static_cast<size_t>(i)], 879, y + 67, 205, 28);
    }
    put(complexitySlider, 255, 733, 180, 33);
    put(densitySlider, 464, 733, 174, 33);
    put(swingSlider, 660, 733, 166, 33);
    put(saveAllButton, 858, 726, 236, 43);
}

void LTAHarmonyAudioProcessorEditor::paint(juce::Graphics& g) {
    const float sx = getWidth() / 1140.0f;
    const float sy = getHeight() / 785.0f;
    g.fillAll(background);
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(sx, sy));
    g.setColour(ink);
    g.fillRect(0, 0, 222, 785);
    g.setColour(pink);
    g.fillRoundedRectangle(26.0f, 37.0f, 48.0f, 48.0f, 13.0f);
    paintLabel(g, "L", 42, 42, 36, 38, juce::Colours::white, 27.0f, true);
    paintLabel(g, "LTA / HARMONY", 86, 43, 129, 28, juce::Colours::white, 16.0f, true);
    paintLabel(g, "MIDI CREATION STUDIO", 27, 104, 175, 25, juce::Colour(0xffa5aabe), 11.0f);
    g.setColour(juce::Colour(0xff363d50));
    g.drawLine(26.0f, 150.0f, 194.0f, 150.0f, 1.0f);
    paintLabel(g, "YOUR IDEA", 27, 173, 170, 20, juce::Colour(0xff9da6bc), 11.0f, true);
    paintLabel(g, "#" + juce::String(static_cast<juce::int64>(displayedIdea % 100000000ULL)),
               27, 196, 170, 34, juce::Colours::white, 25.0f, true);
    g.setColour(juce::Colour(0xff363d50));
    g.drawLine(26.0f, 253.0f, 194.0f, 253.0f, 1.0f);
    paintLabel(g, "HOW TO USE", 27, 274, 175, 21, juce::Colour(0xff9da6bc), 11.0f, true);
    paintLabel(g, "1.  CHOOSE KEY + MOOD", 27, 307, 180, 21, juce::Colours::white, 12.0f);
    paintLabel(g, "2.  GENERATE YOUR IDEA", 27, 340, 180, 21, juce::Colours::white, 12.0f);
    paintLabel(g, "3.  DRAG EACH MIDI LANE", 27, 373, 185, 21, juce::Colours::white, 12.0f);
    paintLabel(g, "HOST-SYNCED MIDI", 27, 510, 180, 22, juce::Colour(0xffa5aabe), 11.0f, true);
    paintLabel(g, "CH 1   CHORDS", 27, 538, 180, 22, juce::Colours::white, 12.0f);
    paintLabel(g, "CH 2   TOPLINE", 27, 567, 180, 22, juce::Colours::white, 12.0f);
    paintLabel(g, "CH 3   BASS", 27, 596, 180, 22, juce::Colours::white, 12.0f);
    paintLabel(g, "OR EXPORT AS .MID FILES", 27, 638, 186, 22, juce::Colour(0xffa5aabe), 11.0f);
    paintLabel(g, "LTA HARMONY  /  V1.0", 27, 743, 176, 20, juce::Colour(0xffa5aabe), 11.0f);

    paintLabel(g, "Make something", 253, 32, 520, 39, ink, 30.0f, true);
    paintLabel(g, "new today.", 253, 68, 355, 39, pink, 30.0f, true);
    paintLabel(g, "ORIGINAL GENERATIVE MIDI  /  USE WITH ANY INSTRUMENT", 622, 94, 476, 21, muted, 10.0f, true);

    const std::array<std::pair<const char*, int>, 6> labels {{
        {"KEY", 254}, {"SCALE", 351}, {"EMOTION", 508}, {"STYLE", 664},
        {"LENGTH", 861}, {"BPM", 965}
    }};
    for (const auto& label : labels)
        paintLabel(g, label.first, label.second, 122, 120, 20, muted, 11.0f, true);
    g.setColour(border);
    g.drawLine(252.0f, 206.0f, 1096.0f, 206.0f, 1.0f);
    paintLabel(g, "CHORD PROGRESSION", 252, 214, 265, 22, ink, 12.0f, true);
    paintLabel(g, "4/4    /    GENERATED VOICINGS", 873, 214, 223, 22, muted, 10.0f);
    auto pattern = processor.getPattern();
    if (pattern && !pattern->chords.empty()) {
        const int size = static_cast<int>(pattern->chords.size());
        const float gap = 4.0f;
        const float cell = (844.0f - (size - 1) * gap) / size;
        for (int i = 0; i < size; ++i) {
            const float x = 252 + i * (cell + gap);
            g.setColour(i % 4 == 0 ? juce::Colour(0xffffedf4) : juce::Colours::white);
            g.fillRoundedRectangle(x, 242.0f, cell, 76.0f, 8.0f);
            g.setColour(border);
            g.drawRoundedRectangle(x + 0.5f, 242.5f, cell - 1, 75.0f, 8.0f, 1.0f);
            paintLabel(g, juce::String(i + 1).paddedLeft('0', 2), static_cast<int>(x + 9), 246,
                       static_cast<int>(cell - 10), 18, muted, size >= 16 ? 9.0f : 11.0f);
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(size >= 16 ? 10.0f : 16.0f, juce::Font::bold)));
            g.drawFittedText(juce::String(pattern->chords[static_cast<size_t>(i)].name),
                static_cast<int>(x + 7), 268, static_cast<int>(cell - 10), 35,
                juce::Justification::centred, 1, 0.65f);
        }
    }
    for (int i = 0; i < 3; ++i) {
        const float y = 336.0f + i * 126.0f;
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(252.0f, y, 844.0f, 110.0f, 12.0f);
        g.setColour(border);
        g.drawRoundedRectangle(252.5f, y + 0.5f, 843.0f, 109.0f, 12.0f, 1.0f);
        g.setColour(laneColours[static_cast<size_t>(i)]);
        g.fillRoundedRectangle(264.0f, y + 15, 4.0f, 23.0f, 2.0f);
        paintLabel(g, laneTitles[static_cast<size_t>(i)], 276, static_cast<int>(y + 11),
                   170, 30, ink, 14.0f, true);
    }
    paintLabel(g, "COMPLEXITY", 255, 710, 140, 20, muted, 10.0f, true);
    paintLabel(g, "NOTE DENSITY", 464, 710, 140, 20, muted, 10.0f, true);
    paintLabel(g, "SWING", 660, 710, 140, 20, muted, 10.0f, true);
    g.setColour(border);
    g.drawLine(252.0f, 700.0f, 1096.0f, 700.0f, 1.0f);
    paintLabel(g, statusText, 255, 768, 840, 15, muted, 10.0f);
    g.restoreState();
}
