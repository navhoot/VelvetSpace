// Offline DSP checks for the Velvet Space 2 engine. Build with -DVELVET_BUILD_TESTS=ON.
#include "Dsp.h"
#include "Presets.h"
#include <cstdio>
#include <chrono>
#include <random>
#include <cstring>
#include <functional>

using namespace velvet;
static int failures = 0;
#define CHECK(c, ...) do { if (c) std::printf("PASS  "); else { std::printf("FAIL  "); ++failures; } std::printf(__VA_ARGS__); std::printf("\n"); } while (0)

using V = Engine::Values;
static int idx(const char* id) { for (int i = 0; i < PCount; ++i) if (!std::strcmp(spec(i).id, id)) return i; return -1; }
static int destIdx(int param) { auto& d = destinations(); for (size_t k = 0; k < d.size(); ++k) if (d[k] == param) return (int) k + 1; return 0; }
static V defaults() { V v; for (int i = 0; i < PCount; ++i) v[(size_t) i] = spec(i).def; return v; }
static V quiet() { // clean reverb only: no motion, no ducking, no saturation
    V v = defaults(); v[SatOn] = 0; v[DuckOn] = 0; v[EqOn] = 0; v[PanOn] = v[OrbitOn] = v[GateOn] = 0; return v;
}
using In = std::function<void(int, float&, float&)>;
struct Run { std::vector<float> l, r, wl, wr; };
static Run render(const V& v, int n, float sr, In input, In sc = nullptr, float bpm = 120, bool playing = false, std::function<void(int, V&)> automate = nullptr) {
    Engine e; e.prepare(sr); Run o; o.l.resize((size_t) n); o.r.resize((size_t) n); o.wl.resize((size_t) n); o.wr.resize((size_t) n);
    V cur = v; double ppq = 0;
    for (int i = 0; i < n; ++i) {
        if (i % 256 == 0) { if (automate) automate(i, cur); e.beginBlock(cur); }
        float l, r, sl = 0, sr2 = 0; input(i, l, r); if (sc) sc(i, sl, sr2);
        auto f = e.process(l, r, sl, sr2, (bool) sc, bpm, playing, ppq); ppq += bpm / 60.0 / sr;
        o.l[(size_t) i] = f.l; o.r[(size_t) i] = f.r; o.wl[(size_t) i] = f.wetL; o.wr[(size_t) i] = f.wetR;
    }
    return o;
}
static double rms(const std::vector<float>& v, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += (double) v[(size_t) i] * v[(size_t) i]; return std::sqrt(s / std::max(1, b - a)); }
static bool finite(const Run& o, float& peak) { peak = 0; for (size_t i = 0; i < o.l.size(); ++i) { if (!std::isfinite(o.l[i]) || !std::isfinite(o.r[i])) return false; peak = std::max({ peak, std::abs(o.l[i]), std::abs(o.r[i]) }); } return true; }

