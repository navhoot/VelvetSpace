#pragma once
#include <JuceHeader.h>
#include "Dsp.h"
#include "Presets.h"

class VelvetSpaceProcessor;

// Factory + user preset engine. User presets are XML files (*.vspreset) in
// <user app data>/Velvet Space/Presets, shared across every DAW and plugin format.
class PresetManager {
public:
    explicit PresetManager(VelvetSpaceProcessor& p);
    struct Entry { juce::String name, category; bool factory; juce::File file; int factoryIndex; };
    void refresh();
    const std::vector<Entry>& entries() const { return list; }
    bool load(int index);
    bool saveUser(const juce::String& name);
    bool deleteUser(int index);
    bool importFile(const juce::File& f);
    bool exportFile(const juce::File& f) const;
    void step(int delta);
    int currentIndex() const { return current; }
    juce::String currentName() const;
    static juce::File userFolder();
    std::function<void()> onChange;
private:
    void applyState(const juce::ValueTree& tree);
    VelvetSpaceProcessor& proc;
    std::vector<Entry> list;
    int current = 0;
};

// Single-producer ring of mono samples for the spectrum display (visual only, lock-free).
struct ScopeRing {
    static constexpr int Size = 8192;
    std::array<float, Size> data {};
    std::atomic<int> write { 0 };
    void push(float x) { int w = write.load(std::memory_order_relaxed); data[(size_t) w] = x; write.store((w + 1) & (Size - 1), std::memory_order_release); }
    void copyLatest(float* dest, int n) const { int w = write.load(std::memory_order_acquire); for (int i = 0; i < n; ++i) dest[i] = data[(size_t) ((w - n + i + Size) & (Size - 1))]; }
};

class VelvetSpaceProcessor : public juce::AudioProcessor {
public:
    VelvetSpaceProcessor();
    ~VelvetSpaceProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    void reset() override { engine.reset(); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return presets.currentIndex(); }
    void setCurrentProgram(int i) override { presets.load(i); }
    const juce::String getProgramName(int i) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String paramId(int i) { return velvet::spec(i).id; }
    static int destIndexFor(const juce::String& id); // lane destination choice index (0 = Off)

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets { *this };
    juce::String presetName { "Clean Canvas" };
    std::atomic<float> pendingGlideMs { -1.0f };

    // Meters for the UI
    std::array<std::atomic<float>, velvet::PCount> live {};          // modulated values
    std::atomic<juce::uint32> lastBlockMs { 0 }; // editor falls back to parameter values when audio is not running
    std::atomic<float> mTypePos { 3 }, mEnv { 0 }, mMidiEnv { 0 }, mDuck { 1 }, mGate { 1 }, mPan { 0 }, mOx { 0 }, mOy { 0 }, mOz { 0 };
    std::array<std::atomic<float>, velvet::Lfos> mLfo {};
    std::array<std::atomic<float>, velvet::Modes> mWeights {};
    std::atomic<float> mInPeak { 0 }, mOutPeak { 0 }, mWetPeak { 0 };
    ScopeRing dryScope, wetScope;
    std::atomic<bool> sidechainActive { false };

    int laneCount() const { return juce::jlimit(1, velvet::Lanes, (int) apvts.state.getProperty("lanes", 4)); }
    void setLaneCount(int n) { apvts.state.setProperty("lanes", juce::jlimit(1, velvet::Lanes, n), nullptr); }

private:
    velvet::Engine engine;
    velvet::Engine::Values values {};
    std::array<std::atomic<float>*, velvet::PCount> pr {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelvetSpaceProcessor)
};
