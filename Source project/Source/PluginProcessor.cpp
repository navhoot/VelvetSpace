#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace velvet;
using APVTS = juce::AudioProcessorValueTreeState;

static juce::StringArray toArray(const List& l) { juce::StringArray a; for (auto* s : l) a.add(s); return a; }

static juce::String formatValue(int i, float v) {
    auto& s = spec(i); juce::String unit = s.unit;
    if (i == RType) {
        int a = (int) std::floor(v) % Modes, b = (a + 1) % Modes; float f = v - std::floor(v);
        if (f < .02f || f > .98f) return juce::String(v, 2) + "  " + modeNames[(size_t) (f > .98f ? b : a)];
        return juce::String(v, 2) + "  " + modeNames[(size_t) a] + " > " + modeNames[(size_t) b];
    }
    if (unit == "Hz" && v >= 1000) return juce::String(v / 1000.0f, v >= 10000 ? 1 : 2) + " kHz";
    if (unit == "ms" && v >= 1000) return juce::String(v / 1000.0f, 2) + " s";
    if (unit == "dB" && (i == LowDrive || i == MidDrive || i == HighDrive || i == DuckDepth)) return juce::String(v, 1) + " dB";
    float range = s.hi - s.lo; int dec = range > 50 ? 0 : range > 5 ? 1 : 2;
    if (unit == "Hz" && v < 10) dec = v < 1 ? 3 : 2;
    if (unit == "s") dec = v < 10 ? 2 : 1;
    if (unit == "ms" && v < 10) dec = 1;
    auto num = dec == 0 ? juce::String(juce::roundToInt(v)) : juce::String(v, dec);
    return num + (unit.isEmpty() ? "" : (unit == "%" ? "%" : " " + unit));
}

int VelvetSpaceProcessor::destIndexFor(const juce::String& id) {
    auto& d = destinations();
    for (size_t k = 0; k < d.size(); ++k) if (id == spec(d[k]).id) return (int) k + 1;
    return 0;
}

APVTS::ParameterLayout VelvetSpaceProcessor::createLayout() {
    static const char* groupNames[] { "Space", "Color", "Motion", "Pump", "Modulation", "Global" };
    std::array<std::unique_ptr<juce::AudioProcessorParameterGroup>, 6> groups;
    for (int g = 0; g < 6; ++g) groups[(size_t) g] = std::make_unique<juce::AudioProcessorParameterGroup>(juce::String(groupNames[g]).toLowerCase(), groupNames[g], "|");
    juce::StringArray destNames { "Off" };
    for (int d : destinations()) destNames.add(spec(d).name);

    for (int i = 0; i < PCount; ++i) {
        auto& s = spec(i);
        juce::ParameterID pid { s.id, 2 };
        std::unique_ptr<juce::RangedAudioParameter> p;
        if (s.kind == Toggle) p = std::make_unique<juce::AudioParameterBool>(pid, s.name, s.def > .5f);
        else if (s.kind == Choice) p = std::make_unique<juce::AudioParameterChoice>(pid, s.name, s.list ? toArray(*s.list) : destNames, (int) s.def);
        else {
            juce::NormalisableRange<float> range(s.lo, s.hi,
                [i](float, float, float n) { return value(i, n); },
                [i](float, float, float v) { return norm(i, v); });
            p = std::make_unique<juce::AudioParameterFloat>(pid, s.name, range, s.def,
                juce::AudioParameterFloatAttributes().withLabel(s.unit).withStringFromValueFunction([i](float v, int) { return formatValue(i, v); }));
        }
        groups[(size_t) s.group]->addChild(std::move(p));
    }
    APVTS::ParameterLayout layout;
    for (auto& g : groups) layout.add(std::move(g));
    return layout;
}

VelvetSpaceProcessor::VelvetSpaceProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "VelvetState", createLayout()) {
    for (int i = 0; i < PCount; ++i) { pr[(size_t) i] = apvts.getRawParameterValue(spec(i).id); live[(size_t) i] = spec(i).def; }
    for (auto& l : mLfo) l = 0;
    for (auto& w : mWeights) w = 0;
    apvts.state.setProperty("lanes", 4, nullptr);
    presets.refresh();
}

