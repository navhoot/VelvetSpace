#pragma once
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
// Velvet Space 2 - original algorithmic reverb engine. No third-party DSP code.
// Wet chain: pre-delay -> morphing reverb -> cuts -> 3-band saturation -> post EQ
//            -> width -> motion (gate, auto-pan, 3D orbit) -> pump (ducker) -> mix.
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>
#include <deque>

namespace velvet {
constexpr float pi = 3.14159265358979323846f;
inline float clamp(float x, float a, float b) { return std::max(a, std::min(b, x)); }
inline float db(float x) { return std::pow(10.0f, x / 20.0f); }

// ----------------------------------------------------------------- parameters
enum P {
    // Space
    Mix, RType, SelMode, MorphTime, CycleRate, CycleSync, CycleSpan, CycleStyle,
    Size, Decay, PreDelay, Damping, Diffusion, Width, Early, ModRate, ModDepth, BassMult, TrebleMult, Freeze,
    ReverbOn, CutsOn, LowCut, HighCut,
    // Color
    SatOn, SatMix, LowType, MidType, HighType, LowDrive, MidDrive, HighDrive, SatLowX, SatHighX, SatTrim,
    EqOn, EqLow, EqMid, EqHigh, EqLowX, EqHighX, EqFreq, EqGain, EqQ,
    // Motion (wet only)
    PanOn, PanRate, PanSync, PanDepth, PanShape,
    OrbitOn, OrbitMode, OrbitRate, OrbitSync, Around, UpDown, NearFar,
    GateOn, GateMode, GateRate, GateSync, GateDepth, GateShape, GateSmooth, GateThresh, GateHold,
    // Pump
    DuckOn, DuckMode, DuckDepth, DuckThresh, DuckAttack, DuckRelease, DuckMix, DuckSource, PumpSync, PumpCurve,
    // Modulation sources
    EnvThresh, EnvAttack, EnvRelease, MidiAttack, MidiRelease, Macro1, Macro2, Macro3, Macro4,
    Input, Output, PresetGlide,
    LfoBase,                       // 4 LFOs x { shape, rate, sync, trigger, phase }
    LaneBase = LfoBase + 4 * 5,    // 16 lanes x { source, destination, direction, depth }
    PCount = LaneBase + 16 * 4
};
constexpr int Lfos = 4, Lanes = 16, Modes = 10;
inline int lfoP(int l, int k) { return LfoBase + l * 5 + k; }   // k: 0 shape 1 rate 2 sync 3 trigger 4 phase
inline int laneP(int n, int k) { return LaneBase + n * 4 + k; } // k: 0 source 1 dest 2 direction 3 depth

enum Kind { Float, Choice, Toggle };
using List = std::vector<const char*>;
inline const List modeNames { "Room", "Chamber", "Plate", "Hall", "Vintage 80s", "Bloom", "Cathedral", "Spring", "Dark Space", "Nebula" };
inline const List selModes { "Snap", "Blend", "Cycle" };
inline const List cycleStyles { "Loop", "Ping-pong", "Drift" };
inline const List syncNames { "Free", "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars", "8 bars",
                              "1/16 D", "1/8 D", "1/4 D", "1/2 D", "1/16 T", "1/8 T", "1/4 T", "1/2 T" };
inline const float beatLengths[] { 0, .0625f, .125f, .25f, .5f, 1, 2, 4, 8, 16, 32, .375f, .75f, 1.5f, 3, .16666667f, .33333333f, .66666667f, 1.33333333f };
inline const List satNames { "Clean", "Tape", "Tube", "Soft clip", "Hard clip", "Fold", "Rectify", "Asymmetric", "Bitcrush", "Downsample" };
inline const List shapes { "Sine", "Triangle", "Saw up", "Saw down", "Square", "Pulse 25%", "Pulse 10%", "Sample & hold",
                           "Smooth random", "Random walk", "Steps 4", "Steps 8", "Exp up", "Exp down", "Bounce", "Half sine" };
inline const List orbitModes { "Full Orbit", "Horizontal", "Vertical", "Figure 8", "Spiral", "Pendulum" };
inline const List gateModes { "Rhythmic", "Envelope" };
inline const List duckModes { "Envelope", "Tempo pump", "Both" };
inline const List duckSources { "Input", "Sidechain" };
inline const List triggers { "Free", "Host", "MIDI", "Audio" }; // free run, host-synced phase, MIDI note retrigger, audio-onset retrigger
inline const List laneSources { "Off", "Audio Env", "MIDI Env", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Macro 1", "Macro 2", "Macro 3", "Macro 4",
                                "Velocity", "Random (per note)", "Duck amount", "Sidechain Env" };
inline const List directions { "Both Ways", "Up", "Down" };

struct Spec { const char* id; const char* name; const char* unit; float lo, hi, def; bool log; Kind kind; const List* list; int group; };
// group: 0 space 1 color 2 motion 3 pump 4 mod 5 global

inline std::deque<std::string>& nameStore() { static std::deque<std::string> s; return s; }
inline const char* keep(std::string s) { auto& st = nameStore(); st.push_back(std::move(s)); return st.back().c_str(); }

inline const std::vector<Spec>& specs() {
    static const std::vector<Spec> s = [] {
        std::vector<Spec> v((size_t) PCount);
        auto F = [&](int i, const char* id, const char* n, const char* u, float lo, float hi, float d, bool lg, int g) { v[(size_t) i] = { id, n, u, lo, hi, d, lg, Float, nullptr, g }; };
        auto C = [&](int i, const char* id, const char* n, const List& l, int d, int g) { v[(size_t) i] = { id, n, "", 0, (float) l.size() - 1, (float) d, false, Choice, &l, g }; };
        auto T = [&](int i, const char* id, const char* n, bool d, int g) { v[(size_t) i] = { id, n, "", 0, 1, d ? 1.0f : 0.0f, false, Toggle, nullptr, g }; };
        F(Mix, "mix", "Wet / Dry", "%", 0, 100, 28, false, 0);
        F(RType, "rtype", "Reverb type", "", 0, Modes - 1, 3, false, 0);
        C(SelMode, "selmode", "Select mode", selModes, 1, 0);
        F(MorphTime, "morphtime", "Morph time", "ms", 0, 5000, 600, false, 0);
        F(CycleRate, "cyclerate", "Cycle rate", "Hz", .005f, 2, .04f, true, 0);
        C(CycleSync, "cyclesync", "Cycle sync", syncNames, 0, 0);
        F(CycleSpan, "cyclespan", "Cycle span", "", 0, Modes, 4, false, 0);
        C(CycleStyle, "cyclestyle", "Cycle style", cycleStyles, 1, 0);
        F(Size, "size", "Size", "%", 0, 100, 48, false, 0);
        F(Decay, "decay", "Decay", "s", .15f, 30, 3.2f, true, 0);
        F(PreDelay, "predelay", "Pre-delay", "ms", 0, 250, 24, false, 0);
        F(Damping, "damping", "Damping", "Hz", 800, 20000, 8200, true, 0);
        F(Diffusion, "diffusion", "Diffusion", "%", 0, 100, 72, false, 0);
        F(Width, "width", "Width", "%", 0, 200, 100, false, 0);
        F(Early, "early", "Early reflections", "dB", -60, 6, -10, false, 0);
        F(ModRate, "modrate", "Tank mod rate", "Hz", .01f, 5, .35f, true, 0);
        F(ModDepth, "moddepth", "Tank mod depth", "ms", 0, 5, .7f, false, 0);
        F(BassMult, "bassmult", "Bass decay", "x", .25f, 2, 1.1f, false, 0);
        F(TrebleMult, "treblemult", "Treble decay", "x", .2f, 1, .7f, false, 0);
        T(Freeze, "freeze", "Freeze", false, 0);
        T(ReverbOn, "reverbon", "Reverb", true, 0);
        T(CutsOn, "cutson", "Reverb cuts", true, 0);
        F(LowCut, "lowcut", "Low cut", "Hz", 20, 2000, 120, true, 0);
        F(HighCut, "highcut", "High cut", "Hz", 1000, 20000, 20000, true, 0);

        T(SatOn, "saton", "Saturation", true, 1);
        F(SatMix, "satmix", "Saturation wet / dry", "%", 0, 100, 100, false, 1);
        C(LowType, "lowtype", "Low band type", satNames, 2, 1);
        C(MidType, "midtype", "Mid band type", satNames, 1, 1);
        C(HighType, "hightype", "High band type", satNames, 3, 1);
        F(LowDrive, "lowdrive", "Low drive", "dB", 0, 36, 0, false, 1);
        F(MidDrive, "middrive", "Mid drive", "dB", 0, 36, 0, false, 1);
        F(HighDrive, "highdrive", "High drive", "dB", 0, 36, 0, false, 1);
        F(SatLowX, "satlowx", "Sat low / mid", "Hz", 60, 2000, 260, true, 1);
        F(SatHighX, "sathighx", "Sat mid / high", "Hz", 1000, 14000, 3600, true, 1);
        F(SatTrim, "sattrim", "Saturation trim", "dB", -18, 6, 0, false, 1);
        T(EqOn, "eqon", "Post EQ", false, 1);
        F(EqLow, "eqlow", "EQ low", "dB", -18, 18, 0, false, 1);
        F(EqMid, "eqmid", "EQ mid", "dB", -18, 18, 0, false, 1);
        F(EqHigh, "eqhigh", "EQ high", "dB", -18, 18, 0, false, 1);
        F(EqLowX, "eqlowx", "EQ low / mid", "Hz", 40, 2000, 250, true, 1);
        F(EqHighX, "eqhighx", "EQ mid / high", "Hz", 1000, 16000, 4500, true, 1);
        F(EqFreq, "eqfreq", "EQ bell freq", "Hz", 40, 18000, 2400, true, 1);
        F(EqGain, "eqgain", "EQ bell gain", "dB", -18, 18, 0, false, 1);
        F(EqQ, "eqq", "EQ bell Q", "", .2f, 8, .71f, true, 1);

        T(PanOn, "panon", "Auto pan", false, 2);
        F(PanRate, "panrate", "Pan rate", "Hz", .02f, 20, 0.5f, true, 2);
        C(PanSync, "pansync", "Pan sync", syncNames, 0, 2);
        F(PanDepth, "pandepth", "Pan depth", "%", 0, 100, 100, false, 2);
        C(PanShape, "panshape", "Pan shape", shapes, 0, 2);
        T(OrbitOn, "orbiton", "3D space", false, 2);
        C(OrbitMode, "orbitmode", "Orbit path", orbitModes, 0, 2);
        F(OrbitRate, "orbitrate", "Orbit rate", "Hz", .01f, 10, .2f, true, 2);
        C(OrbitSync, "orbitsync", "Orbit sync", syncNames, 0, 2);
        F(Around, "around", "Around", "%", 0, 100, 0, false, 2);
        F(UpDown, "updown", "Up / down", "%", 0, 100, 0, false, 2);
        F(NearFar, "nearfar", "Near / far", "%", 0, 100, 0, false, 2);
        T(GateOn, "gateon", "Gate", false, 2);
        C(GateMode, "gatemode", "Gate mode", gateModes, 0, 2);
        F(GateRate, "gaterate", "Gate rate", "Hz", .1f, 32, 4, true, 2);
        C(GateSync, "gatesync", "Gate sync", syncNames, 3, 2);
        F(GateDepth, "gatedepth", "Gate depth", "%", 0, 100, 0, false, 2);
        C(GateShape, "gateshape", "Gate shape", shapes, 4, 2);
        F(GateSmooth, "gatesmooth", "Gate smoothing", "ms", .5f, 200, 6, true, 2);
        F(GateThresh, "gatethresh", "Gate threshold", "dB", -70, 0, -36, false, 2);
        F(GateHold, "gatehold", "Gate hold", "ms", 0, 2000, 180, false, 2);

        T(DuckOn, "duckon", "Ducking", true, 3);
        C(DuckMode, "duckmode", "Duck mode", duckModes, 0, 3);
        F(DuckDepth, "duckdepth", "Duck depth", "dB", 0, 48, 0, false, 3);
        F(DuckThresh, "duckthresh", "Sensitivity", "dB", -60, 0, -24, false, 3);
        F(DuckAttack, "duckattack", "Duck attack", "ms", .1f, 500, 5, true, 3);
        F(DuckRelease, "duckrelease", "Duck release", "ms", 10, 5000, 350, true, 3);
        F(DuckMix, "duckmix", "Duck wet / dry", "%", 0, 100, 100, false, 3);
        C(DuckSource, "ducksource", "Duck source", duckSources, 0, 3);
        C(PumpSync, "pumpsync", "Pump rate", syncNames, 5, 3);
        F(PumpCurve, "pumpcurve", "Pump curve", "", .3f, 6, 2, true, 3);

        F(EnvThresh, "envthresh", "Audio threshold", "dB", -70, 0, -36, false, 4);
        F(EnvAttack, "envattack", "Env attack", "ms", .1f, 1000, 2, true, 4);
        F(EnvRelease, "envrelease", "Env release", "ms", 5, 5000, 180, true, 4);
        F(MidiAttack, "midiattack", "MIDI env attack", "ms", .1f, 2000, 5, true, 4);
        F(MidiRelease, "midirelease", "MIDI env release", "ms", 5, 8000, 400, true, 4);
        F(Macro1, "macro1", "Macro 1", "%", 0, 100, 0, false, 4);
        F(Macro2, "macro2", "Macro 2", "%", 0, 100, 0, false, 4);
        F(Macro3, "macro3", "Macro 3", "%", 0, 100, 0, false, 4);
        F(Macro4, "macro4", "Macro 4", "%", 0, 100, 0, false, 4);
        F(Input, "input", "Input", "dB", -24, 12, 0, false, 5);
        F(Output, "output", "Output", "dB", -24, 12, 0, false, 5);
        F(PresetGlide, "presetglide", "Preset morph", "ms", 0, 8000, 900, false, 5);
        static const float lfoRates[] { .25f, .5f, 1.0f, 2.0f };
        for (int l = 0; l < Lfos; ++l) {
            auto n = std::to_string(l + 1);
            C(lfoP(l, 0), keep("lfo" + n + "shape"), keep("LFO " + n + " shape"), shapes, l == 1 ? 1 : 0, 4);
            F(lfoP(l, 1), keep("lfo" + n + "rate"), keep("LFO " + n + " rate"), "Hz", .01f, 40, lfoRates[l], true, 4);
            C(lfoP(l, 2), keep("lfo" + n + "sync"), keep("LFO " + n + " sync"), syncNames, 0, 4);
            C(lfoP(l, 3), keep("lfo" + n + "trig"), keep("LFO " + n + " trigger"), triggers, 0, 4);
            F(lfoP(l, 4), keep("lfo" + n + "phase"), keep("LFO " + n + " phase"), "deg", 0, 360, 0, false, 4);
        }
        for (int n = 0; n < Lanes; ++n) {
            auto k = std::to_string(n + 1);
            C(laneP(n, 0), keep("lane" + k + "src"), keep("Lane " + k + " source"), laneSources, n < 4 ? (n == 0 ? 1 : n == 1 ? 2 : n + 1) : 0, 4);
            v[(size_t) laneP(n, 1)] = { keep("lane" + k + "dest"), keep("Lane " + k + " destination"), "", 0, (float) (PCount - 1 - Lanes * 3), 0, false, Choice, nullptr, 4 };
            C(laneP(n, 2), keep("lane" + k + "dir"), keep("Lane " + k + " direction"), directions, 0, 4);
            F(laneP(n, 3), keep("lane" + k + "depth"), keep("Lane " + k + " depth"), "%", -100, 100, 0, false, 4);
        }
        return v;
    }();
    return s;
}

// Destinations: every parameter except lane routing (source/destination/direction) and preset glide.
inline const std::vector<int>& destinations() {
    static const std::vector<int> d = [] {
        std::vector<int> v;
        for (int i = 0; i < PCount; ++i) {
            if (i == PresetGlide) continue;
            if (i >= LaneBase && (i - LaneBase) % 4 != 3) continue;
            v.push_back(i);
        }
        return v;
    }();
    return d;
}
inline int destCount() { return (int) destinations().size() + 1; } // + "Off"
inline const Spec& spec(int i) { return specs()[(size_t) i]; }
inline float norm(int i, float v) {
    auto& s = spec(i); v = clamp(v, s.lo, s.hi);
    if (s.hi <= s.lo) return 0;
    return s.log ? std::log(v / s.lo) / std::log(s.hi / s.lo) : (v - s.lo) / (s.hi - s.lo);
}
inline float value(int i, float n) {
    auto& s = spec(i); n = clamp(n, 0, 1);
    float v = s.log ? s.lo * std::pow(s.hi / s.lo, n) : s.lo + (s.hi - s.lo) * n;
    return s.kind == Float ? v : std::round(v);
}

// ----------------------------------------------------------------- reverb mode traits
// Every mode is a vector of traits. Blend/Cycle interpolate these continuously (circularly).
struct Traits { float scale, diffusion, mod, early, damp, bandwidth, bloom, spring, density, stereo; };
inline const std::array<Traits, Modes>& modeTraits() {
    static const std::array<Traits, Modes> t {{
        //  scale diff  mod   early damp  bw    bloom spring dens  stereo
        { .58f, .86f, .40f, 1.50f, 1.00f, 1.00f, 0.0f, 0.0f, .75f, .85f },  // Room
        { .82f, 1.00f, .65f, 1.15f, .95f, .90f, 0.1f, 0.0f, .85f, 1.0f },   // Chamber
        { .40f, 1.22f, .55f, .20f, 1.40f, 1.00f, 0.0f, 0.0f, 1.0f, 1.1f },  // Plate
        { 1.12f, 1.00f, 1.00f, .80f, 1.00f, .95f, 0.15f, 0.0f, .9f, 1.15f },// Hall
        { 1.02f, .90f, 1.70f, .70f, .70f, .42f, 0.1f, 0.0f, .8f, 1.2f },    // Vintage 80s
        { 1.20f, 1.10f, 1.10f, .25f, .90f, .80f, 1.0f, 0.0f, .95f, 1.25f }, // Bloom
        { 1.50f, 1.05f, 1.30f, .55f, .85f, .85f, 0.35f, 0.0f, 1.0f, 1.3f }, // Cathedral
        { .45f, .55f, .50f, .15f, 1.10f, .38f, 0.0f, 1.0f, .6f, .9f },      // Spring
        { 1.30f, 1.00f, 1.20f, .45f, .40f, .35f, 0.4f, 0.0f, .9f, 1.2f },   // Dark Space
        { 1.38f, 1.25f, 2.40f, .30f, .90f, .75f, 0.7f, 0.0f, 1.0f, 1.4f },  // Nebula
    }};
    return t;
}
inline Traits traitsAt(float pos, std::array<float, Modes>* weights = nullptr) {
    pos = std::fmod(pos, (float) Modes); if (pos < 0) pos += Modes;
    int a = (int) pos % Modes, b = (a + 1) % Modes; float f = pos - std::floor(pos);
    auto& t = modeTraits(); const float* A = &t[(size_t) a].scale; const float* B = &t[(size_t) b].scale;
    Traits r; float* R = &r.scale;
    for (int i = 0; i < 10; ++i) R[i] = A[i] + (B[i] - A[i]) * f;
    if (weights) { weights->fill(0); (*weights)[(size_t) a] += 1 - f; (*weights)[(size_t) b] += f; }
    return r;
}

// ----------------------------------------------------------------- building blocks
struct Delay {
    std::vector<float> a; int head = 0;
    void prepare(int n) { a.assign((size_t) n + 8, 0); head = 0; }
    void clear() { std::fill(a.begin(), a.end(), 0); head = 0; }
    float read(float d) const {
        const int n = (int) a.size();
        float r = (float) head - clamp(d, 1, (float) n - 3);
        int i = (int) std::floor(r); float f = r - (float) i;
        i %= n; if (i < 0) i += n; int j = i + 1 == n ? 0 : i + 1;
        return a[(size_t) i] + f * (a[(size_t) j] - a[(size_t) i]);
    }
    void push(float x) { a[(size_t) head] = x; if (++head == (int) a.size()) head = 0; }
};
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void pass(float sr, float hz, bool high, float q = .70710678f) {
        float w = 2 * pi * clamp(hz, 10, sr * .45f) / sr, c = std::cos(w), s = std::sin(w), al = s / (2 * q), inv = 1 / (1 + al);
        b0 = (high ? (1 + c) : (1 - c)) * .5f * inv; b1 = (high ? -(1 + c) : (1 - c)) * inv; b2 = b0; a1 = -2 * c * inv; a2 = (1 - al) * inv;
    }
    void bell(float sr, float hz, float q, float gainDb) {
        float A = std::pow(10.0f, gainDb / 40), w = 2 * pi * clamp(hz, 10, sr * .45f) / sr, c = std::cos(w), al = std::sin(w) / (2 * q), inv = 1 / (1 + al / A);
        b0 = (1 + al * A) * inv; b1 = -2 * c * inv; b2 = (1 - al * A) * inv; a1 = -2 * c * inv; a2 = (1 - al / A) * inv;
    }
    float tick(float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void clear() { z1 = z2 = 0; }
};
struct LR4 { Biquad a, b; void set(float sr, float hz, bool high) { a.pass(sr, hz, high); b.pass(sr, hz, high); } float tick(float x) { return b.tick(a.tick(x)); } void clear() { a.clear(); b.clear(); } };
// Phase-coherent 3-band splitter (Linkwitz-Riley 4th order, low band all-pass compensated).
struct Split3 {
    LR4 lp1, hp1, lp2, hp2, apL, apH; float f1 = -1, f2 = -1;
    void set(float sr, float lo, float hi) {
        hi = std::max(hi, lo * 1.25f);
        if (lo != f1) { lp1.set(sr, lo, false); hp1.set(sr, lo, true); f1 = lo; }
        if (hi != f2) { lp2.set(sr, hi, false); hp2.set(sr, hi, true); apL.set(sr, hi, false); apH.set(sr, hi, true); f2 = hi; }
    }
    void tick(float x, float& l, float& m, float& h) { float a = lp1.tick(x), r = hp1.tick(x); l = apL.tick(a) + apH.tick(a); m = lp2.tick(r); h = hp2.tick(r); }
    void clear() { lp1.clear(); hp1.clear(); lp2.clear(); hp2.clear(); apL.clear(); apH.clear(); }
};
struct OnePole { float z = 0; float lp(float x, float a) { z += a * (x - z); return z; } };

inline float saturate(float x, int type, float drive) {
    float g = drive, y = x * g;
    switch (type) {
        case 0: return x;
        case 1: return std::tanh(y) / std::sqrt(g);
        case 2: return (std::tanh(y + .3f) - std::tanh(.3f)) / (.92f * std::sqrt(g));
        case 3: return y / (1 + std::abs(y)) / std::sqrt(g);
        case 4: return clamp(y, -1, 1) / std::sqrt(g);
        case 5: return (2 / pi) * std::asin(std::sin(y * pi * .5f)) / std::sqrt(g);
        case 6: return (.6f * y + .4f * std::abs(y)) / std::sqrt(g);
        case 7: return (y > 0 ? std::tanh(y) : .6f * std::tanh(y / .6f)) / std::sqrt(g);
        default: return x; // 8/9 are stateful, handled in BandSat
    }
}
struct BandSat {
    float hold = 0; int count = 0; float dcIn = 0, dcOut = 0;
    float tick(float x, int type, float driveDb) {
        float drive = db(driveDb), y;
        if (type == 8) { float bits = 16 - driveDb * (13.0f / 36.0f); float q = std::pow(2.0f, bits - 1); y = std::round(clamp(x, -1, 1) * q) / q; }
        else if (type == 9) { int factor = 1 + (int) (driveDb * (31.0f / 36.0f)); if (count <= 0) { hold = x; count = factor; } --count; y = hold; }
        else y = saturate(x, type, drive);
        if (type == 0) return y;
        float o = y - dcIn + .9988f * dcOut; dcIn = y; dcOut = o; return o;
    }
    void clear() { hold = 0; count = 0; dcIn = dcOut = 0; }
};

inline float lfoWave(int shape, float p, float rA, float rB, float walk) {
    switch (shape) {
        case 0: return std::sin(2 * pi * p);
        case 1: return 1 - 4 * std::abs(p - .5f);
        case 2: return 2 * p - 1;
        case 3: return 1 - 2 * p;
        case 4: return p < .5f ? 1.0f : -1.0f;
        case 5: return p < .25f ? 1.0f : -1.0f;
        case 6: return p < .1f ? 1.0f : -1.0f;
        case 7: return rB;
        case 8: { float t = .5f - .5f * std::cos(pi * p); return rA + (rB - rA) * t; }
        case 9: return walk;
        case 10: return std::floor(p * 4) / 1.5f - 1;
        case 11: return std::floor(p * 8) / 3.5f - 1;
        case 12: return 2 * (std::exp(4 * p) - 1) / (std::exp(4.0f) - 1) - 1;
        case 13: return 1 - 2 * (std::exp(4 * p) - 1) / (std::exp(4.0f) - 1);
        case 14: return 2 * std::abs(std::sin(2 * pi * p)) - 1;
        default: return p < .5f ? std::sin(2 * pi * p) * 2 - 1 : -1.0f;
    }
}

struct Lfo {
    float phase = 0, rA = 0, rB = 0, walk = 0, out = 0;
    void restart() { phase = 0; }
};

struct Meters {
    float typePos = 3, env = 0, midiEnv = 0, duckGain = 1, gateGain = 1, panPos = 0, orbitX = 0, orbitY = 0, orbitZ = 0;
    std::array<float, Lfos> lfo {}; std::array<float, Modes> weights {};
};

// ----------------------------------------------------------------- engine
class Engine {
public:
    static constexpr int Control = 32;
    using Values = std::array<float, PCount>;