int main() {
    const float sr = 48000; const int S = (int) sr;
    std::mt19937 rng(7); std::uniform_real_distribution<float> uni(-0.5f, 0.5f);
    std::vector<float> noise((size_t) S * 4); for (auto& x : noise) x = uni(rng);
    In noiseIn = [&](int i, float& l, float& r) { l = noise[(size_t) i % noise.size()]; r = noise[(size_t) (i * 7) % noise.size()]; };
    In silence = [](int, float& l, float& r) { l = r = 0; };
    In impulse = [](int i, float& l, float& r) { l = r = i == 0 ? 1.0f : 0.0f; };

    { // 1. 0% wet with every wet-only module cranked: output must be bit-identical to input.
        V v = defaults(); v[Mix] = 0; v[LowDrive] = v[MidDrive] = v[HighDrive] = 30; v[LowType] = 5; v[MidType] = 8; v[HighType] = 9;
        v[EqOn] = 1; v[EqHigh] = 12; v[PanOn] = v[OrbitOn] = v[GateOn] = 1; v[GateDepth] = 100; v[Around] = 100; v[DuckDepth] = 30;
        auto o = render(v, S, sr, noiseIn);
        double err = 0; for (int i = 0; i < S; ++i) { float l, r; noiseIn(i, l, r); err = std::max(err, (double) std::abs(o.l[(size_t) i] - l) + std::abs(o.r[(size_t) i] - r)); }
        CHECK(err == 0.0, "dry path untouched at 0%% wet with saturation/EQ/motion/pump active (max error %.3g)", err);
    }
    { // 2. Decay control matches measured RT60 (Schroeder integration, T20 extrapolated) across modes.
        for (int mode : { 3, 2, 6 }) for (float t60 : { 1.0f, 3.0f, 8.0f }) {
            V v = quiet(); v[Mix] = 100; v[RType] = (float) mode; v[SelMode] = 0; v[Decay] = t60; v[BassMult] = 1; v[TrebleMult] = 1; v[Damping] = 20000;
            v[Early] = -60; v[CutsOn] = 0; v[ModDepth] = 0; v[PreDelay] = 0;
            int n = (int) (sr * (t60 * 1.6f + .5f));
            auto o = render(v, n, sr, impulse);
            std::vector<double> edc((size_t) n + 1, 0); for (int i = n - 1; i >= 0; --i) edc[(size_t) i] = edc[(size_t) i + 1] + o.l[(size_t) i] * o.l[(size_t) i] + o.r[(size_t) i] * o.r[(size_t) i];
            auto at = [&](double d) { for (int i = 0; i < n; ++i) if (10 * std::log10(edc[(size_t) i] / edc[0]) <= d) return i / sr; return (float) n / sr; };
            float rt = (at(-30) - at(-10)) * 3; // T20 measured after the build-up (bloom / cathedral modes swell first)
            CHECK(std::abs(rt - t60) / t60 < .25f, "RT60 %-11s Decay %.1fs -> measured %.2fs", modeNames[(size_t) mode], t60, rt);
        }
    }
    { // 3. Wet level is sensible relative to dry (pink-ish noise, 100% wet, Hall).
        V v = quiet(); v[Mix] = 100; v[RType] = 3; v[SelMode] = 0;
        auto o = render(v, S * 3, sr, noiseIn);
        double in = 0; for (int i = S; i < S * 3; ++i) { float l, r; noiseIn(i, l, r); in += l * l; } in = std::sqrt(in / (S * 2));
        double ratio = 20 * std::log10(rms(o.l, S, S * 3) / in);
        CHECK(ratio > -14 && ratio < 3, "wet level vs dry: %.1f dB", ratio);
    }
    { // 4. Stability: random settings incl. random modulation lanes, and extremes.
        std::uniform_real_distribution<float> u(0, 1); bool ok = true; float peakAll = 0;
        for (int trial = 0; trial < 30; ++trial) {
            V v; for (int i = 0; i < PCount; ++i) v[(size_t) i] = value(i, trial < 2 ? (float) trial : u(rng));
            v[Output] = 0; v[Input] = 0; v[Mix] = 50; v[Freeze] = trial == 1 ? 1.0f : 0.0f;
            auto o = render(v, S * 3, sr, noiseIn, noiseIn, 140, true);
            float pk; if (!finite(o, pk)) ok = false; peakAll = std::max(peakAll, pk);
        }
        CHECK(ok && peakAll < 64, "30 random/extreme settings stay finite and bounded (peak %.2f)", peakAll);
    }
    { // 5. Every factory preset: ids valid, lane destinations valid, output finite.
        bool ok = true; float worst = 0; int bad = 0;
        for (auto& p : factoryPresets()) {
            V v = defaults();
            for (auto& [id, val] : p.values) { int i = idx(id); if (i < 0) { std::printf("      unknown id %s in %s\n", id, p.name); ok = false; ++bad; } else v[(size_t) i] = val; }
            for (size_t k = 0; k < p.lanes.size(); ++k) { int d = idx(p.lanes[k].dest), di = d < 0 ? 0 : destIdx(d); if (di == 0) { std::printf("      bad lane dest %s in %s\n", p.lanes[k].dest, p.name); ok = false; ++bad; }
                v[(size_t) laneP((int) k, 0)] = (float) p.lanes[k].source; v[(size_t) laneP((int) k, 1)] = (float) di; v[(size_t) laneP((int) k, 2)] = (float) p.lanes[k].direction; v[(size_t) laneP((int) k, 3)] = p.lanes[k].depth; }
            auto o = render(v, S * 3, sr, noiseIn, noiseIn, 120, true);
            float pk; if (!finite(o, pk)) ok = false; worst = std::max(worst, pk);
        }
        CHECK(ok && worst < 8, "%d factory presets valid and stable (peak %.2f, %d errors)", (int) factoryPresets().size(), worst, bad);
    }
    { // 6. Ducker (envelope, input key): loud input reduces wet; recovers after release.
        V v = quiet(); v[Mix] = 100; v[DuckOn] = 1; v[DuckDepth] = 24; v[DuckThresh] = -40; v[DuckAttack] = 2; v[DuckRelease] = 200;
        In burst = [&](int i, float& l, float& r) { if (i < S) noiseIn(i, l, r); else l = r = 0; };
        auto d = render(v, S * 2, sr, burst); v[DuckOn] = 0; auto n = render(v, S * 2, sr, burst);
        double during = 20 * std::log10(rms(d.l, S / 2, S) / rms(n.l, S / 2, S)), after = 20 * std::log10(rms(d.l, S + S / 2, S * 2) / rms(n.l, S + S / 2, S * 2));
        CHECK(during < -10 && after > -2, "ducker: %.1f dB while input plays, %.1f dB after release", during, after);
    }
    { // 7. Sidechain key ducks the wet signal when the source is Sidechain.
        V v = quiet(); v[Mix] = 100; v[DuckOn] = 1; v[DuckDepth] = 24; v[DuckThresh] = -40; v[DuckSource] = 1;
        In key = [&](int i, float& l, float& r) { l = r = (i / 4800) % 2 ? .8f * std::sin(i * .05f) : 0.0f; };
        auto d = render(v, S * 2, sr, noiseIn, key); v[DuckOn] = 0; auto n = render(v, S * 2, sr, noiseIn, key);
        double g = 20 * std::log10(rms(d.l, S, S * 2) / rms(n.l, S, S * 2));
        CHECK(g < -4, "sidechain ducking average gain change %.1f dB", g);
    }
    { // 8. Tempo pump: periodic gain at the selected note value, synced to host tempo.
        V v = quiet(); v[Mix] = 100; v[DuckOn] = 1; v[DuckMode] = 1; v[DuckDepth] = 24; v[PumpSync] = 5; v[DuckMix] = 100;
        auto d = render(v, S * 3, sr, noiseIn, nullptr, 120, true);
        // 1/4 at 120 bpm = 0.5 s. Compare windows just after vs just before the beat.
        double early = 0, late = 0; for (int b = 2; b < 5; ++b) { int s0 = b * S / 2; early += rms(d.l, s0 + 100, s0 + 2000); late += rms(d.l, s0 - 3000, s0 - 200); }
        double depth = 20 * std::log10(early / late);
        CHECK(depth < -8, "tempo pump dips on the beat: %.1f dB", depth);
    }
    { // 9. Every saturation type changes the wet signal and stays bounded.
        bool ok = true; int changed = 0;
        V base = quiet(); base[Mix] = 100; base[SatOn] = 1; base[LowDrive] = base[MidDrive] = base[HighDrive] = 24;
        V cl = base; cl[LowType] = cl[MidType] = cl[HighType] = 0; auto clean = render(cl, S, sr, noiseIn);
        for (int t = 1; t < (int) satNames.size(); ++t) {
            V v = base; v[LowType] = v[MidType] = v[HighType] = (float) t; auto o = render(v, S, sr, noiseIn);
            float pk; if (!finite(o, pk) || pk > 8) ok = false;
            double diff = 0; for (int i = S / 2; i < S; ++i) diff += std::abs(o.l[(size_t) i] - clean.l[(size_t) i]); diff /= S / 2;
            if (diff > 1e-3) ++changed;
        }
        CHECK(ok && changed == (int) satNames.size() - 1, "all %d saturation types active and bounded (%d changed the sound)", (int) satNames.size() - 1, changed);
        V m = base; m[LowType] = m[MidType] = m[HighType] = 4; m[SatMix] = 0; auto zero = render(m, S, sr, noiseIn); V off = base; off[SatOn] = 0; clean = render(off, S, sr, noiseIn);
        double e = 0; for (int i = 0; i < S; ++i) e = std::max(e, (double) std::abs(zero.l[(size_t) i] - clean.l[(size_t) i]));
        CHECK(e < 1e-4, "saturation wet/dry at 0%% equals unsaturated reverb (max diff %.2g)", e);
    }
    { // 10. Post EQ with adjustable crossovers shapes the wet spectrum.
        V v = quiet(); v[Mix] = 100; v[EqOn] = 1; v[EqHigh] = -18; v[EqHighX] = 2000;
        auto cut = render(v, S * 2, sr, noiseIn); v[EqOn] = 0; auto flat = render(v, S * 2, sr, noiseIn);
        auto hf = [&](const std::vector<float>& x) { double s = 0; for (int i = S; i < S * 2; ++i) { double d = x[(size_t) i] - x[(size_t) i - 1]; s += d * d; } return s; };
        double r = 10 * std::log10(hf(cut.l) / hf(flat.l));
        CHECK(r < -8, "post EQ high band -18 dB reduces HF energy by %.1f dB", r);
    }
    { // 11. Motion: gate, auto-pan and orbit all audibly change the wet signal.
        V v = quiet(); v[Mix] = 100;
        auto ref = render(v, S * 2, sr, noiseIn);
        V g = v; g[GateOn] = 1; g[GateDepth] = 100; g[GateSync] = 0; g[GateRate] = 4; g[GateShape] = 4; auto go = render(g, S * 2, sr, noiseIn);
        double mn = 1e9, mx = 0; for (int w = 0; w < 40; ++w) { double x = rms(go.l, S + w * 1200, S + w * 1200 + 1200); mn = std::min(mn, x); mx = std::max(mx, x); }
        CHECK(20 * std::log10(mx / (mn + 1e-9)) > 20, "rhythmic gate depth: %.1f dB", 20 * std::log10(mx / (mn + 1e-9)));
        V p = v; p[PanOn] = 1; p[PanDepth] = 100; p[PanRate] = 2; auto po = render(p, S * 2, sr, noiseIn);
        double lmn = 1e9, lmx = 0; for (int w = 0; w < 40; ++w) { double a = rms(po.l, S + w * 1200, S + w * 1200 + 1200), b = rms(po.r, S + w * 1200, S + w * 1200 + 1200); double bal = 20 * std::log10(a / (b + 1e-9)); lmn = std::min(lmn, bal); lmx = std::max(lmx, bal); }
        CHECK(lmx - lmn > 12, "auto pan balance swing %.1f dB", lmx - lmn);
        V o = v; o[OrbitOn] = 1; o[Around] = 100; o[UpDown] = 100; o[NearFar] = 100; o[OrbitRate] = 1; auto oo = render(o, S * 2, sr, noiseIn);
        double d = 0; for (int i = S; i < S * 2; ++i) d += std::abs(oo.l[(size_t) i] - ref.l[(size_t) i]); d /= S;
        double om = 1e9, ox = 0; for (int w = 0; w < 40; ++w) { double a = rms(oo.l, S + w * 1200, S + w * 1200 + 1200) / (rms(oo.r, S + w * 1200, S + w * 1200 + 1200) + 1e-9); om = std::min(om, a); ox = std::max(ox, a); }
        CHECK(d > .01 && 20 * std::log10(ox / om) > 6, "3D orbit moves the image (L/R swing %.1f dB)", 20 * std::log10(ox / om));
    }
    { // 12. Cycle and blend morphing are click-free: compare 2nd-difference peaks with a static setting on a sine.
        In sine = [](int i, float& l, float& r) { l = r = .5f * std::sin(2 * pi * 330 * i / 48000.0f); };
        auto spike = [&](const Run& o) { double mx = 0; for (int i = S; i < S * 4; ++i) mx = std::max(mx, (double) std::abs(o.l[(size_t) i] - 2 * o.l[(size_t) i - 1] + o.l[(size_t) i - 2])); return mx / rms(o.l, S, S * 4); };
        V st = quiet(); st[Mix] = 100; st[SelMode] = 0; st[RType] = 3; double base = spike(render(st, S * 4, sr, sine));
        V cy = st; cy[SelMode] = 2; cy[CycleRate] = 1; cy[CycleSpan] = 10; cy[CycleStyle] = 0; double c1 = spike(render(cy, S * 4, sr, sine));
        V sn = st; double c2 = spike(render(sn, S * 4, sr, sine, nullptr, 120, false, [](int i, V& v) { v[RType] = (float) ((i / 24000) % Modes); }));
        V bl = st; bl[SelMode] = 1; double c3 = spike(render(bl, S * 4, sr, sine, nullptr, 120, false, [](int i, V& v) { v[RType] = 9.0f * (float) (i % 96000) / 96000; }));
        CHECK(c1 < base * 3 && c2 < base * 3 && c3 < base * 3, "morph click test (ratio to static): cycle %.2f, snap jumps %.2f, blend sweep %.2f", c1 / base, c2 / base, c3 / base);
    }
    { // 13. Each destination modulated by a fast LFO at full depth stays finite and bounded.
        bool ok = true; float worst = 0; auto& d = destinations();
        for (size_t k = 0; k < d.size(); ++k) {
            V v = defaults(); v[Mix] = 60; v[GateOn] = v[PanOn] = v[OrbitOn] = v[EqOn] = 1;
            v[(size_t) laneP(0, 0)] = 3; v[(size_t) laneP(0, 1)] = (float) (k + 1); v[(size_t) laneP(0, 3)] = 100; v[(size_t) lfoP(0, 1)] = 7; v[(size_t) lfoP(0, 0)] = 4;
            auto o = render(v, S / 2, sr, noiseIn);
            float pk; if (!finite(o, pk)) { ok = false; std::printf("      non-finite with %s\n", spec(d[k]).name); } worst = std::max(worst, pk);
        }
        CHECK(ok && worst < 40, "all %d destinations modulated at full depth stay finite (peak %.2f)", (int) d.size(), worst);
    }
    { // 14. Modulation actually moves the target (LFO -> Mix, Audio Env -> Mix, MIDI env -> Mix).
        V v = quiet(); v[(size_t) laneP(0, 0)] = 3; v[(size_t) laneP(0, 1)] = (float) destIdx(Mix); v[(size_t) laneP(0, 3)] = 50; v[(size_t) lfoP(0, 1)] = 2;
        Engine e; e.prepare(sr); float mn = 1e9f, mx = -1e9f;
        for (int i = 0; i < S; ++i) { if (i % 256 == 0) e.beginBlock(v); float l, r; noiseIn(i, l, r); e.process(l, r, 0, 0, false, 120, false, 0); mn = std::min(mn, e.current(Mix)); mx = std::max(mx, e.current(Mix)); }
        CHECK(mx - mn > 70, "LFO lane sweeps Wet/Dry by %.0f%% (depth 50%%, both ways)", mx - mn);
        V m = quiet(); m[(size_t) laneP(0, 0)] = 2; m[(size_t) laneP(0, 1)] = (float) destIdx(Size); m[(size_t) laneP(0, 2)] = 1; m[(size_t) laneP(0, 3)] = 40;
        Engine e2; e2.prepare(sr); e2.beginBlock(m); float before = 0, held = 0;
        for (int i = 0; i < S; ++i) { if (i == S / 2) e2.noteOn(1); if (i % 256 == 0) e2.beginBlock(m); e2.process(0, 0, 0, 0, false, 120, false, 0); if (i == S / 2 - 1) before = e2.current(Size); held = e2.current(Size); }
        CHECK(held - before > 30, "MIDI envelope lane raises Size %.1f -> %.1f on note-on", before, held);
    }
    { // 15. Freeze holds the tail.
        V v = quiet(); v[Mix] = 100; v[Decay] = 1;
        auto fr = render(v, S * 4, sr, [&](int i, float& l, float& r) { if (i < S) noiseIn(i, l, r); else l = r = 0; }, nullptr, 120, false, [S](int i, V& x) { x[Freeze] = i > S ? 1.0f : 0.0f; });
        double a = rms(fr.l, S + S / 4, S + S / 2), b = rms(fr.l, S * 3, S * 4);
        CHECK(20 * std::log10(b / a) > -6, "freeze sustains the tail (%.1f dB change over 2.5 s)", 20 * std::log10(b / a));
    }
    { // 16. CPU: everything on, 10 s of stereo audio.
        V v = defaults(); v[Mix] = 40; v[SatOn] = v[EqOn] = v[PanOn] = v[OrbitOn] = v[GateOn] = 1; v[GateDepth] = 50; v[SelMode] = 2; v[DuckDepth] = 12;
        for (int n = 0; n < Lanes; ++n) { v[(size_t) laneP(n, 0)] = (float) (1 + n % 6); v[(size_t) laneP(n, 1)] = (float) (1 + (n * 7) % (destCount() - 1)); v[(size_t) laneP(n, 3)] = 10; }
        auto t0 = std::chrono::steady_clock::now(); auto o = render(v, S * 10, sr, noiseIn, noiseIn, 120, true);
        double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        float pk; bool ok = finite(o, pk);
        CHECK(ok && secs / 10 < .25, "CPU with every module + 16 lanes: %.1f%% of one core at 48 kHz", secs * 10);
    }
    { // 17. Silence in -> silence out after the tail (no self-oscillation or DC).
        V v = defaults(); v[Mix] = 100; v[Decay] = 2; auto o = render(v, S * 8, sr, [&](int i, float& l, float& r) { if (i < S / 4) noiseIn(i, l, r); else l = r = 0; });
        double tail = rms(o.l, S * 7, S * 8);
        CHECK(tail < 1e-4, "tail dies away (rms %.2g after 7 s)", tail);
    }
    std::printf("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