bool VelvetSpaceProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (l.getMainInputChannelSet() != out && l.getMainInputChannelSet() != juce::AudioChannelSet::mono()) return false;
    if (l.inputBuses.size() > 1) {
        auto sc = l.getChannelSet(true, 1);
        if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void VelvetSpaceProcessor::prepareToPlay(double sr, int) { engine.prepare((float) sr); }

void VelvetSpaceProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    for (int i = 0; i < PCount; ++i) values[(size_t) i] = pr[(size_t) i]->load();
    float g = pendingGlideMs.exchange(-1.0f);
    if (g >= 0) engine.setGlide(g);
    engine.beginBlock(values);

    float bpm = 120; bool playing = false; double ppq = 0;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) {
        if (auto b = pos->getBpm()) bpm = (float) juce::jlimit(20.0, 400.0, *b);
        playing = pos->getIsPlaying();
        if (auto p = pos->getPpqPosition()) ppq = *p;
    }
    auto main = getBusBuffer(buffer, true, 0);
    auto out = getBusBuffer(buffer, false, 0);
    const int n = buffer.getNumSamples();
    const int inCh = main.getNumChannels(), outCh = out.getNumChannels();
    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
        auto sc = getBusBuffer(buffer, true, 1);
        if (sc.getNumChannels() > 0) { scL = sc.getReadPointer(0); scR = sc.getReadPointer(juce::jmin(1, sc.getNumChannels() - 1)); }
    }
    sidechainActive = scL != nullptr;
    if (inCh == 0 || outCh == 0) { buffer.clear(); return; }
    float* L = out.getWritePointer(0);
    float* R = outCh > 1 ? out.getWritePointer(1) : nullptr;
    const float* inL = main.getReadPointer(0);
    const float* inR = inCh > 1 ? main.getReadPointer(1) : inL;
    const double ppqPerSample = bpm / 60.0 / getSampleRate();

    auto midiIt = midi.begin();
    float peakIn = 0, peakOut = 0, peakWet = 0;
    for (int i = 0; i < n; ++i) {
        for (; midiIt != midi.end() && (*midiIt).samplePosition <= i; ++midiIt) {
            auto msg = (*midiIt).getMessage();
            if (msg.isNoteOn()) engine.noteOn(msg.getFloatVelocity());
            else if (msg.isNoteOff()) engine.noteOff();
            else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
        }
        float l = inL[i], r = inR[i];
        auto f = engine.process(l, r, scL ? scL[i] : 0.0f, scR ? scR[i] : 0.0f, scL != nullptr, bpm, playing, ppq + i * ppqPerSample);
        peakIn = juce::jmax(peakIn, std::abs(l), std::abs(r));
        dryScope.push(.5f * (l + r)); wetScope.push(.5f * (f.wetL + f.wetR));
        if (R) { L[i] = f.l; R[i] = f.r; } else L[i] = .5f * (f.l + f.r);
        peakOut = juce::jmax(peakOut, std::abs(f.l), std::abs(f.r));
        peakWet = juce::jmax(peakWet, std::abs(f.wetL), std::abs(f.wetR));
    }
    for (int c = 2; c < outCh; ++c) out.clear(c, 0, n);
    auto& m = engine.meters();
    for (int i = 0; i < PCount; ++i) live[(size_t) i].store(engine.current(i), std::memory_order_relaxed);
    lastBlockMs = juce::Time::getMillisecondCounter();
    mTypePos = m.typePos; mEnv = m.env; mMidiEnv = m.midiEnv; mDuck = m.duckGain; mGate = m.gateGain; mPan = m.panPos;
    mOx = m.orbitX; mOy = m.orbitY; mOz = m.orbitZ;
    for (int i = 0; i < Lfos; ++i) mLfo[(size_t) i] = m.lfo[(size_t) i];
    for (int i = 0; i < Modes; ++i) mWeights[(size_t) i] = m.weights[(size_t) i];
    mInPeak = juce::jmax(peakIn, mInPeak.load() * .9f); mOutPeak = juce::jmax(peakOut, mOutPeak.load() * .9f); mWetPeak = juce::jmax(peakWet, mWetPeak.load() * .9f);
}

int VelvetSpaceProcessor::getNumPrograms() { return juce::jmax(1, (int) presets.entries().size()); }
const juce::String VelvetSpaceProcessor::getProgramName(int i) {
    auto& e = presets.entries(); return i >= 0 && i < (int) e.size() ? e[(size_t) i].name : juce::String();
}

void VelvetSpaceProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    state.setProperty("presetName", presetName, nullptr);
    state.setProperty("version", JucePlugin_VersionString, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void VelvetSpaceProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            auto tree = juce::ValueTree::fromXml(*xml);
            presetName = tree.getProperty("presetName", "Clean Canvas").toString();
            apvts.replaceState(tree);
            // Snap stepped parameters to their stored state (the tree can match a cached value while the raw
            // normalised value still sits between steps, which would leave a host-set value unrestored).
            for (auto* p : getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
                    if (dynamic_cast<juce::AudioParameterBool*>(p) || dynamic_cast<juce::AudioParameterChoice*>(p))
                        if (auto* v = apvts.getRawParameterValue(rp->paramID))
                            rp->setValueNotifyingHost(rp->convertTo0to1(v->load()));
            if (!apvts.state.hasProperty("lanes")) apvts.state.setProperty("lanes", 4, nullptr);
            presets.refresh();
        }
}

juce::AudioProcessorEditor* VelvetSpaceProcessor::createEditor() { return new VelvetSpaceEditor(*this); }