    void prepare(float sampleRate) {
        sr = sampleRate;
        for (auto& d : tank) d.prepare((int) (sr * .7f));
        for (auto& d : diffuser) d.prepare((int) (sr * .05f));
        for (auto& d : bloomAp) d.prepare((int) (sr * .2f));
        for (auto& d : pre) d.prepare((int) (sr * .26f));
        for (auto& d : early) d.prepare((int) (sr * .2f));
        for (auto& d : itd) d.prepare((int) (sr * .004f));
        smK = 1 - std::exp(-1 / (sr * .04f));
        setGlide(35); ready = false; reset();
    }
    void setGlide(float ms) { smoothA = 1 - std::exp(-1 / (sr * .001f * std::max(1.0f, ms))); glideLeft = (int) (sr * .001f * ms * 4); }
    void reset() {
        smReady = false;
        for (auto& d : tank) d.clear(); for (auto& d : diffuser) d.clear(); for (auto& d : bloomAp) d.clear();
        for (auto& d : pre) d.clear(); for (auto& d : early) d.clear(); for (auto& d : itd) d.clear();
        for (auto& s : satSplit) s.clear(); for (auto& s : eqSplit) s.clear();
        for (auto& c : bandSat) for (auto& b : c) b.clear();
        for (auto& b : cutLo) b.clear(); for (auto& b : cutHi) b.clear(); for (auto& b : bell) b.clear();
        damp.fill(0); bass.fill(0); springZ.fill({}); bwLp = {}; distLp = {}; elevLp = {};
        env = envL = 0; duck = 0; grDb = 0; midiEnv = 0; midiTarget = 0; gateGain = 1; gateEnv = 0; holdLeft = 0; duckGainS = 1;
        mods.fill(0); lastMods.fill(0); rng = 0x8f31da21;
        for (auto& l : lfo) { l = {}; l.rA = random(); l.rB = random(); }
        panPh = orbitPh = gatePh = cyclePh = drift = 0; tankPh = 0; controlCounter = 0; typeS = -1;
    }
    void noteOn(float velocity) {
        midiTarget = 1; heldNotes++; lastVelocity = velocity; noteRandom = .5f * (random() + 1);
        for (int l = 0; l < Lfos; ++l) if ((int) tgt[(size_t) lfoP(l, 3)] == 2) lfo[(size_t) l].restart();
    }
    void noteOff() { heldNotes = std::max(0, heldNotes - 1); if (heldNotes == 0) midiTarget = 0; }
    void allNotesOff() { heldNotes = 0; midiTarget = 0; }
    const Meters& meters() const { return m; }
    float current(int i) const { return cur[(size_t) i]; }

