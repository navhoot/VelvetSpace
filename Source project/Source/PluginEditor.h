#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct EditorUi; // all components live in PluginEditor.cpp

class VelvetSpaceEditor : public juce::AudioProcessorEditor {
public:
    explicit VelvetSpaceEditor(VelvetSpaceProcessor&);
    ~VelvetSpaceEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void applySize();
    static constexpr int BaseW = 1360, BaseH = 862, CollapsedH = 630;
    float scale = 1.0f;
    bool matrixShown = true;
private:
    VelvetSpaceProcessor& proc;
    std::unique_ptr<EditorUi> ui;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelvetSpaceEditor)
};