// ---------------------------------------------------------------- presets
PresetManager::PresetManager(VelvetSpaceProcessor& p) : proc(p) {}

juce::File PresetManager::userFolder() {
    auto f = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Velvet Space").getChildFile("Presets");
    f.createDirectory();
    return f;
}

void PresetManager::refresh() {
    list.clear();
    auto& fp = velvet::factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i) list.push_back({ fp[(size_t) i].name, fp[(size_t) i].category, true, {}, i });
    auto files = userFolder().findChildFiles(juce::File::findFiles, false, "*.vspreset");
    files.sort();
    for (auto& f : files) list.push_back({ f.getFileNameWithoutExtension(), "User", false, f, -1 });
    current = 0;
    for (int i = 0; i < (int) list.size(); ++i) if (list[(size_t) i].name == proc.presetName) { current = i; break; }
    if (onChange) onChange();
}

juce::String PresetManager::currentName() const { return proc.presetName; }

void PresetManager::applyState(const juce::ValueTree& tree) {
    proc.pendingGlideMs = proc.apvts.getRawParameterValue("presetglide")->load();
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p)) {
            if (rp->getParameterID() == "presetglide") continue;
            auto child = tree.getChildWithProperty("id", rp->getParameterID());
            float n = child.isValid() ? rp->convertTo0to1((float) child.getProperty("value")) : rp->getDefaultValue();
            rp->beginChangeGesture(); rp->setValueNotifyingHost(n); rp->endChangeGesture();
        }
    int lanes = tree.getProperty("lanes", -1);
    if (lanes < 1) { // factory preset: show every used lane (minimum 4)
        lanes = 4;
        for (int n = 0; n < Lanes; ++n) {
            auto c = tree.getChildWithProperty("id", juce::String(spec(laneP(n, 1)).id));
            if (c.isValid() && (float) c.getProperty("value") > 0) lanes = juce::jmax(lanes, n + 1);
        }
    }
    proc.setLaneCount(lanes);
}

bool PresetManager::load(int index) {
    if (index < 0 || index >= (int) list.size()) return false;
    auto& e = list[(size_t) index];
    juce::ValueTree tree("VelvetState");
    auto add = [&](const juce::String& id, float v) { juce::ValueTree c("PARAM"); c.setProperty("id", id, nullptr); c.setProperty("value", v, nullptr); tree.appendChild(c, nullptr); };
    if (e.factory) {
        auto& f = velvet::factoryPresets()[(size_t) e.factoryIndex];
        for (auto& [id, v] : f.values) add(id, v);
        for (size_t k = 0; k < f.lanes.size() && k < (size_t) Lanes; ++k) {
            auto& ln = f.lanes[k]; int n = (int) k;
            add(spec(laneP(n, 0)).id, (float) ln.source);
            add(spec(laneP(n, 1)).id, (float) VelvetSpaceProcessor::destIndexFor(ln.dest));
            add(spec(laneP(n, 2)).id, (float) ln.direction);
            add(spec(laneP(n, 3)).id, ln.depth);
        }
    } else {
        auto xml = juce::XmlDocument::parse(e.file);
        if (!xml) return false;
        tree = juce::ValueTree::fromXml(*xml);
        if (!tree.isValid()) return false;
    }
    applyState(tree);
    proc.presetName = e.name;
    current = index;
    if (onChange) onChange();
    return true;
}

bool PresetManager::saveUser(const juce::String& rawName) {
    auto name = juce::File::createLegalFileName(rawName.trim());
    if (name.isEmpty()) return false;
    proc.presetName = name;
    auto state = proc.apvts.copyState();
    state.setProperty("presetName", name, nullptr);
    auto xml = state.createXml();
    if (!xml || !xml->writeTo(userFolder().getChildFile(name + ".vspreset"))) return false;
    refresh();
    return true;
}

bool PresetManager::deleteUser(int index) {
    if (index < 0 || index >= (int) list.size() || list[(size_t) index].factory) return false;
    bool ok = list[(size_t) index].file.deleteFile();
    proc.presetName = "Clean Canvas";
    refresh();
    return ok;
}

bool PresetManager::importFile(const juce::File& f) {
    if (!f.existsAsFile()) return false;
    auto dest = userFolder().getChildFile(f.getFileName());
    if (!f.copyFileTo(dest)) return false;
    refresh();
    for (int i = 0; i < (int) list.size(); ++i) if (!list[(size_t) i].factory && list[(size_t) i].file == dest) return load(i);
    return false;
}

bool PresetManager::exportFile(const juce::File& f) const {
    auto state = proc.apvts.copyState();
    state.setProperty("presetName", proc.presetName, nullptr);
    auto xml = state.createXml();
    return xml && xml->writeTo(f.withFileExtension("vspreset"));
}

void PresetManager::step(int delta) {
    if (list.empty()) return;
    int n = (int) list.size();
    load(((current + delta) % n + n) % n);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VelvetSpaceProcessor(); }