    void beginBlock(const Values& v) {
        tgt = v;
        for (int i = 0; i < PCount; ++i) tnorm[(size_t) i] = norm(i, v[(size_t) i]);
        if (!ready) { base = tnorm; cur = prev = v; ready = true; for (int i = 0; i < 6; ++i) onS[(size_t) i] = v[(size_t) onIds[i]] > .5f ? 1.0f : 0.0f; updateCoefficients(); }
    }

    struct Frame { float l, r, wetL, wetR; };
    // detL/detR = detector input (main or sidechain chosen by host code); bpm/ppq for sync.
    Frame process(float inL, float inR, float scL, float scR, bool scAvailable, float bpm, bool playing, double ppq) {
        float l0 = inL, r0 = inR;
        bool useSc = scAvailable && (int) cur[DuckSource] == 1;
        float dL = useSc ? scL : inL, dR = useSc ? scR : inR;
        float inPeak = std::max(std::abs(inL), std::abs(inR)), detPeak = std::max(std::abs(dL), std::abs(dR));
        float scPeak = scAvailable ? std::max(std::abs(scL), std::abs(scR)) : 0;
        env = follow(inPeak, env, envA, envR); scEnv = follow(scPeak, scEnv, envA, envR);
        duck = follow(detPeak, duck, detA, detR); // fast detector; attack/release act on the gain below
        midiEnv += (midiTarget * (0.4f + 0.6f * lastVelocity) - midiEnv) * (midiTarget > 0 ? midiA : midiR);

        if (controlCounter == 0) {
            runModulation(bpm, playing, ppq);
            prev = cur;
            float blockA = 1 - std::pow(1 - smoothA, (float) Control);
            for (int i = 0; i < PCount; ++i) {
                float mod = mods[(size_t) i];
                if (spec(i).kind == Float) { base[(size_t) i] += (tnorm[(size_t) i] - base[(size_t) i]) * blockA; cur[(size_t) i] = value(i, base[(size_t) i] + mod); }
                else cur[(size_t) i] = value(i, tnorm[(size_t) i] + mod);
            }
            if (glideLeft > 0 && (glideLeft -= Control) <= 0) setGlide(35);
            updateCoefficients();
        }
        controlCounter = (controlCounter + 1) % Control;
        tFrac = (float) (controlCounter == 0 ? Control : controlCounter) / Control;
        inGain = db(lp(Input)); outGain = db(lp(Output));
        // Toggle crossfades (no clicks when switching modules on/off).
        for (int i = 0; i < 6; ++i) onS[(size_t) i] += ((cur[(size_t) onIds[i]] > .5f ? 1.0f : 0.0f) - onS[(size_t) i]) * toggleA;

        // ---- reverb
        float l = inL * inGain, r = inR * inGain;
        float pL = pre[0].read(preSamples), pR = pre[1].read(preSamples); pre[0].push(l); pre[1].push(r);
        float revL, revR; reverb(pL, pR, revL, revR);
        float rvOn = onS[0];
        float wL = pL + (revL - pL) * rvOn, wR = pR + (revR - pR) * rvOn;
        // ---- cuts
        if (onS[1] > 1e-4f) { float cL = cutHi[0].tick(cutLo[0].tick(wL)), cR = cutHi[1].tick(cutLo[1].tick(wR)); wL += (cL - wL) * onS[1]; wR += (cR - wR) * onS[1]; }
        else { cutHi[0].tick(cutLo[0].tick(wL)); cutHi[1].tick(cutLo[1].tick(wR)); }
        // ---- saturation (multi band, multi type) with its own wet/dry
        {
            float sAmt = onS[2] * lp(SatMix) * .01f;
            float w[2] = { wL, wR };
            for (int ch = 0; ch < 2; ++ch) {
                float bL, bM, bH; satSplit[ch].tick(w[ch], bL, bM, bH);
                float s = bandSat[ch][0].tick(bL, (int) cur[LowType], cur[LowDrive]) + bandSat[ch][1].tick(bM, (int) cur[MidType], cur[MidDrive])
                        + bandSat[ch][2].tick(bH, (int) cur[HighType], cur[HighDrive]);
                s *= satTrim;
                w[ch] += (s - w[ch]) * sAmt;
            }
            wL = w[0]; wR = w[1];
        }
        // ---- post-saturation shaping EQ (3-band with adjustable crossovers + bell)
        {
            float w[2] = { wL, wR };
            for (int ch = 0; ch < 2; ++ch) {
                float a, b, c; eqSplit[ch].tick(w[ch], a, b, c);
                float e = bell[ch].tick(a * eqG[0] + b * eqG[1] + c * eqG[2]);
                w[ch] += (e - w[ch]) * onS[3];
            }
            wL = w[0]; wR = w[1];
        }
        // ---- width
        { float mid = (wL + wR) * .5f, side = (wL - wR) * .5f * lp(Width) * .01f; wL = mid + side; wR = mid - side; }
        // ---- motion: gate -> auto pan -> 3D orbit (wet only)
        motion(wL, wR, bpm, playing, ppq);
        // ---- pump
        float duckG = pump(bpm, playing, ppq);
        float dg = 1 + (duckG - 1) * lp(DuckMix) * .01f * onS[4];
        wL *= dg; wR *= dg; m.duckGain = 1 + (duckG - 1) * onS[4];

        float mixAmt = lp(Mix) * .01f;
        Frame f { (l0 * (1 - mixAmt) + wL * mixAmt) * outGain, (r0 * (1 - mixAmt) + wR * mixAmt) * outGain, wL, wR };
        if (!std::isfinite(f.l) || !std::isfinite(f.r)) { reset(); f = { 0, 0, 0, 0 }; }
        return f;
    }

private:
    float sr = 48000; bool ready = false; Values tgt {}, tnorm {}, base {}, cur {}, prev {}, mods {}, lastMods {}; float tFrac = 1;
    float lp(int i) const { return prev[(size_t) i] + (cur[(size_t) i] - prev[(size_t) i]) * tFrac; }
    float smoothA = .001f, toggleA = .002f; int glideLeft = 0, controlCounter = 0;
    Meters m;
    // reverb state
    std::array<Delay, 16> tank; std::array<Delay, 8> diffuser; std::array<Delay, 8> bloomAp; std::array<float, 8> bloomLen {}; std::array<float, 4> bloomG {}; std::array<Delay, 2> pre, early, itd;
    std::array<float, 16> damp {}, bass {}, lengths {}, fbLo {}, fbHi {};
    std::array<std::array<float, 12>, 2> springZ {};
    std::array<OnePole, 2> bwLp, distLp, elevLp;
    float tankPh = 0, preSamples = 1, inGain = 1, outGain = 1, diffusion = .6f, modDepthS = 0, dampA = .5f, bassA = .05f, bwA = 1, earlyG = .3f, tailComp = 1;
    Traits tr {}; float typeS = -1, cyclePh = 0, drift = 0;
    std::array<float, 6> onS { 1, 1, 1, 0, 1, 0 };
    static constexpr int onIds[6] { ReverbOn, CutsOn, SatOn, EqOn, DuckOn, Freeze };
    // color
    std::array<Biquad, 2> cutLo, cutHi, bell; std::array<Split3, 2> satSplit, eqSplit; std::array<std::array<BandSat, 3>, 2> bandSat;
    std::array<float, 3> eqG { 1, 1, 1 }; float satTrim = 1;
    // motion
    float panPh = 0, orbitPh = 0, gatePh = 0, gateGain = 1, gateEnv = 0; int holdLeft = 0; float gA = .01f;
    // pump + sources
    float env = 0, envL = 0, scEnv = 0, duck = 0, duckGainS = 1, envA = .1f, envR = .001f, duckA = .1f, duckR = .001f, detA = .5f, detR = .01f, midiA = .01f, midiR = .001f;
    float midiEnv = 0, midiTarget = 0, lastVelocity = 1, noteRandom = 0; int heldNotes = 0; bool transientArmed = false, wasPlaying = false;
    std::array<Lfo, Lfos> lfo {};
    uint32_t rng = 0x8f31da21;

    float random() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) (rng & 0x00ffffff) / 8388607.5f - 1; }
    static float follow(float in, float prev, float a, float r) { return prev + (in - prev) * (in > prev ? a : r); }
    float coeff(float ms) const { return 1 - std::exp(-1 / (sr * .001f * std::max(.01f, ms))); }
    static float advance(float& ph, float hz, float dt) { ph += hz * dt; ph -= std::floor(ph); return ph; }
    float rateFor(int syncIdx, float hz, float bpm) const { float b = beatLengths[std::clamp(syncIdx, 0, 18)]; return b > 0 ? bpm / (60 * b) : hz; }
    float syncedPhase(int syncIdx, float& ph, float hz, float bpm, bool playing, double ppq, float dt) {
        float b = beatLengths[std::clamp(syncIdx, 0, 18)];
        if (b > 0 && playing) { ph = (float) (ppq / b - std::floor(ppq / b)); return ph; }
        return advance(ph, rateFor(syncIdx, hz, bpm), dt);
    }

    void runModulation(float bpm, bool playing, double ppq) {
        lastMods = mods; Values next {};
        float dt = (float) Control / sr;
        float envDb = 20 * std::log10(env + 1e-9f);
        float audioEnv = clamp((envDb - cur[EnvThresh]) / 30.0f, 0, 1);
        audioEnvS = audioEnv; // attack/release already applied by follower
        bool transient = audioEnv > .35f && !transientArmed; if (audioEnv > .35f) transientArmed = true; else if (audioEnv < .15f) transientArmed = false;
        for (int i = 0; i < Lfos; ++i) {
            auto& L = lfo[(size_t) i]; int trig = (int) cur[(size_t) lfoP(i, 3)], sync = (int) cur[(size_t) lfoP(i, 2)];
            float old = L.phase;
            if (trig == 3 && transient) { L.phase = 0; old = 1; }
            if (trig == 1 && playing && !wasPlaying) { L.phase = 0; old = 1; }
            if (trig == 1 || sync > 0) syncedPhase(sync, L.phase, cur[(size_t) lfoP(i, 1)], bpm, playing && trig == 1, ppq, dt);
            else advance(L.phase, cur[(size_t) lfoP(i, 1)], dt);
            if (L.phase < old) { L.rA = L.rB; L.rB = random(); L.walk = clamp(L.walk + random() * .45f, -1, 1); }
            float p = L.phase + cur[(size_t) lfoP(i, 4)] / 360.0f; p -= std::floor(p);
            L.out = lfoWave((int) cur[(size_t) lfoP(i, 0)], p, L.rA, L.rB, L.walk); m.lfo[(size_t) i] = L.out;
        }
        wasPlaying = playing;
        float scE = clamp((20 * std::log10(scEnv + 1e-9f) - cur[EnvThresh]) / 30.0f, 0, 1);
        auto& dl = destinations();
        for (int n = 0; n < Lanes; ++n) {
            int src = (int) cur[(size_t) laneP(n, 0)], dest = (int) cur[(size_t) laneP(n, 1)], dir = (int) cur[(size_t) laneP(n, 2)];
            float depth = cur[(size_t) laneP(n, 3)] * .01f;
            if (src <= 0 || dest <= 0 || dest > (int) dl.size() || depth == 0) continue;
            float s = 0; bool bipolar = false;
            switch (src) {
                case 1: s = audioEnvS; break; case 2: s = midiEnv; break;
                case 3: case 4: case 5: case 6: s = lfo[(size_t) (src - 3)].out; bipolar = true; break;
                case 7: case 8: case 9: case 10: s = cur[(size_t) (Macro1 + src - 7)] * .01f; break;
                case 11: s = lastVelocity; break; case 12: s = noteRandom; break;
                case 13: s = 1 - duckGainS; break; case 14: s = scE; break;
            }
            float u = bipolar ? (s + 1) * .5f : s;   // 0..1
            float b = bipolar ? s : 2 * s - 1;       // -1..1
            float amt = dir == 0 ? b : dir == 1 ? u : -u;
            next[(size_t) dl[(size_t) dest - 1]] += amt * depth;
        }
        mods = next;
        m.env = audioEnvS; m.midiEnv = midiEnv;
    }
    float audioEnvS = 0;

    void updateCoefficients() {
        envA = coeff(cur[EnvAttack]); envR = coeff(cur[EnvRelease]);
        duckA = coeff(cur[DuckAttack]); duckR = coeff(cur[DuckRelease]); detA = coeff(.2f); detR = coeff(15);
        midiA = coeff(cur[MidiAttack]); midiR = coeff(cur[MidiRelease]);
        toggleA = coeff(15);
        preSamples = std::max(1.0f, cur[PreDelay] * .001f * sr);
        // ---- reverb type selection: Snap (glide to nearest), Blend (continuous), Cycle (auto sweep)
        int mode = (int) cur[SelMode]; float dt = (float) Control / sr; float target = cur[RType];
        if (mode == 0) target = std::round(target);
        if (mode == 2) {
            float span = cur[CycleSpan]; int style = (int) cur[CycleStyle];
            float b = beatLengths[std::clamp((int) cur[CycleSync], 0, 18)];
            float hz = b > 0 ? lastBpm / (60 * b) : cur[CycleRate];
            advance(cyclePh, hz, dt);
            if (style == 0) target = cur[RType] + cyclePh * span;
            else if (style == 1) target = cur[RType] + (1 - std::abs(2 * cyclePh - 1)) * span;
            else { drift += (random() * .5f) * hz * dt * 8; drift = clamp(drift, 0, 1); target = cur[RType] + drift * span; }
        }
        float glideMs = mode == 1 ? 40.0f : std::max(40.0f, cur[MorphTime]);
        if (typeS < 0) typeS = target;
        // Move along the shortest circular path when cycling past the last mode.
        float diff = target - typeS;
        if (mode == 2) { diff = std::fmod(diff, (float) Modes); if (diff > Modes * .5f) diff -= Modes; if (diff < -Modes * .5f) diff += Modes; }
        typeS += diff * (1 - std::exp(-dt * 1000 / glideMs));
        if (mode != 2) typeS = clamp(typeS, 0, Modes - 1);
        tr = traitsAt(typeS, &m.weights); m.typePos = std::fmod(typeS + Modes * 4, (float) Modes);

        diffusion = clamp((.12f + .64f * cur[Diffusion] * .01f) * tr.diffusion, 0, .8f);
        modDepthS = cur[ModDepth] * tr.mod * .001f * sr;
        dampA = 1 - std::exp(-2 * pi * std::min(cur[Damping] * tr.damp, sr * .45f) / sr);
        bassA = 1 - std::exp(-2 * pi * 280 / sr);
        bwA = 1 - std::exp(-2 * pi * std::min(20000.0f * tr.bandwidth, sr * .45f) / sr);
        earlyG = db(cur[Early]) * tr.early;
        // Bloom all-pass smear: its own ring-out must always be faster than the tail (decay/3).
        for (int k = 0; k < 4; ++k) {
            float secs = (.021f + k * .019f) * (.6f + .6f * cur[Size] * .01f) * tr.scale;
            bloomLen[(size_t) (k * 2)] = sr * secs; bloomLen[(size_t) (k * 2 + 1)] = sr * (secs + .0031f);
            bloomG[(size_t) k] = std::min(.62f, std::pow(10.0f, -3 * (secs + .0031f) / std::max(.05f, cur[Decay] / 3)));
        }
        static constexpr float baseLen[16] { .0297f, .0371f, .0411f, .0437f, .0531f, .0593f, .0617f, .0677f, .0713f, .0799f, .0839f, .0893f, .0971f, .1013f, .1079f, .1139f };
        float lenScale = (.3f + cur[Size] * .012f) * tr.scale;
        tailComp = 2.0f * std::sqrt(lenScale / 1.0f) * (0.75f + 0.25f * tr.density);
        bool frozen = cur[Freeze] > .5f;
        for (int i = 0; i < 16; ++i) {
            lengths[(size_t) i] = sr * baseLen[i] * lenScale * (1 - (1 - tr.density) * .5f * ((i & 3) / 3.0f));
            float sec = lengths[(size_t) i] / sr;
            fbLo[(size_t) i] = frozen ? 1.0f : std::pow(10.0f, -3 * sec / (cur[Decay] * cur[BassMult]));
            fbHi[(size_t) i] = frozen ? 1.0f : std::pow(10.0f, -3 * sec / (cur[Decay] * cur[TrebleMult]));
        }
        // ---- cuts / color
        for (int c = 0; c < 2; ++c) {
            cutLo[c].pass(sr, cur[LowCut], true); cutHi[c].pass(sr, std::min(cur[HighCut], sr * .45f), false);
            satSplit[c].set(sr, cur[SatLowX], cur[SatHighX]);
            eqSplit[c].set(sr, cur[EqLowX], cur[EqHighX]);
            bell[c].bell(sr, cur[EqFreq], cur[EqQ], cur[EqGain]);
        }
        satTrim = db(cur[SatTrim]);
        eqG = { db(cur[EqLow]), db(cur[EqMid]), db(cur[EqHigh]) };
        gA = coeff(cur[GateSmooth]);
    }
    float lastBpm = 120;

    // Per-sample smoothing of every reverb coefficient, so morphing between modes (cycle / blend / snap)
    // glides delay lengths and gains instead of stepping them every control block.
    struct RevSmooth { float spring = 0, bloom = 0, stereo = 1, tailComp = 1, earlyG = 0, bw = 1, diff = 0, damp = 1, mod = 0;
                       std::array<float, 16> len {}, fbLo {}, fbHi {}; std::array<float, 8> bloomLen {}; std::array<float, 4> bloomG {}; } sm;
    bool smReady = false; float smK = .001f;
    void smoothReverb() {
        auto go = [this](float& v, float t) { v += (t - v) * smK; };
        if (!smReady) { sm.spring = tr.spring; sm.bloom = tr.bloom; sm.stereo = tr.stereo; sm.tailComp = tailComp; sm.earlyG = earlyG; sm.bw = bwA; sm.diff = diffusion;
                        sm.damp = dampA; sm.mod = modDepthS; sm.len = lengths; sm.fbLo = fbLo; sm.fbHi = fbHi; sm.bloomLen = bloomLen; sm.bloomG = bloomG; smReady = true; return; }
        go(sm.spring, tr.spring); go(sm.bloom, tr.bloom); go(sm.stereo, tr.stereo); go(sm.tailComp, tailComp); go(sm.earlyG, earlyG); go(sm.bw, bwA);
        go(sm.diff, diffusion); go(sm.damp, dampA); go(sm.mod, modDepthS);
        for (int i = 0; i < 16; ++i) { go(sm.len[(size_t) i], lengths[(size_t) i]); go(sm.fbLo[(size_t) i], fbLo[(size_t) i]); go(sm.fbHi[(size_t) i], fbHi[(size_t) i]); }
        for (int i = 0; i < 8; ++i) go(sm.bloomLen[(size_t) i], bloomLen[(size_t) i]);
        for (int i = 0; i < 4; ++i) go(sm.bloomG[(size_t) i], bloomG[(size_t) i]);
    }

    void reverb(float inL, float inR, float& outL, float& outR) {
        smoothReverb();
        float frozen = onS[5];
        // Bandwidth (vintage / dark modes)
        float xL = bwLp[0].lp(inL, sm.bw), xR = bwLp[1].lp(inR, sm.bw);
        // Spring: dispersive all-pass chain gives the classic chirp
        if (sm.spring > .001f) {
            float s[2] = { xL, xR };
            for (int ch = 0; ch < 2; ++ch) {
                float v = s[ch];
                for (int k = 0; k < 12; ++k) { float a = -.62f; float y = a * v + springZ[ch][k]; springZ[ch][k] = v - a * y; v = y; }
                s[ch] = s[ch] + (v - s[ch]) * sm.spring;
            }
            xL = s[0]; xR = s[1];
        }
        // Early reflections
        early[0].push(xL); early[1].push(xR);
        float sc = .6f + .4f * tr.scale;
        float erL = .34f * early[0].read(sr * .0113f * sc) + .25f * early[1].read(sr * .0277f * sc) + .2f * early[0].read(sr * .0493f * sc) + .13f * early[1].read(sr * .0791f * sc);
        float erR = .34f * early[1].read(sr * .0137f * sc) + .25f * early[0].read(sr * .0311f * sc) + .2f * early[1].read(sr * .0571f * sc) + .13f * early[0].read(sr * .0839f * sc);
        // Input sm.diff
        for (int i = 0; i < 4; ++i) for (int ch = 0; ch < 2; ++ch) {
            float& x = ch == 0 ? xL : xR; auto& d = diffuser[(size_t) (i * 2 + ch)];
            float del = d.read(sr * (.0031f + i * .0027f + ch * .00073f)); float y = del - sm.diff * x; d.push(x + sm.diff * y); x = y;
        }
        // Bloom: long all-pass smear that slows the build-up
        if (sm.bloom > .001f) {
            float b[2] = { xL, xR };
            for (int ch = 0; ch < 2; ++ch) {
                float v = b[ch];
                for (int k = 0; k < 4; ++k) { auto& d = bloomAp[(size_t) (k * 2 + ch)]; float g = sm.bloomG[(size_t) k]; float del = d.read(sm.bloomLen[(size_t) (k * 2 + ch)]); float y = del - g * v; d.push(v + g * y); v = y; }
                b[ch] = b[ch] + (v - b[ch]) * sm.bloom;
            }
            xL = b[0]; xR = b[1];
        }
        float inject = 1 - frozen;
        tankPh += std::max(.01f, cur[ModRate]) / sr; if (tankPh >= 1) tankPh -= 1;
        std::array<float, 16> v {}; float sum = 0;
        for (int i = 0; i < 16; ++i) {
            float mod = std::sin(2 * pi * (tankPh * (1 + i * .013f) + i * .618034f)) * sm.mod;
            float x = tank[(size_t) i].read(sm.len[(size_t) i] + mod);
            float da = sm.damp + (1 - sm.damp) * frozen;
            damp[(size_t) i] += da * (x - damp[(size_t) i]);
            bass[(size_t) i] += bassA * (damp[(size_t) i] - bass[(size_t) i]);
            v[(size_t) i] = bass[(size_t) i] * sm.fbLo[(size_t) i] + (damp[(size_t) i] - bass[(size_t) i]) * sm.fbHi[(size_t) i];
            sum += v[(size_t) i];
        }
        float tL = 0, tR = 0;
        for (int i = 0; i < 16; ++i) {
            float fb = v[(size_t) i] - sum * .125f; // Householder (orthogonal) feedback
            float ex = ((i & 1) ? xR : xL) * ((i & 2) ? -.25f : .25f) * inject;
            tank[(size_t) i].push(fb + ex);
            tL += v[(size_t) i] * ((i & 1) ? -.28f : .28f); tR += v[(size_t) i] * ((i & 4) ? -.28f : .28f);
        }
        float mid = (tL + tR) * .5f, side = (tL - tR) * .5f * sm.stereo;
        outL = (mid + side) * sm.tailComp + erL * sm.earlyG * inject;
        outR = (mid - side) * sm.tailComp + erR * sm.earlyG * inject;
    }

    void motion(float& L, float& R, float bpm, bool playing, double ppq) {
        lastBpm = bpm; float dt = 1 / sr;
        // Gate / tremolo
        float gateTarget = 1;
        if (cur[GateOn] > .5f) {
            float depth = lp(GateDepth) * .01f;
            if ((int) cur[GateMode] == 0) {
                float p = syncedPhase((int) cur[GateSync], gatePh, cur[GateRate], bpm, playing, ppq, dt);
                float w = (lfoWave((int) cur[GateShape], p, 0, 0, 0) + 1) * .5f;
                gateTarget = 1 - depth * (1 - w);
            } else {
                float lvl = 20 * std::log10(env + 1e-9f);
                if (lvl > cur[GateThresh]) holdLeft = (int) (cur[GateHold] * .001f * sr);
                bool open = holdLeft > 0; if (holdLeft > 0) --holdLeft;
                gateTarget = open ? 1.0f : 1 - depth;
            }
        }
        gateGain += (gateTarget - gateGain) * gA; m.gateGain = gateGain;
        L *= gateGain; R *= gateGain;
        // Auto pan
        float pan = 0;
        if (cur[PanOn] > .5f) {
            float p = syncedPhase((int) cur[PanSync], panPh, cur[PanRate], bpm, playing, ppq, dt);
            pan = lfoWave((int) cur[PanShape], p, 0, 0, 0) * lp(PanDepth) * .01f;
        }
        // 3D orbit
        float x = 0, y = 0, z = 0;
        if (cur[OrbitOn] > .5f) {
            float p = syncedPhase((int) cur[OrbitSync], orbitPh, cur[OrbitRate], bpm, playing, ppq, dt);
            float th = 2 * pi * p;
            switch ((int) cur[OrbitMode]) {
                case 0: x = std::cos(th); z = std::sin(th); y = std::sin(2 * th) * .7f; break;
                case 1: x = std::cos(th); z = std::sin(th); break;
                case 2: y = std::cos(th); z = std::sin(th); break;
                case 3: x = std::sin(th); z = std::sin(2 * th); break;
                case 4: { float rr = .55f + .45f * std::sin(th * .25f); x = rr * std::cos(th); z = rr * std::sin(th); y = std::sin(th * .25f); break; }
                default: x = std::sin(th); break;
            }
            x *= lp(Around) * .01f; y *= lp(UpDown) * .01f; z *= lp(NearFar) * .01f;
        }
        float px = clamp(pan + x, -1, 1);
        m.panPos = px; m.orbitX = x; m.orbitY = y; m.orbitZ = z;
        // Equal-power balance
        float a = (px + 1) * pi * .25f; float gL = std::cos(a) * 1.41421356f, gR = std::sin(a) * 1.41421356f;
        L *= std::min(gL, 1.41421356f); R *= std::min(gR, 1.41421356f);
        // Inter-aural delay for the far ear (up to 0.6 ms)
        itd[0].push(L); itd[1].push(R);
        float dly = std::abs(x) * .0006f * sr;
        if (x > 0.001f) L = itd[0].read(1 + dly); else if (x < -0.001f) R = itd[1].read(1 + dly);
        // Distance: level drop + air absorption.  Elevation: high-shelf tilt.
        if (z != 0 || y != 0) {
            float far = (z + 1) * .5f;
            float g = db(-9.0f * std::max(0.0f, z)) * db(3.0f * std::max(0.0f, -z));
            float cutoff = 18000 * std::pow(.18f, std::max(0.0f, z));
            float aa = 1 - std::exp(-2 * pi * std::min(cutoff, sr * .45f) / sr);
            float shelf = y * 0.9f; (void) far;
            float s[2] = { L, R };
            for (int ch = 0; ch < 2; ++ch) {
                float lp = distLp[ch].lp(s[ch], aa);
                float low = elevLp[ch].lp(lp, 1 - std::exp(-2 * pi * 2500 / sr));
                s[ch] = (lp + (lp - low) * shelf) * g;
            }
            L = s[0]; R = s[1];
        }
    }

    float pump(float bpm, bool playing, double ppq) {
        int mode = (int) cur[DuckMode]; float depth = cur[DuckDepth];
        float gEnv = 1, gPump = 1;
        if (mode != 1) {
            float lvl = 20 * std::log10(duck + 1e-9f);
            float over = clamp((lvl - cur[DuckThresh]) / 12.0f, 0, 1);
            float target = -depth * over;
            grDb += (target - grDb) * (target < grDb ? duckA : duckR);
            gEnv = db(grDb);
        } else grDb = 0;
        if (mode != 0) {
            float b = beatLengths[std::clamp((int) cur[PumpSync], 1, 18)];
            float ph = playing ? (float) (ppq / b - std::floor(ppq / b)) : advance(pumpFree, bpm / (60 * b), 1 / sr);
            float rise = std::pow(ph, 1 / std::max(.05f, cur[PumpCurve]));
            gPump = db(-depth * (1 - rise));
        }
        duckGainS = std::min(gEnv, gPump);
        return duckGainS;
    }
    float pumpFree = 0, grDb = 0;
};
}
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
