#include "PluginEditor.h"
#include <complex>

using namespace velvet;
using juce::Colour; using juce::Graphics; using juce::Rectangle; using juce::Point; using juce::Path;

// ================================================================ palette
namespace col {
const Colour bg0 { 0xff0e1217 }, bg1 { 0xff151a21 }, panelA { 0xff1c232c }, panelB { 0xff141920 }, well { 0xff0b0e13 },
             edge { 0xff353e4a }, text { 0xffe9edf2 }, dim { 0xff8b95a4 }, faint { 0xff4a5462 }, lime { 0xffd6ee6e },
             space { 0xfff08c6c }, color { 0xfff2b25c }, motion { 0xff5ed6c2 }, pump { 0xffa88cf5 }, matrix { 0xfff06c9b },
             bandLo { 0xffc9d65a }, bandMid { 0xff9a6cf0 }, bandHi { 0xff4fc9b4 };
}
static juce::Font font(float h, bool bold = true) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }

// ================================================================ look and feel
class Lnf : public juce::LookAndFeel_V4 {
public:
    Lnf() {
        setColour(juce::PopupMenu::backgroundColourId, Colour(0xff161b22));
        setColour(juce::PopupMenu::textColourId, col::text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, Colour(0xff2c3644));
        setColour(juce::PopupMenu::highlightedTextColourId, col::lime);
        setColour(juce::PopupMenu::headerTextColourId, col::dim);
        setColour(juce::ComboBox::textColourId, col::text);
        setColour(juce::TextButton::textColourOffId, col::text);
        setColour(juce::TextButton::textColourOnId, col::lime);
        setColour(juce::AlertWindow::backgroundColourId, Colour(0xff161b22));
        setColour(juce::AlertWindow::textColourId, col::text);
        setColour(juce::TextEditor::backgroundColourId, col::well);
        setColour(juce::TextEditor::outlineColourId, col::edge);
        setColour(juce::TextEditor::focusedOutlineColourId, col::lime);
        setColour(juce::ScrollBar::thumbColourId, Colour(0xff3a4452));
        setColour(juce::TooltipWindow::backgroundColourId, Colour(0xff161b22));
        setColour(juce::TooltipWindow::textColourId, col::text);
        setColour(juce::TooltipWindow::outlineColourId, col::edge);
    }
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override {
        auto acc = s.findColour(juce::Slider::rotarySliderFillColourId);
        auto r = Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(1);
        float R = juce::jmin(r.getWidth(), r.getHeight()) * .5f; auto c = r.getCentre();
        auto at = [&](float ang, float rad) { return Point<float>(c.x + rad * std::sin(ang), c.y - rad * std::cos(ang)); };
        const int N = 37;
        for (int i = 0; i < N; ++i) {
            float t = (float) i / (N - 1), ang = a0 + t * (a1 - a0);
            bool lit = t <= pos + 1e-4f;
            float len = (i % 4 == 0) ? .80f : .86f;
            g.setColour(lit ? acc.interpolatedWith(col::text, .15f * (1 - t)) : Colour(0xff3a4350));
            g.drawLine(juce::Line<float>(at(ang, R * len), at(ang, R * .97f)), lit ? 1.8f : 1.2f);
        }
        float br = R * .70f;
        juce::ColourGradient grad(Colour(0xff323a45), c.x, c.y - br, Colour(0xff10141a), c.x, c.y + br, false);
        g.setGradientFill(grad); g.fillEllipse(c.x - br, c.y - br, br * 2, br * 2);
        g.setColour(Colour(0xff4a5463)); g.drawEllipse(c.x - br, c.y - br, br * 2, br * 2, 1.2f);
        g.setColour(Colour(0xff0a0d11)); g.drawEllipse(c.x - br * .82f, c.y - br * .82f, br * 1.64f, br * 1.64f, 1.0f);
        if (s.getProperties().contains("live")) {
            float lv = (float) s.getProperties()["live"];
            if (std::abs(lv - pos) > .004f) {
                Path arc; float aP = a0 + pos * (a1 - a0), aL = a0 + juce::jlimit(0.0f, 1.0f, lv) * (a1 - a0);
                arc.addCentredArc(c.x, c.y, R * .76f, R * .76f, 0, juce::jmin(aP, aL), juce::jmax(aP, aL), true);
                g.setColour(col::text.withAlpha(.85f)); g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                auto d = at(aL, R * .76f); g.fillEllipse(d.x - 2.5f, d.y - 2.5f, 5, 5);
            }
        }
        float ang = a0 + pos * (a1 - a0);
        g.setColour(col::text);
        g.drawLine(juce::Line<float>(at(ang, br * .18f), at(ang, br * .86f)), 2.4f);
        float dr = br * .17f; g.setColour(acc); g.fillEllipse(c.x - dr, c.y - dr, dr * 2, dr * 2);
    }
    void drawLinearSlider(Graphics& g, int x, int y, int w, int h, float pos, float mn, float mx, juce::Slider::SliderStyle st, juce::Slider& s) override {
        if (st != juce::Slider::LinearHorizontal) { LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, mn, mx, st, s); return; }
        auto acc = s.findColour(juce::Slider::trackColourId);
        float cy = y + h * .5f;
        Rectangle<float> tr((float) x, cy - 2.5f, (float) w, 5);
        g.setColour(col::well); g.fillRoundedRectangle(tr, 2.5f);
        g.setColour(Colour(0xff2b333e)); g.drawRoundedRectangle(tr, 2.5f, 1);
        bool bip = s.getProperties()["bipolar"];
        float x0 = bip ? x + w * .5f : (float) x;
        g.setColour(acc); g.fillRoundedRectangle(Rectangle<float>(juce::jmin(x0, pos), cy - 2.5f, std::abs(pos - x0), 5), 2.5f);
        if (s.getProperties().contains("live")) {
            float lp = x + w * (float) s.getProperties()["live"];
            if (std::abs(lp - pos) > 1) { g.setColour(col::text.withAlpha(.8f)); g.fillRect(Rectangle<float>(juce::jmin(lp, pos), cy + 3.5f, std::abs(lp - pos), 1.5f)); }
        }
        g.setColour(Colour(0xffcfd4dc)); g.fillEllipse(pos - 5.5f, cy - 5.5f, 11, 11);
        g.setColour(Colour(0xff0d1015)); g.drawEllipse(pos - 5.5f, cy - 5.5f, 11, 11, 1);
    }
    void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& b) override {
        Rectangle<float> r(0, 0, (float) w, (float) h);
        g.setColour(Colour(0xff0f1318)); g.fillRoundedRectangle(r.reduced(.5f), 4);
        g.setColour(b.hasKeyboardFocus(true) ? col::lime.withAlpha(.6f) : Colour(0xff3a4350)); g.drawRoundedRectangle(r.reduced(.5f), 4, 1);
        Path p; float ax = w - 14.0f, ay = h * .5f;
        p.startNewSubPath(ax - 4, ay - 2); p.lineTo(ax, ay + 2); p.lineTo(ax + 4, ay - 2);
        g.setColour(col::text); g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    juce::Font getComboBoxFont(juce::ComboBox& b) override { return font(b.getHeight() > 22 ? 13.0f : 11.5f); }
    void positionComboBoxText(juce::ComboBox& b, juce::Label& l) override {
        l.setBounds(4, 1, b.getWidth() - 24, b.getHeight() - 2); l.setFont(getComboBoxFont(b));
        l.setJustificationType((bool) b.getProperties()["left"] ? juce::Justification::centredLeft : juce::Justification::centred);
    }
    juce::Font getPopupMenuFont() override { return font(13.0f, false); }
    void drawButtonBackground(Graphics& g, juce::Button& b, const Colour&, bool over, bool down) override {
        auto r = b.getLocalBounds().toFloat().reduced(.5f);
        bool on = b.getToggleState();
        g.setColour(on ? Colour(0xff283039) : (down ? Colour(0xff222932) : over ? Colour(0xff1d232b) : Colour(0xff161b22)));
        g.fillRoundedRectangle(r, 4);
        g.setColour(on ? col::lime.withAlpha(.55f) : Colour(0xff3a4350)); g.drawRoundedRectangle(r, 4, 1);
    }
    juce::Font getTextButtonFont(juce::TextButton&, int h) override { return font(juce::jmin(13.0f, h * .52f)); }
};

// ================================================================ shared context
struct Ctx {
    VelvetSpaceProcessor& p;
    juce::AudioProcessorValueTreeState& s;
    explicit Ctx(VelvetSpaceProcessor& pr) : p(pr), s(pr.apvts) {}
    bool visuals = true;
    static constexpr int Hist = 300, Spec = 220;
    std::array<float, Hist> grHist {}, gateHist {}, envHist {}, midiHist {};
    std::array<std::array<float, Hist>, Lfos> lfoHist {};
    int histPos = 0;
    std::array<Point<float>, 48> trail {}; int trailPos = 0;
    std::array<float, Spec> dryDb {}, wetDb {};
    float live(int i) const { return p.live[(size_t) i].load(std::memory_order_relaxed); }
    float plain(int i) const { return p.apvts.getRawParameterValue(spec(i).id)->load(); }
    bool running() const { return juce::Time::getMillisecondCounter() - p.lastBlockMs.load() < 400; }
    float typePos() const { return running() ? p.mTypePos.load() : plain(RType); }
    juce::RangedAudioParameter* param(int i) const { return s.getParameter(spec(i).id); }
    void set(int i, float v) { if (auto* pr = param(i)) { pr->beginChangeGesture(); pr->setValueNotifyingHost(pr->convertTo0to1(v)); pr->endChangeGesture(); } }
    void setInGesture(int i, float v) { if (auto* pr = param(i)) pr->setValueNotifyingHost(pr->convertTo0to1(v)); }
    float hist(const std::array<float, Hist>& h, int k) const { return h[(size_t) ((histPos + k) % Hist)]; } // k = 0 oldest
};

// ================================================================ basic controls
struct LiveCtl { virtual ~LiveCtl() = default; virtual void updateLive() = 0; };

static juce::String upper(const juce::String& s) { return s.toUpperCase(); }

static void drawWave(Graphics& g, Rectangle<float> r, int shape, Colour c, float thick = 1.4f) {
    Path p; const int n = 48; float rA = -.6f, rB = .5f;
    for (int i = 0; i <= n; ++i) {
        float t = (float) i / n, v;
        if (shape == 7 || shape == 8 || shape == 9) { static const float pts[] { .2f, -.7f, .8f, -.2f, .5f, -.9f, .1f, .7f }; float q = t * 7.99f; int k = (int) q; float f = q - k;
            v = shape == 7 ? pts[k] : shape == 8 ? pts[k] + (pts[(k + 1) % 8] - pts[k]) * (.5f - .5f * std::cos(juce::MathConstants<float>::pi * f)) : pts[k] * .6f + pts[(k + 1) % 8] * .4f * f; }
        else v = lfoWave(shape, std::fmod(t * 2, 1.0f), rA, rB, 0);
        float x = r.getX() + t * r.getWidth(), y = r.getCentreY() - v * r.getHeight() * .45f;
        if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
    }
    g.setColour(c); g.strokePath(p, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

class Knob : public juce::Component, public LiveCtl {
public:
    Knob(Ctx& c, int index, Colour accent, juce::String label = {}) : ctx(c), idx(index), acc(accent), name(label.isEmpty() ? upper(spec(index).name) : label) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::rotarySliderFillColourId, accent);
        s.setPopupDisplayEnabled(false, false, nullptr);
        s.setMouseDragSensitivity(220);
        addAndMakeVisible(s);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(c.s, spec(index).id, s);
        s.setDoubleClickReturnValue(true, (double) spec(index).def);
        s.onValueChange = [this] { repaint(); };
        shape = spec(index).list == &shapes;
        s.setTooltip(spec(index).name);
    }
    void resized() override {
        auto r = getLocalBounds(); r.removeFromTop(13); r.removeFromBottom(shape ? 26 : 15);
        int d = juce::jmin(r.getWidth(), r.getHeight());
        s.setBounds(r.withSizeKeepingCentre(d, d));
    }
    void paint(Graphics& g) override {
        g.setColour(col::dim); g.setFont(font(9.5f)); g.drawFittedText(name, getLocalBounds().removeFromTop(12), juce::Justification::centred, 1, .7f);
        auto b = getLocalBounds();
        if (shape) {
            auto wr = b.removeFromBottom(26).toFloat();
            drawWave(g, wr.removeFromTop(12).withSizeKeepingCentre(38, 11), (int) s.getValue(), acc);
            g.setColour(col::text); g.setFont(font(10.5f)); g.drawText(s.getTextFromValue(s.getValue()), wr.toNearestInt(), juce::Justification::centred);
        } else { g.setColour(col::text); g.setFont(font(12.0f)); g.drawText(s.getTextFromValue(s.getValue()), b.removeFromBottom(15), juce::Justification::centred); }
    }
    void updateLive() override {
        float v = ctx.live(idx), pr = (float) s.valueToProportionOfLength(v);
        if (std::abs(pr - lastLive) > .002f) { lastLive = pr; s.getProperties().set("live", pr); s.repaint(); }
    }
    juce::Slider s;
private:
    Ctx& ctx; int idx; Colour acc; juce::String name; bool shape = false; float lastLive = -1;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Horizontal slider: label on top, value on the right.
class HSlider : public juce::Component, public LiveCtl {
public:
    HSlider(Ctx& c, int index, Colour accent, juce::String label = {}, bool bipolar = false, bool showLabel = true) : ctx(c), idx(index), name(label.isEmpty() ? upper(spec(index).name) : label), withLabel(showLabel) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::trackColourId, accent);
        s.getProperties().set("bipolar", bipolar);
        addAndMakeVisible(s);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(c.s, spec(index).id, s);
        s.setDoubleClickReturnValue(true, (double) spec(index).def);
        s.onValueChange = [this] { repaint(); };
        s.setTooltip(spec(index).name);
    }
    void resized() override { auto r = getLocalBounds(); if (withLabel) r.removeFromTop(12); r.removeFromRight(valueW); s.setBounds(r); }
    void paint(Graphics& g) override {
        auto r = getLocalBounds();
        if (withLabel) { g.setColour(col::dim); g.setFont(font(9.5f)); g.drawText(name, r.removeFromTop(12), juce::Justification::centredLeft); }
        g.setColour(col::text); g.setFont(font(12.0f));
        g.drawText(s.getTextFromValue(s.getValue()), r.removeFromRight(valueW), juce::Justification::centredRight);
    }
    void updateLive() override {
        float pr = (float) s.valueToProportionOfLength(ctx.live(idx));
        if (std::abs(pr - lastLive) > .002f) { lastLive = pr; s.getProperties().set("live", pr); s.repaint(); }
    }
    juce::Slider s; int valueW = 62;
private:
    Ctx& ctx; int idx; juce::String name; bool withLabel; float lastLive = -1;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class Led : public juce::ToggleButton {
public:
    Led(Ctx& c, int index, Colour accent, juce::String label = {}) : acc(accent), name(label.isEmpty() ? upper(spec(index).name) : label) {
        att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(c.s, spec(index).id, *this);
        setTooltip(spec(index).name);
    }
    void paintButton(Graphics& g, bool over, bool) override {
        auto r = getLocalBounds().toFloat().reduced(.5f); bool on = getToggleState();
        g.setColour(over ? Colour(0xff1b2129) : Colour(0xff12171d)); g.fillRoundedRectangle(r, 4);
        g.setColour(on ? acc.withAlpha(.75f) : Colour(0xff3a4350)); g.drawRoundedRectangle(r, 4, 1);
        g.setColour(on ? col::text : col::dim); g.setFont(font(r.getHeight() > 22 ? 12.5f : 10.5f));
        g.drawText(name, r.reduced(9, 0), juce::Justification::centredLeft);
        float d = r.getHeight() * .42f; auto lc = Point<float>(r.getRight() - r.getHeight() * .5f - 2, r.getCentreY());
        if (on) { g.setColour(acc.withAlpha(.25f)); g.fillEllipse(lc.x - d, lc.y - d, d * 2, d * 2); }
        g.setColour(on ? acc : Colour(0xff2a313b)); g.fillEllipse(lc.x - d * .55f, lc.y - d * .55f, d * 1.1f, d * 1.1f);
        g.setColour(on ? acc.brighter(.6f) : Colour(0xff4a5463)); g.drawEllipse(lc.x - d * .55f, lc.y - d * .55f, d * 1.1f, d * 1.1f, 1);
    }
private:
    Colour acc; juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

static void fillDestMenu(juce::ComboBox& b) {
    b.addItem("Off", 1);
    static const char* gn[] { "Space", "Color", "Motion", "Pump", "Mod sources", "Global" };
    juce::PopupMenu sub; juce::String cur;
    auto flush = [&] { if (cur.isNotEmpty()) b.getRootMenu()->addSubMenu(cur, sub); sub = {}; };
    auto& d = destinations();
    for (size_t k = 0; k < d.size(); ++k) {
        int i = d[k];
        juce::String sec = i >= LaneBase ? "Lane depths" : i >= LfoBase ? "LFOs" : gn[spec(i).group];
        if (sec != cur) { flush(); cur = sec; }
        sub.addItem((int) k + 2, spec(i).name);
    }
    flush();
}

class Combo : public juce::ComboBox {
public:
    Combo(Ctx& c, int index, bool left = false) {
        auto& sp = spec(index);
        if (sp.list) { int id = 1; for (auto* n : *sp.list) addItem(n, id++); }
        else fillDestMenu(*this);
        getProperties().set("left", left);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(c.s, sp.id, *this);
        setTooltip(sp.name);
    }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// ================================================================ panel base
class Panel : public juce::Component {
public:
    Panel(juce::String t, Colour a) : title(std::move(t)), acc(a) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(.5f);
        juce::ColourGradient gr(col::panelA, 0, 0, col::panelB, 0, r.getHeight(), false);
        g.setGradientFill(gr); g.fillRoundedRectangle(r, 10);
        g.saveState(); g.reduceClipRegion(getLocalBounds().reduced(2));
        for (int k = 0; k < 4; ++k) {
            Path w; float y0 = r.getHeight() * (.35f + .08f * k);
            for (float x = 0; x <= r.getWidth(); x += 6) { float y = y0 + 6 * std::sin(x * .018f + k * 1.3f) + 3 * std::sin(x * .05f + k); if (x == 0) w.startNewSubPath(x, y); else w.lineTo(x, y); }
            g.setColour(acc.withAlpha(.035f)); g.strokePath(w, juce::PathStrokeType(1.2f));
        }
        g.restoreState();
        g.setColour(acc.withAlpha(.55f)); g.drawRoundedRectangle(r, 10, 1.2f);
        g.setColour(acc); g.setFont(font(12.0f)); g.drawText(title, 14, 8, 300, 16, juce::Justification::centredLeft);
        g.setColour(acc.withAlpha(.22f)); g.fillRect(12.0f, 29.0f, r.getWidth() - 24, 1.0f);
        paintExtra(g);
    }
    virtual void paintExtra(Graphics&) {}
    juce::String title; Colour acc;
};

template <class T, class... A> T& add(juce::Component& parent, std::vector<std::unique_ptr<juce::Component>>& owned, A&&... a) {
    auto c = std::make_unique<T>(std::forward<A>(a)...); auto& r = *c; parent.addAndMakeVisible(r); owned.push_back(std::move(c)); return r;
}

// ================================================================ graph helpers
static float logX(float hz, Rectangle<float> r) { return r.getX() + r.getWidth() * std::log(hz / 20.0f) / std::log(1000.0f); }
static float xToHz(float x, Rectangle<float> r) { return 20.0f * std::pow(1000.0f, juce::jlimit(0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static void drawWell(Graphics& g, Rectangle<float> r) {
    g.setColour(col::well); g.fillRoundedRectangle(r, 6);
    g.setColour(Colour(0xff2a323d)); g.drawRoundedRectangle(r.reduced(.5f), 6, 1);
}
static void freqGrid(Graphics& g, Rectangle<float> r) {
    g.setColour(Colour(0xff1d242d));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) g.drawVerticalLine((int) logX(f, r), r.getY(), r.getBottom());
}
static juce::String hzText(float f) { return f >= 1000 ? juce::String(f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String(juce::roundToInt(f)) + " Hz"; }

// ================================================================ SPACE visuals
class TailField : public juce::Component {
public:
    TailField(Ctx& c, bool big) : ctx(c), large(big) {}
    static float envelope(float u, const Traits& tr, float decay, float size, float diff, float mrate, float mdepth) {
        if (u < 0) return 0;
        float att = .004f + .03f * (1 - diff) + tr.bloom * .32f * (.3f + size);
        float build = 1 - std::exp(-u / att);
        float dec = std::exp(-6.9f * u / decay);
        float wob = 1 + .32f * juce::jmin(1.2f, tr.mod * (.25f + mdepth * .5f)) * std::sin(2 * pi * u * (.35f + mrate * 1.6f) / juce::jmax(.4f, decay * .25f)) * (1 - std::exp(-u * 3));
        return build * dec * wob * (.7f + .3f * tr.density);
    }
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto plot = r.reduced(6, 4); plot.removeFromTop(large ? 22.0f : 14.0f);
        float pos = ctx.typePos();
        std::array<float, Modes> w {}; auto tr = traitsAt(pos, &w);
        int a = (int) std::floor(pos) % Modes; if (a < 0) a += Modes; int b = (a + 1) % Modes; float f = pos - std::floor(pos);
        float decay = ctx.live(Decay), size = ctx.live(Size) * .01f, pre = ctx.live(PreDelay) * .001f, diff = ctx.live(Diffusion) * .01f;
        float mix = ctx.live(Mix) * .01f, mr = ctx.live(ModRate), md = ctx.live(ModDepth), early = db(ctx.live(Early));
        bool frozen = ctx.live(Freeze) > .5f;
        float span = juce::jlimit(.6f, 14.0f, pre + decay * 1.1f);
        auto acc = col::space;
        // grid
        g.setColour(Colour(0xff1b222b));
        for (int i = 1; i < 6; ++i) g.drawVerticalLine((int) (plot.getX() + plot.getWidth() * i / 6), plot.getY(), plot.getBottom());
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine((int) (plot.getY() + plot.getHeight() * i / 4), plot.getX(), plot.getRight());
        auto curve = [&](const Traits& t, float scale) {
            Path p; const int n = 140;
            for (int i = 0; i <= n; ++i) {
                float tt = span * i / n, v = frozen && tt > pre + .25f ? envelope(.25f, t, 1e9f, size, diff, mr, md) : envelope(tt - pre, t, decay, size, diff, mr, md);
                float x = plot.getX() + plot.getWidth() * i / n, y = plot.getBottom() - juce::jlimit(0.0f, 1.0f, v * scale) * plot.getHeight() * .92f;
                if (i == 0) p.startNewSubPath(x, y); else p.lineTo(x, y);
            }
            return p;
        };
        float wetScale = .25f + .75f * mix;
        if (f > .02f && f < .98f) {
            auto& mt = modeTraits();
            g.setColour(col::dim.withAlpha(.35f));
            float dash[] { 3, 3 };
            Path pa = curve(mt[(size_t) a], wetScale), pb = curve(mt[(size_t) b], wetScale), da, dbp;
            juce::PathStrokeType(1.0f).createDashedStroke(da, pa, dash, 2); juce::PathStrokeType(1.0f).createDashedStroke(dbp, pb, dash, 2);
            g.fillPath(da); g.fillPath(dbp);
        }
        Path main = curve(tr, wetScale), fill = main;
        fill.lineTo(plot.getRight(), plot.getBottom()); fill.lineTo(plot.getX(), plot.getBottom()); fill.closeSubPath();
        g.setGradientFill(juce::ColourGradient(acc.withAlpha(.38f), 0, plot.getY(), acc.withAlpha(.02f), 0, plot.getBottom(), false)); g.fillPath(fill);
        g.setColour(acc); g.strokePath(main, juce::PathStrokeType(large ? 2.2f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // early reflections
        g.setColour(acc.brighter(.4f).withAlpha(.8f));
        for (int k = 0; k < 7; ++k) {
            float t = pre + (.004f + k * (.006f + .012f * size)) * (.6f + tr.scale * .5f);
            float hgt = juce::jlimit(0.0f, 1.0f, early * tr.early * .55f * wetScale * std::pow(.8f, (float) k));
            float x = plot.getX() + plot.getWidth() * t / span;
            g.drawLine(x, plot.getBottom(), x, plot.getBottom() - hgt * plot.getHeight() * .92f, 1.2f);
        }
        // dry impulse
        g.setColour(col::text.withAlpha(.9f));
        float dryH = (1 - mix) * plot.getHeight() * .92f;
        g.fillRect(plot.getX(), plot.getBottom() - dryH, 2.5f, dryH);
        // texts
        auto top = r.reduced(8, 3).removeFromTop(large ? 20.0f : 13.0f);
        g.setFont(font(large ? 11.0f : 8.5f)); g.setColour(col::dim); g.drawText("TAIL FIELD", top, juce::Justification::centredLeft);
        juce::String nm = (f < .02f || f > .98f) ? juce::String(modeNames[(size_t) (f > .98f ? b : a)]).toUpperCase()
                                                 : juce::String(modeNames[(size_t) a]).toUpperCase() + " > " + juce::String(modeNames[(size_t) b]).toUpperCase();
        g.setColour(col::text); g.setFont(font(large ? 13.0f : 9.5f)); g.drawText(frozen ? nm + "  (FROZEN)" : nm, top, juce::Justification::centred);
        g.setColour(acc); g.setFont(font(large ? 11.0f : 8.5f)); g.drawText(juce::String(juce::roundToInt(mix * 100)) + "%", top, juce::Justification::centredRight);
        if (large) {
            g.setColour(col::dim); g.setFont(font(10.0f, false));
            g.drawText("0 s", plot.withTrimmedTop(plot.getHeight() - 12), juce::Justification::bottomLeft);
            g.drawText(juce::String(span, 1) + " s", plot.withTrimmedTop(plot.getHeight() - 12), juce::Justification::bottomRight);
            g.setColour(col::text); g.drawText("DRY", plot.getX() + 6, plot.getBottom() - dryH - 2, 40, 12, juce::Justification::centredLeft);
        }
    }
private:
    Ctx& ctx; bool large;
};

class ModeStrip : public juce::Component {
public:
    explicit ModeStrip(Ctx& c) : ctx(c) {}
    void paint(Graphics& g) override {
        static const char* ab[] { "ROOM", "CHMB", "PLATE", "HALL", "80s", "BLOOM", "CATH", "SPRNG", "DARK", "NEBUL" };
        static const char* ab2[] { "RM", "CH", "PL", "HL", "80", "BL", "CA", "SP", "DK", "NB" };
        auto r = getLocalBounds().toFloat(); float w = r.getWidth() / Modes;
        int target = (int) std::round(ctx.plain(RType)) % Modes;
        for (int i = 0; i < Modes; ++i) {
            auto c = Rectangle<float>(r.getX() + i * w, r.getY(), w, r.getHeight()).reduced(1, 0);
            float wt = ctx.running() ? ctx.p.mWeights[(size_t) i].load() : (i == target ? 1.0f : 0.0f);
            g.setColour(col::well); g.fillRoundedRectangle(c, 3);
            g.setColour(col::space.withAlpha(.15f + .7f * wt)); g.fillRoundedRectangle(c, 3);
            if (i == target) { g.setColour(col::text.withAlpha(.8f)); g.drawRoundedRectangle(c, 3, 1); }
            g.setColour(wt > .5f ? Colour(0xff10141a) : col::dim); g.setFont(font(r.getHeight() > 16 ? 10.5f : 7.5f));
            g.drawText(r.getHeight() > 16 && w > 84 ? juce::String(modeNames[(size_t) i]).toUpperCase() : juce::String(w < 30 ? ab2[i] : ab[i]), c.toNearestInt(), juce::Justification::centred);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override {
        int i = juce::jlimit(0, Modes - 1, (int) (e.position.x / getWidth() * Modes));
        ctx.set(RType, (float) i);
    }
private:
    Ctx& ctx;
};

// ================================================================ COLOR visuals
class CrossoverMap : public juce::Component {
public:
    CrossoverMap(Ctx& c, bool big) : ctx(c), large(big) {}
    Rectangle<float> plot() const { auto r = getLocalBounds().toFloat().reduced(6); r.removeFromTop(16); r.removeFromBottom(14); return r; }
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto p = plot();
        float x1 = logX(ctx.live(SatLowX), p), x2 = logX(ctx.live(SatHighX), p);
        Colour bc[] { col::bandLo, col::bandMid, col::bandHi };
        float xs[] { p.getX(), x1, x2, p.getRight() };
        int types[] { (int) ctx.live(LowType), (int) ctx.live(MidType), (int) ctx.live(HighType) };
        float drives[] { ctx.live(LowDrive), ctx.live(MidDrive), ctx.live(HighDrive) };
        bool on = ctx.live(SatOn) > .5f;
        static const char* names[] { "LOW", "MID", "HIGH" };
        for (int k = 0; k < 3; ++k) {
            auto br = Rectangle<float>(xs[k], p.getY(), juce::jmax(1.0f, xs[k + 1] - xs[k]), p.getHeight());
            g.setColour(bc[k].withAlpha(on ? .14f + .30f * drives[k] / 36 : .06f)); g.fillRect(br.reduced(1, 0));
            // transfer curve of this band's saturation
            auto tc = br.reduced(8, 8); if (tc.getWidth() > 20) {
                tc = tc.withSizeKeepingCentre(juce::jmin(tc.getWidth(), tc.getHeight() * 1.5f), tc.getHeight());
                Path tp; BandSat bs; float dr = drives[k];
                for (int i = 0; i <= 60; ++i) { float x = -1 + 2.0f * i / 60, y = types[k] >= 8 ? bs.tick(x, types[k], dr) : saturate(x, types[k], db(dr)); y = juce::jlimit(-1.2f, 1.2f, y);
                    float px = tc.getX() + tc.getWidth() * i / 60, py = tc.getCentreY() - y * tc.getHeight() * .42f; if (i == 0) tp.startNewSubPath(px, py); else tp.lineTo(px, py); }
                g.setColour(bc[k].withAlpha(on ? .9f : .35f)); g.strokePath(tp, juce::PathStrokeType(1.4f));
            }
            g.setColour(bc[k]); g.setFont(font(large ? 10.5f : 8.5f));
            g.drawText(names[k], Rectangle<float>(br.getX(), r.getBottom() - 16, br.getWidth(), 14), juce::Justification::centred);
            if (large) { g.setColour(col::dim); g.setFont(font(10.0f, false)); g.drawText(juce::String(satNames[(size_t) types[k]]) + "  " + juce::String(drives[k], 1) + " dB", br.reduced(4).toNearestInt(), juce::Justification::centredTop); }
        }
        g.setColour(col::faint); g.drawHorizontalLine((int) p.getCentreY(), p.getX(), p.getRight());
        for (int k = 0; k < 2; ++k) {
            float x = k == 0 ? x1 : x2; Colour c = k == 0 ? col::bandLo : col::bandHi;
            g.setColour(c.withAlpha(.8f)); g.drawVerticalLine((int) x, p.getY(), p.getBottom());
            g.setColour(drag == k ? col::text : c); g.fillEllipse(x - 4.5f, p.getCentreY() - 4.5f, 9, 9);
        }
        g.setFont(font(large ? 10.5f : 8.5f)); g.setColour(col::dim); g.drawText("CROSSOVER MAP", r.reduced(8, 3).removeFromTop(13), juce::Justification::centredLeft);
        g.setColour(col::color); g.drawText(hzText(ctx.live(SatLowX)), Rectangle<float>(x1 - 40, r.getY() + 3, 80, 13), juce::Justification::centred);
        g.drawText(hzText(ctx.live(SatHighX)), Rectangle<float>(juce::jmin(x2 - 40, r.getRight() - 86), r.getY() + 3, 80, 13), juce::Justification::centredRight);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        auto p = plot(); float x1 = logX(ctx.plain(SatLowX), p), x2 = logX(ctx.plain(SatHighX), p);
        drag = std::abs(e.position.x - x1) < std::abs(e.position.x - x2) ? 0 : 1;
        if (auto* pr = ctx.param(drag == 0 ? SatLowX : SatHighX)) pr->beginChangeGesture();
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override { if (drag >= 0) { ctx.setInGesture(drag == 0 ? SatLowX : SatHighX, xToHz(e.position.x, plot())); repaint(); } }
    void mouseUp(const juce::MouseEvent&) override { if (drag >= 0) if (auto* pr = ctx.param(drag == 0 ? SatLowX : SatHighX)) pr->endChangeGesture(); drag = -1; repaint(); }
private:
    Ctx& ctx; bool large; int drag = -1;
};

class EqCurve : public juce::Component {
public:
    EqCurve(Ctx& c, bool big) : ctx(c), large(big) {}
    Rectangle<float> plot() const { auto r = getLocalBounds().toFloat().reduced(6, 4); r.removeFromTop(large ? 18.0f : 12.0f); return r; }
    float yFor(float dB, Rectangle<float> p) const { return p.getCentreY() - dB / 20.0f * p.getHeight() * .5f; }
    static float response(float f, float lo, float hi, float gL, float gM, float gH, float bf, float bg, float bq, float sr) {
        auto lp = [](float f, float fc) { float x = std::pow(f / fc, 4.0f); return 1 / (1 + x); };
        hi = std::max(hi, lo * 1.25f);
        float L = lp(f, lo), H1 = 1 - L, M = H1 * lp(f, hi), Hh = H1 * (1 - lp(f, hi));
        float lin = L * db(gL) + M * db(gM) + Hh * db(gH);
        Biquad bq2; bq2.bell(sr, bf, bq, bg);
        float w = 2 * pi * f / sr; std::complex<float> z1 = std::polar(1.0f, -w), z2 = z1 * z1;
        auto h = (bq2.b0 + bq2.b1 * z1 + bq2.b2 * z2) / (1.0f + bq2.a1 * z1 + bq2.a2 * z2);
        return 20 * std::log10(lin * std::abs(h) + 1e-6f);
    }
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto p = plot(); freqGrid(g, p);
        bool on = ctx.live(EqOn) > .5f;
        g.setColour(Colour(0xff252d37)); g.drawHorizontalLine((int) p.getCentreY(), p.getX(), p.getRight());
        float lo = ctx.live(EqLowX), hi = ctx.live(EqHighX), gl = ctx.live(EqLow), gm = ctx.live(EqMid), gh = ctx.live(EqHigh);
        float bf = ctx.live(EqFreq), bgn = ctx.live(EqGain), bq = ctx.live(EqQ);
        float sr = (float) (ctx.p.getSampleRate() > 0 ? ctx.p.getSampleRate() : 48000.0);
        Path c;
        for (int i = 0; i <= 160; ++i) {
            float x = p.getX() + p.getWidth() * i / 160, f = xToHz(x, p);
            float y = yFor(juce::jlimit(-20.0f, 20.0f, response(f, lo, hi, gl, gm, gh, bf, bgn, bq, sr)), p);
            if (i == 0) c.startNewSubPath(x, y); else c.lineTo(x, y);
        }
        auto acc = col::color.withAlpha(on ? 1.0f : .35f);
        Path fill = c; fill.lineTo(p.getRight(), p.getCentreY()); fill.lineTo(p.getX(), p.getCentreY()); fill.closeSubPath();
        g.setColour(acc.withAlpha(on ? .16f : .05f)); g.fillPath(fill);
        g.setColour(acc); g.strokePath(c, juce::PathStrokeType(1.8f));
        float x1 = logX(lo, p), x2 = logX(std::max(hi, lo * 1.25f), p);
        g.setColour(col::bandLo.withAlpha(.6f)); g.drawVerticalLine((int) x1, p.getY(), p.getBottom());
        g.setColour(col::bandHi.withAlpha(.6f)); g.drawVerticalLine((int) x2, p.getY(), p.getBottom());
        auto h = handles();
        Colour hc[] { col::bandLo, col::bandHi, col::bandLo, col::bandMid, col::bandHi, col::text };
        for (int k = 0; k < 6; ++k) { g.setColour(drag == k ? col::text : hc[k]); float rr = k < 2 ? 3.5f : 4.5f; g.fillEllipse(h[(size_t) k].x - rr, h[(size_t) k].y - rr, rr * 2, rr * 2); }
        g.setFont(font(large ? 10.5f : 8.5f)); g.setColour(col::dim);
        g.drawText("POST EQ  " + hzText(lo) + " / " + hzText(hi), r.reduced(8, 2).removeFromTop(large ? 16.0f : 11.0f), juce::Justification::centredLeft);
    }
    std::array<Point<float>, 6> handles() const {
        auto p = plot(); float lo = ctx.plain(EqLowX), hi = std::max(ctx.plain(EqHighX), lo * 1.25f);
        float x1 = logX(lo, p), x2 = logX(hi, p);
        return { Point<float>(x1, p.getBottom() - 4), Point<float>(x2, p.getBottom() - 4),
                 Point<float>((p.getX() + x1) * .5f, yFor(ctx.plain(EqLow), p)), Point<float>((x1 + x2) * .5f, yFor(ctx.plain(EqMid), p)),
                 Point<float>((x2 + p.getRight()) * .5f, yFor(ctx.plain(EqHigh), p)), Point<float>(logX(ctx.plain(EqFreq), p), yFor(ctx.plain(EqGain), p)) };
    }
    void mouseDown(const juce::MouseEvent& e) override {
        auto h = handles(); float best = 1e9f; drag = -1;
        for (int k = 0; k < 6; ++k) { float d = h[(size_t) k].getDistanceFrom(e.position); if (d < best) { best = d; drag = k; } }
        for (int id : ids(drag)) if (auto* pr = ctx.param(id)) pr->beginChangeGesture();
        if (ctx.plain(EqOn) < .5f) ctx.set(EqOn, 1);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (drag < 0) return; auto p = plot();
        float gain = juce::jlimit(-18.0f, 18.0f, (p.getCentreY() - e.position.y) / (p.getHeight() * .5f) * 20.0f);
        switch (drag) {
            case 0: ctx.setInGesture(EqLowX, xToHz(e.position.x, p)); break;
            case 1: ctx.setInGesture(EqHighX, xToHz(e.position.x, p)); break;
            case 2: ctx.setInGesture(EqLow, gain); break;
            case 3: ctx.setInGesture(EqMid, gain); break;
            case 4: ctx.setInGesture(EqHigh, gain); break;
            case 5: ctx.setInGesture(EqFreq, xToHz(e.position.x, p)); ctx.setInGesture(EqGain, gain); break;
        }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override { for (int id : ids(drag)) if (auto* pr = ctx.param(id)) pr->endChangeGesture(); drag = -1; repaint(); }
    void mouseDoubleClick(const juce::MouseEvent&) override { for (int id : { EqLow, EqMid, EqHigh, EqGain }) ctx.set(id, 0); }
private:
    std::vector<int> ids(int k) const { switch (k) { case 0: return { EqLowX }; case 1: return { EqHighX }; case 2: return { EqLow }; case 3: return { EqMid }; case 4: return { EqHigh }; case 5: return { EqFreq, EqGain }; default: return {}; } }
    Ctx& ctx; bool large; int drag = -1;
};

// ================================================================ MOTION visual
class MotionView : public juce::Component {
public:
    MotionView(Ctx& c, bool big) : ctx(c), large(big) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto inner = r.reduced(8);
        auto orb = inner.removeFromLeft(juce::jmin(inner.getHeight(), inner.getWidth() * .5f)).reduced(2);
        inner.removeFromLeft(8);
        auto c = orb.getCentre(); float R = orb.getWidth() * .44f;
        g.setColour(Colour(0xff1e2630));
        for (float k : { .33f, .66f, 1.0f }) g.drawEllipse(c.x - R * k, c.y - R * k, 2 * R * k, 2 * R * k, 1);
        g.drawLine(c.x - R, c.y, c.x + R, c.y, 1); g.drawLine(c.x, c.y - R, c.x, c.y + R, 1);
        // head
        g.setColour(col::dim); g.fillEllipse(c.x - 6, c.y - 6, 12, 12);
        Path nose; nose.addTriangle(c.x - 3, c.y - 6, c.x + 3, c.y - 6, c.x, c.y - 10); g.fillPath(nose);
        bool on = ctx.live(OrbitOn) > .5f;
        for (size_t i = 0; i < ctx.trail.size(); ++i) {
            auto pt = ctx.trail[(i + (size_t) ctx.trailPos) % ctx.trail.size()]; float a = (float) i / ctx.trail.size();
            g.setColour(col::motion.withAlpha(a * .5f * (on ? 1.0f : .3f))); g.fillEllipse(c.x + pt.x * R - 2, c.y - pt.y * R - 2, 4, 4);
        }
        float ox = ctx.p.mOx.load(), oy = ctx.p.mOy.load(), oz = ctx.p.mOz.load();
        float dr = 4 + 3 * (oy + 1);
        g.setColour(col::motion.withAlpha(on ? 1.0f : .35f)); g.fillEllipse(c.x + ox * R - dr, c.y - oz * R - dr, dr * 2, dr * 2);
        g.setFont(font(large ? 10.5f : 8.0f)); g.setColour(col::dim);
        g.drawText("FAR", orb.withHeight(12), juce::Justification::centredTop);
        g.drawText("NEAR", orb.withTrimmedTop(orb.getHeight() - 12), juce::Justification::centredBottom);
        // pan bar
        auto pb = inner.removeFromTop(large ? 34.0f : 26.0f);
        g.drawText("AUTO PAN", pb.removeFromTop(11), juce::Justification::centredLeft);
        auto bar = pb.reduced(0, 3);
        g.setColour(Colour(0xff1b222b)); g.fillRoundedRectangle(bar, 3);
        float pan = ctx.p.mPan.load();
        float px = bar.getCentreX() + pan * bar.getWidth() * .5f;
        g.setColour(col::motion.withAlpha(ctx.live(PanOn) > .5f ? 1.0f : .3f)); g.fillRoundedRectangle(px - 3, bar.getY(), 6, bar.getHeight(), 2);
        g.setColour(col::faint); g.drawVerticalLine((int) bar.getCentreX(), bar.getY(), bar.getBottom());
        // gate history
        inner.removeFromTop(4);
        g.setColour(col::dim); g.drawText("GATE", inner.removeFromTop(11), juce::Justification::centredLeft);
        auto gr = inner.reduced(0, 2);
        g.setColour(Colour(0xff1b222b)); g.fillRoundedRectangle(gr, 3);
        Path gp; int n = Ctx::Hist;
        for (int k = 0; k < n; ++k) { float x = gr.getX() + gr.getWidth() * k / (n - 1), y = gr.getBottom() - ctx.hist(ctx.gateHist, k) * gr.getHeight() * .9f;
            if (k == 0) gp.startNewSubPath(x, y); else gp.lineTo(x, y); }
        g.setColour(Colour(0xfff0a05c).withAlpha(ctx.live(GateOn) > .5f ? 1.0f : .35f)); g.strokePath(gp, juce::PathStrokeType(1.4f));
    }
private:
    Ctx& ctx; bool large;
};

// ================================================================ PUMP visuals
class SpectrumView : public juce::Component {
public:
    SpectrumView(Ctx& c, bool big) : ctx(c), large(big) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto top = r.reduced(8, 4).removeFromTop(14);
        g.setFont(font(large ? 10.5f : 8.5f));
        g.setColour(col::text); g.drawText("DRY", top, juce::Justification::centredLeft);
        g.setColour(col::pump); g.drawText("WET  (REVERB > SATURATION > EQ > MOTION > PUMP)", top.withTrimmedLeft(30), juce::Justification::centredLeft);
        g.setColour(col::lime); g.drawText("DUCK", top, juce::Justification::centredRight);
        auto p = r.reduced(6); p.removeFromTop(16); p.removeFromBottom(12);
        freqGrid(g, p);
        auto path = [&](const std::array<float, Ctx::Spec>& d) {
            Path pa;
            for (int i = 0; i < Ctx::Spec; ++i) { float x = p.getX() + p.getWidth() * i / (Ctx::Spec - 1), v = juce::jlimit(0.0f, 1.0f, (d[(size_t) i] + 84) / 84);
                float y = p.getBottom() - v * p.getHeight(); if (i == 0) pa.startNewSubPath(x, y); else pa.lineTo(x, y); }
            return pa;
        };
        Path wet = path(ctx.wetDb), dry = path(ctx.dryDb), wf = wet;
        wf.lineTo(p.getRight(), p.getBottom()); wf.lineTo(p.getX(), p.getBottom()); wf.closeSubPath();
        g.setGradientFill(juce::ColourGradient(col::pump.withAlpha(.35f), 0, p.getY(), col::pump.withAlpha(.03f), 0, p.getBottom(), false)); g.fillPath(wf);
        g.setColour(col::pump); g.strokePath(wet, juce::PathStrokeType(1.6f));
        g.setColour(col::text.withAlpha(.75f)); g.strokePath(dry, juce::PathStrokeType(1.1f));
        // duck history (gain reduction hanging from the top)
        Path dp; int n = Ctx::Hist;
        for (int k = 0; k < n; ++k) { float x = p.getX() + p.getWidth() * k / (n - 1), grDb = juce::jlimit(0.0f, 48.0f, -20 * std::log10(juce::jmax(1e-4f, ctx.hist(ctx.grHist, k))));
            float y = p.getY() + grDb / 48.0f * p.getHeight() * .6f; if (k == 0) dp.startNewSubPath(x, y); else dp.lineTo(x, y); }
        g.setColour(col::lime.withAlpha(.85f)); g.strokePath(dp, juce::PathStrokeType(1.3f));
        g.setColour(col::dim); g.setFont(font(large ? 10.0f : 8.0f, false));
        auto ax = Rectangle<float>(p.getX(), r.getBottom() - 13, p.getWidth(), 11);
        g.drawText("20 Hz", ax, juce::Justification::centredLeft); g.drawText("20 kHz", ax, juce::Justification::centredRight);
        g.drawText("1 kHz", Rectangle<float>(logX(1000, p) - 30, ax.getY(), 60, 11), juce::Justification::centred);
    }
private:
    Ctx& ctx; bool large;
};

class PumpShape : public juce::Component {
public:
    explicit PumpShape(Ctx& c) : ctx(c) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        auto p = r.reduced(8, 6); p.removeFromTop(11);
        g.setFont(font(8.5f)); g.setColour(col::dim); g.drawText("PUMP SHAPE", r.reduced(8, 3).removeFromTop(12), juce::Justification::centredLeft);
        int mode = (int) ctx.live(DuckMode); float depth = ctx.live(DuckDepth), curve = ctx.live(PumpCurve);
        Path pa;
        for (int i = 0; i <= 80; ++i) {
            float ph = std::fmod(i / 40.0f, 1.0f), gDb = mode == 0 ? 0 : depth * std::pow(1 - ph, curve);
            float x = p.getX() + p.getWidth() * i / 80, y = p.getY() + juce::jlimit(0.0f, 1.0f, gDb / 48.0f) * p.getHeight();
            if (i == 0) pa.startNewSubPath(x, y); else pa.lineTo(x, y);
        }
        g.setColour(col::pump.withAlpha(mode == 0 ? .3f : 1.0f)); g.strokePath(pa, juce::PathStrokeType(1.6f));
        g.setColour(col::dim); g.setFont(font(8.0f, false)); g.drawText(mode == 0 ? "envelope only" : juce::String(syncNames[(size_t) ctx.live(PumpSync)]) + " pump", p.toNearestInt(), juce::Justification::bottomRight);
    }
private:
    Ctx& ctx;
};

class GrBar : public juce::Component {
public:
    explicit GrBar(Ctx& c) : ctx(c) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(col::dim); g.setFont(font(9.5f)); g.drawText("DUCK REDUCTION", r.removeFromTop(12), juce::Justification::centredLeft);
        float grDb = juce::jlimit(0.0f, 48.0f, -20 * std::log10(juce::jmax(1e-4f, ctx.p.mDuck.load())));
        auto valR = r.removeFromRight(62);
        g.setColour(col::text); g.setFont(font(12.0f)); g.drawText((grDb < .05f ? juce::String("0.0") : "-" + juce::String(grDb, 1)) + " dB", valR, juce::Justification::centredRight);
        auto bar = r.withSizeKeepingCentre(r.getWidth(), 6);
        g.setColour(col::well); g.fillRoundedRectangle(bar, 3);
        g.setColour(col::pump); g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * grDb / 48.0f), 3);
    }
private:
    Ctx& ctx;
};

class LfoScope : public juce::Component {
public:
    explicit LfoScope(Ctx& c) : ctx(c) {}
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat(); drawWell(g, r);
        Colour cs[] { col::lime, col::motion, col::pump, col::space, col::text, col::color };
        const char* names[] { "LFO 1", "LFO 2", "LFO 3", "LFO 4", "AUDIO ENV", "MIDI ENV" };
        auto p = r.reduced(10); auto legend = p.removeFromTop(16);
        float lw = legend.getWidth() / 6;
        for (int k = 0; k < 6; ++k) { g.setColour(cs[k]); g.setFont(font(10.0f)); g.drawText(names[k], legend.removeFromLeft(lw), juce::Justification::centredLeft); }
        int n = Ctx::Hist;
        for (int k = 0; k < 6; ++k) {
            Path pa;
            for (int i = 0; i < n; ++i) {
                float v = k < 4 ? ctx.hist(ctx.lfoHist[(size_t) k], i) : (k == 4 ? ctx.hist(ctx.envHist, i) : ctx.hist(ctx.midiHist, i)) * 2 - 1;
                float x = p.getX() + p.getWidth() * i / (n - 1), y = p.getCentreY() - v * p.getHeight() * .45f;
                if (i == 0) pa.startNewSubPath(x, y); else pa.lineTo(x, y);
            }
            g.setColour(cs[k].withAlpha(.85f)); g.strokePath(pa, juce::PathStrokeType(1.3f));
        }
    }
private:
    Ctx& ctx;
};

// ================================================================ module panels (ALL IN ONE)
using Owned = std::vector<std::unique_ptr<juce::Component>>;

class SpacePanel : public Panel {
public:
    SpacePanel(Ctx& c, std::vector<LiveCtl*>& lives) : Panel("SPACE", col::space), ctx(c) {
        for (int i = 0; i < Modes; ++i) modeBox.addItem(modeNames[(size_t) i], i + 1);
        modeBox.onChange = [this] { if (!syncing) ctx.set(RType, (float) (modeBox.getSelectedItemIndex())); };
        modeBox.setTooltip("Reverb mode (sets the reverb type)");
        addAndMakeVisible(modeBox);
        selMode = &add<Combo>(*this, owned, c, SelMode);
        type = &add<Knob>(*this, owned, c, RType, col::lime, "REVERB TYPE"); lives.push_back(type);
        tail = &add<TailField>(*this, owned, c, false);
        strip = &add<ModeStrip>(*this, owned, c);
        morph = &add<HSlider>(*this, owned, c, MorphTime, col::space, "MORPH TIME"); lives.push_back(morph);
        cycle = &add<HSlider>(*this, owned, c, CycleRate, col::space, "CYCLE RATE"); lives.push_back(cycle);
        cycleSync = &add<Combo>(*this, owned, c, CycleSync);
        int ks[] { Size, Decay, PreDelay, Damping, Diffusion, Width };
        const char* kn[] { "SIZE", "DECAY", "PRE-DELAY", "DAMPING", "DIFFUSION", "WIDTH" };
        for (int k = 0; k < 6; ++k) { knobs[k] = &add<Knob>(*this, owned, c, ks[k], k == 5 ? col::motion : col::lime, kn[k]); lives.push_back(knobs[k]); }
        mix = &add<HSlider>(*this, owned, c, Mix, col::lime, "WET / DRY"); lives.push_back(mix);
        rv = &add<Led>(*this, owned, c, ReverbOn, col::lime, "REVERB");
        cuts = &add<Led>(*this, owned, c, CutsOn, col::lime, "REVERB CUTS");
        lo = &add<HSlider>(*this, owned, c, LowCut, col::motion, "LOW CUT"); lives.push_back(lo);
        hi = &add<HSlider>(*this, owned, c, HighCut, col::motion, "HIGH CUT"); lives.push_back(hi);
        lo->valueW = hi->valueW = 58;
    }
    void resized() override {
        modeBox.setBounds(12, 46, 196, 24); selMode->setBounds(214, 46, 96, 24);
        type->setBounds(6, 76, 98, 104);
        tail->setBounds(108, 78, 204, 86); strip->setBounds(108, 167, 204, 13);
        morph->setBounds(12, 184, 298, 26); cycle->setBounds(12, 184, 210, 26); cycleSync->setBounds(230, 196, 80, 18);
        for (int k = 0; k < 6; ++k) knobs[k]->setBounds(11 + (k % 3) * 100, 214 + (k / 3) * 80, 100, 80);
        mix->setBounds(12, 376, 298, 26);
        rv->setBounds(12, 408, 146, 22); cuts->setBounds(164, 408, 146, 22);
        lo->setBounds(12, 436, 146, 26); hi->setBounds(164, 436, 146, 26);
    }
    void paintExtra(Graphics& g) override {
        g.setColour(col::dim); g.setFont(font(9.5f));
        g.drawText("MODE", 12, 33, 196, 12, juce::Justification::centred);
        g.drawText("SELECT", 214, 33, 96, 12, juce::Justification::centred);
    }
    void tick() {
        bool cyc = (int) ctx.plain(SelMode) == 2;
        morph->setVisible(!cyc); cycle->setVisible(cyc); cycleSync->setVisible(cyc);
        if (!modeBox.isPopupActive()) {
            int m = ((int) std::round(ctx.typePos()) % Modes + Modes) % Modes;
            if (modeBox.getSelectedItemIndex() != m) { syncing = true; modeBox.setSelectedItemIndex(m, juce::dontSendNotification); syncing = false; }
        }
        if (ctx.visuals) { tail->repaint(); strip->repaint(); }
    }
private:
    Ctx& ctx; Owned owned; juce::ComboBox modeBox; bool syncing = false;
    Combo* selMode; Knob* type; TailField* tail; ModeStrip* strip; HSlider *morph, *cycle; Combo* cycleSync;
    Knob* knobs[6]; HSlider* mix; Led *rv, *cuts; HSlider *lo, *hi;
};

class ColorPanel : public Panel {
public:
    ColorPanel(Ctx& c, std::vector<LiveCtl*>& lives) : Panel("COLOR", col::color), ctx(c) {
        sat = &add<Led>(*this, owned, c, SatOn, col::lime, "SATURATION");
        satMix = &add<HSlider>(*this, owned, c, SatMix, col::color, "SAT WET / DRY"); lives.push_back(satMix);
        int ts[] { LowType, MidType, HighType }, ds[] { LowDrive, MidDrive, HighDrive };
        const char* dn[] { "LOW DRIVE", "MID DRIVE", "HIGH DRIVE" };
        Colour bc[] { col::bandLo, col::bandMid, col::bandHi };
        for (int k = 0; k < 3; ++k) { types[k] = &add<Combo>(*this, owned, c, ts[k]); drives[k] = &add<Knob>(*this, owned, c, ds[k], bc[k], dn[k]); lives.push_back(drives[k]); }
        map = &add<CrossoverMap>(*this, owned, c, false);
        eq = &add<Led>(*this, owned, c, EqOn, col::color, "POST EQ");
        curve = &add<EqCurve>(*this, owned, c, false);
        int gs[] { EqLow, EqMid, EqHigh }; const char* gn[] { "LOW", "MID", "HIGH" };
        for (int k = 0; k < 3; ++k) { gains[k] = &add<Knob>(*this, owned, c, gs[k], bc[k], gn[k]); lives.push_back(gains[k]); }
    }
    void resized() override {
        sat->setBounds(12, 36, 300, 22);
        satMix->setBounds(12, 62, 300, 26);
        for (int k = 0; k < 3; ++k) { types[k]->setBounds(12 + k * 101, 104, 97, 22); drives[k]->setBounds(12 + k * 101, 128, 97, 78); }
        map->setBounds(12, 210, 300, 92);
        eq->setBounds(12, 306, 300, 20);
        curve->setBounds(12, 330, 300, 64);
        for (int k = 0; k < 3; ++k) gains[k]->setBounds(12 + k * 101, 396, 97, 70);
    }
    void paintExtra(Graphics& g) override {
        g.setColour(col::dim); g.setFont(font(9.5f));
        const char* n[] { "LOW BAND", "MID BAND", "HIGH BAND" };
        for (int k = 0; k < 3; ++k) g.drawText(n[k], 12 + k * 101, 91, 97, 12, juce::Justification::centredLeft);
    }
    void tick() { if (ctx.visuals) { map->repaint(); curve->repaint(); } }
private:
    Ctx& ctx; Owned owned; Led *sat, *eq; HSlider* satMix; Combo* types[3]; Knob *drives[3], *gains[3]; CrossoverMap* map; EqCurve* curve;
};

class MotionPanel : public Panel {
public:
    MotionPanel(Ctx& c, std::vector<LiveCtl*>& lives) : Panel("MOTION", col::motion), ctx(c) {
        pan = &add<Led>(*this, owned, c, PanOn, col::motion, "AUTO PAN"); panSync = &add<Combo>(*this, owned, c, PanSync);
        int pk[] { PanRate, PanDepth, PanShape }; const char* pn[] { "RATE", "DEPTH", "SHAPE" };
        for (int k = 0; k < 3; ++k) { panK[k] = &add<Knob>(*this, owned, c, pk[k], col::motion, pn[k]); lives.push_back(panK[k]); }
        orbit = &add<Led>(*this, owned, c, OrbitOn, col::pump, "3D SPACE"); orbitMode = &add<Combo>(*this, owned, c, OrbitMode); orbitSync = &add<Combo>(*this, owned, c, OrbitSync);
        int ok[] { OrbitRate, Around, UpDown, NearFar }; const char* on[] { "RATE", "AROUND", "UP / DOWN", "NEAR / FAR" };
        for (int k = 0; k < 4; ++k) { orbK[k] = &add<Knob>(*this, owned, c, ok[k], col::pump, on[k]); lives.push_back(orbK[k]); }
        gate = &add<Led>(*this, owned, c, GateOn, Colour(0xfff0a05c), "GATE"); gateMode = &add<Combo>(*this, owned, c, GateMode); gateSync = &add<Combo>(*this, owned, c, GateSync);
        int gk[] { GateRate, GateDepth, GateShape, GateSmooth }; const char* gn[] { "RATE", "AMOUNT", "SHAPE", "SMOOTH" };
        for (int k = 0; k < 4; ++k) { gateK[k] = &add<Knob>(*this, owned, c, gk[k], Colour(0xfff0a05c), gn[k]); lives.push_back(gateK[k]); }
        view = &add<MotionView>(*this, owned, c, false);
    }
    void resized() override {
        pan->setBounds(12, 36, 150, 22); panSync->setBounds(168, 36, 142, 22);
        for (int k = 0; k < 3; ++k) panK[k]->setBounds(12 + k * 100, 60, 98, 84);
        orbit->setBounds(12, 148, 110, 22); orbitMode->setBounds(126, 148, 104, 22); orbitSync->setBounds(234, 148, 76, 22);
        for (int k = 0; k < 4; ++k) orbK[k]->setBounds(10 + k * 76, 172, 76, 72);
        gate->setBounds(12, 248, 90, 22); gateMode->setBounds(106, 248, 108, 22); gateSync->setBounds(218, 248, 92, 22);
        for (int k = 0; k < 4; ++k) gateK[k]->setBounds(10 + k * 76, 272, 76, 84);
        view->setBounds(12, 360, 298, 104);
    }
    void tick() {
        bool rh = (int) ctx.plain(GateMode) == 0;
        gateSync->setEnabled(rh); gateK[0]->setEnabled(rh); gateK[2]->setEnabled(rh);
        gateK[0]->setAlpha(rh ? 1.0f : .4f); gateK[2]->setAlpha(rh ? 1.0f : .4f);
        if (ctx.visuals) view->repaint();
    }
private:
    Ctx& ctx; Owned owned; Led *pan, *orbit, *gate; Combo *panSync, *orbitMode, *orbitSync, *gateMode, *gateSync; Knob *panK[3], *orbK[4], *gateK[4]; MotionView* view;
};

class PumpPanel : public Panel {
public:
    PumpPanel(Ctx& c, std::vector<LiveCtl*>& lives) : Panel("PUMP", col::pump), ctx(c) {
        duck = &add<Led>(*this, owned, c, DuckOn, col::pump, "DUCKING"); src = &add<Combo>(*this, owned, c, DuckSource);
        mode = &add<Combo>(*this, owned, c, DuckMode); sync = &add<Combo>(*this, owned, c, PumpSync);
        int k1[] { DuckDepth, DuckThresh, DuckAttack, DuckRelease }; const char* n1[] { "DEPTH", "SENSITIVITY", "ATTACK", "RELEASE" };
        for (int k = 0; k < 4; ++k) { knobs[k] = &add<Knob>(*this, owned, c, k1[k], col::pump, n1[k]); lives.push_back(knobs[k]); }
        curveK = &add<Knob>(*this, owned, c, PumpCurve, col::pump, "CURVE"); lives.push_back(curveK);
        mixK = &add<Knob>(*this, owned, c, DuckMix, col::lime, "DUCK MIX"); lives.push_back(mixK);
        shape = &add<PumpShape>(*this, owned, c);
        gr = &add<GrBar>(*this, owned, c);
        spec = &add<SpectrumView>(*this, owned, c, false);
    }
    void resized() override {
        duck->setBounds(12, 36, 190, 22); src->setBounds(208, 36, 110, 22);
        mode->setBounds(12, 64, 150, 20); sync->setBounds(168, 64, 150, 20);
        for (int k = 0; k < 4; ++k) knobs[k]->setBounds(8 + k * 78, 90, 78, 80);
        curveK->setBounds(8, 172, 78, 76); mixK->setBounds(86, 172, 78, 76);
        shape->setBounds(170, 178, 148, 66);
        gr->setBounds(12, 252, 306, 26);
        spec->setBounds(12, 284, 306, 180);
    }
    void tick() {
        bool tempo = (int) ctx.plain(DuckMode) != 0; sync->setEnabled(tempo); curveK->setAlpha(tempo ? 1.0f : .4f);
        gr->repaint(); if (ctx.visuals) { spec->repaint(); shape->repaint(); }
    }
private:
    Ctx& ctx; Owned owned; Led* duck; Combo *src, *mode, *sync; Knob *knobs[4], *curveK, *mixK; PumpShape* shape; GrBar* gr; SpectrumView* spec;
};

// ================================================================ MODULATION MATRIX
class LaneRow : public juce::Component, public LiveCtl {
public:
    LaneRow(Ctx& c, int lane, Colour tick) : ctx(c), n(lane), tc(tick),
        src(c, laneP(lane, 0), true), dest(c, laneP(lane, 1), true), dir(c, laneP(lane, 2), true),
        depth(c, laneP(lane, 3), col::lime, {}, true, false) {
        for (juce::Component* k : { (juce::Component*) &src, (juce::Component*) &dest, (juce::Component*) &dir, (juce::Component*) &depth }) addAndMakeVisible(k);
    }
    void resized() override {
        src.setBounds(10, 1, 150, 22); dest.setBounds(166, 1, 280, 22); dir.setBounds(452, 1, 120, 22); depth.setBounds(584, 0, getWidth() - 590, 24);
    }
    void paint(Graphics& g) override {
        g.setColour(Colour(0xff12171d)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 4);
        g.setColour(tc); g.fillRoundedRectangle(2, 3, 3, (float) getHeight() - 6, 1.5f);
    }
    void updateLive() override { depth.updateLive(); }
private:
    Ctx& ctx; int n; Colour tc; Combo src, dest, dir; HSlider depth;
};

class LfoBlock : public juce::Component {
public:
    LfoBlock(Ctx& c, int l, std::vector<LiveCtl*>& lives, Colour accent) : ctx(c), idx(l), acc(accent),
        shape(c, lfoP(l, 0), accent, "LFO " + juce::String(l + 1)), rate(c, lfoP(l, 1), col::lime, "RATE"), sync(c, lfoP(l, 2)), trig(c, lfoP(l, 3)) {
        for (juce::Component* k : { (juce::Component*) &shape, (juce::Component*) &rate, (juce::Component*) &sync, (juce::Component*) &trig }) addAndMakeVisible(k);
        lives.push_back(&shape); lives.push_back(&rate); rate.valueW = 48;
    }
    void resized() override {
        shape.setBounds(0, 0, 56, getHeight());
        rate.setBounds(58, 6, getWidth() - 58, 26);
        int cw = (getWidth() - 62) / 2;
        sync.setBounds(58, 38, cw, 18); trig.setBounds(62 + cw, 38, cw, 18);
    }
    void paint(Graphics& g) override {
        auto r = Rectangle<float>(58, 62, (float) getWidth() - 58, 8);
        g.setColour(col::well); g.fillRoundedRectangle(r, 3);
        float v = ctx.p.mLfo[(size_t) idx].load();
        g.setColour(acc); g.fillRoundedRectangle(r.getCentreX() + juce::jmin(0.0f, v) * r.getWidth() * .5f, r.getY(), std::abs(v) * r.getWidth() * .5f, r.getHeight(), 3);
    }
private:
    Ctx& ctx; int idx; Colour acc; Knob shape; HSlider rate; Combo sync, trig;
};

class MatrixPanel : public Panel {
public:
    MatrixPanel(Ctx& c, std::vector<LiveCtl*>& lives) : Panel("MODULATION MATRIX", col::matrix), ctx(c) {
        int es[] { EnvThresh, EnvAttack, EnvRelease, MidiAttack, MidiRelease };
        const char* en[] { "AUDIO THRESH", "ENV ATTACK", "ENV RELEASE", "MIDI ATTACK", "MIDI RELEASE" };
        for (int k = 0; k < 5; ++k) { env[k] = &add<HSlider>(*this, owned, c, es[k], col::lime, en[k]); env[k]->valueW = 44; lives.push_back(env[k]); }
        Colour lc[] { col::lime, col::motion, col::pump, col::space };
        for (int l = 0; l < Lfos; ++l) lfos[l] = &add<LfoBlock>(*this, owned, c, l, lives, lc[l]);
        addLane.setButtonText("+ ADD LANE"); removeLane.setButtonText("- REMOVE"); hide.setButtonText("HIDE MATRIX");
        for (auto* b : { &addLane, &removeLane, &hide }) addAndMakeVisible(b);
        addLane.onClick = [this] { ctx.p.setLaneCount(ctx.p.laneCount() + 1); layoutLanes(); };
        removeLane.onClick = [this] {
            int n = ctx.p.laneCount(); if (n <= 1) return;
            ctx.set(laneP(n - 1, 1), 0); ctx.set(laneP(n - 1, 3), 0);
            ctx.p.setLaneCount(n - 1); layoutLanes();
        };
        Colour tcs[] { col::lime, col::motion, col::pump, col::space, col::color, col::matrix };
        for (int n = 0; n < Lanes; ++n) { rows[(size_t) n] = std::make_unique<LaneRow>(c, n, tcs[n % 6]); content.addChildComponent(*rows[(size_t) n]); lives.push_back(rows[(size_t) n].get()); }
        view.setViewedComponent(&content, false); view.setScrollBarsShown(true, false); view.setScrollBarThickness(8);
        addAndMakeVisible(view);
    }
    void resized() override {
        for (int k = 0; k < 5; ++k) env[k]->setBounds(16 + k * 90, 38, 84, 28);
        int x0 = 470, lw = (getWidth() - x0 - 12) / Lfos;
        for (int l = 0; l < Lfos; ++l) lfos[l]->setBounds(x0 + l * lw, 30, lw - 8, 74);
        addLane.setBounds(16, 80, 100, 20); removeLane.setBounds(122, 80, 90, 20);
        view.setBounds(16, 122, getWidth() - 32, getHeight() - 122 - 34);
        hide.setBounds(getWidth() - 126, getHeight() - 30, 110, 22);
        layoutLanes();
    }
    void layoutLanes() {
        int n = ctx.p.laneCount(), w = view.getWidth() - (n * 26 > view.getHeight() ? 10 : 0);
        content.setSize(juce::jmax(10, w), n * 26);
        for (int i = 0; i < Lanes; ++i) { rows[(size_t) i]->setVisible(i < n); rows[(size_t) i]->setBounds(0, i * 26, w, 24); }
        shownLanes = n; repaint();
    }
    void paintExtra(Graphics& g) override {
        g.setColour(col::dim); g.setFont(font(9.5f));
        g.drawText("SOURCE", 30, 108, 140, 12, juce::Justification::centredLeft);
        g.drawText("DESTINATION", 186, 108, 200, 12, juce::Justification::centredLeft);
        g.drawText("DIRECTION", 472, 108, 100, 12, juce::Justification::centredLeft);
        g.drawText("DEPTH", 604, 108, 100, 12, juce::Justification::centredLeft);
        g.drawText(juce::String(shownLanes) + " / 16 LANES", 220, 84, 120, 12, juce::Justification::centredLeft);
    }
    void tick() { if (ctx.p.laneCount() != shownLanes) layoutLanes(); for (auto* l : lfos) l->repaint(); }
    juce::TextButton hide;
private:
    Ctx& ctx; Owned owned; HSlider* env[5]; LfoBlock* lfos[Lfos];
    juce::TextButton addLane, removeLane; juce::Viewport view; juce::Component content;
    std::array<std::unique_ptr<LaneRow>, Lanes> rows; int shownLanes = 0;
};

// ================================================================ MODULES tab: every parameter of a module + big visual
class ModulePage : public Panel {
public:
    ModulePage(Ctx& c, int page, std::vector<LiveCtl*>& lives) : Panel(pageTitle(page), pageColour(page)), ctx(c), pg(page) {
        if (page < 5) {
            std::vector<int> ids;
            for (int i = 0; i < LfoBase; ++i) { int gr = spec(i).group; if (gr == page || (page == 4 && gr == 5)) ids.push_back(i); }
            for (int i : ids) {
                auto& s = spec(i); juce::Component* comp;
                if (s.kind == Float) { auto& k = add<Knob>(*this, owned, c, i, acc); lives.push_back(&k); comp = &k; }
                else if (s.kind == Toggle) comp = &add<Led>(*this, owned, c, i, acc, "ON");
                else comp = &add<Combo>(*this, owned, c, i);
                cells.push_back({ comp, s.kind, i });
            }
        }
        switch (page) {
            case 0: big.push_back(&add<TailField>(*this, owned, c, true)); big.push_back(&add<ModeStrip>(*this, owned, c)); break;
            case 1: big.push_back(&add<CrossoverMap>(*this, owned, c, true)); big.push_back(&add<EqCurve>(*this, owned, c, true)); break;
            case 2: big.push_back(&add<MotionView>(*this, owned, c, true)); break;
            case 3: big.push_back(&add<SpectrumView>(*this, owned, c, true)); big.push_back(&add<PumpShape>(*this, owned, c)); break;
            case 4: big.push_back(&add<LfoScope>(*this, owned, c)); break;
            default: big.push_back(&add<SpectrumView>(*this, owned, c, true)); big.push_back(&add<TailField>(*this, owned, c, true));
                     big.push_back(&add<MotionView>(*this, owned, c, true)); big.push_back(&add<CrossoverMap>(*this, owned, c, true)); break;
        }
    }
    static juce::String pageTitle(int p) { const char* t[] { "SPACE  /  ALL PARAMETERS", "COLOR  /  ALL PARAMETERS", "MOTION  /  ALL PARAMETERS", "PUMP  /  ALL PARAMETERS", "MOD SOURCES  /  MACROS  /  GLOBAL", "VISUALS" }; return t[p]; }
    static Colour pageColour(int p) { Colour c[] { col::space, col::color, col::motion, col::pump, col::matrix, col::lime }; return c[p]; }
    void resized() override {
        auto area = getLocalBounds().reduced(12); area.removeFromTop(26);
        if (pg == 5) {
            auto top = area.removeFromTop(area.getHeight() / 2 - 4); area.removeFromTop(8);
            big[0]->setBounds(top.removeFromLeft(top.getWidth() * 3 / 5)); top.removeFromLeft(8); big[1]->setBounds(top);
            big[2]->setBounds(area.removeFromLeft(area.getWidth() * 2 / 5)); area.removeFromLeft(8); big[3]->setBounds(area);
            return;
        }
        auto vis = area.removeFromRight(480); area.removeFromRight(10);
        const int cw = 104, ch = 96, cols = juce::jmax(1, area.getWidth() / cw);
        int x = 0, y = 0;
        for (auto& c : cells) {
            auto cell = Rectangle<int>(area.getX() + x * cw, area.getY() + y * ch, cw, ch).reduced(3);
            if (c.kind == Float) c.comp->setBounds(cell);
            else c.comp->setBounds(cell.withSizeKeepingCentre(cell.getWidth(), 24).translated(0, 6));
            if (++x == cols) { x = 0; ++y; }
        }
        if (big.size() == 2) {
            int h0 = pg == 0 ? vis.getHeight() - 40 : pg == 3 ? vis.getHeight() - 110 : vis.getHeight() / 2 - 4;
            big[0]->setBounds(vis.removeFromTop(h0)); vis.removeFromTop(8); big[1]->setBounds(pg == 0 ? vis.removeFromTop(26) : vis);
        } else big[0]->setBounds(vis);
    }
    void paintExtra(Graphics& g) override {
        g.setColour(col::dim); g.setFont(font(9.5f));
        for (auto& c : cells) if (c.kind != Float) g.drawText(upper(spec(c.idx).name), c.comp->getBounds().translated(0, -16).withHeight(14), juce::Justification::centred);
    }
    void tick() { if (ctx.visuals) for (auto* b : big) b->repaint(); }
private:
    struct Cell { juce::Component* comp; Kind kind; int idx; };
    Ctx& ctx; int pg; Owned owned; std::vector<Cell> cells; std::vector<juce::Component*> big;
};

// ================================================================ header
class PresetBar : public juce::Component {
public:
    explicit PresetBar(VelvetSpaceProcessor& p) : proc(p) {
        prev.setButtonText("<"); next.setButtonText(">");
        prev.onClick = [this] { proc.presets.step(-1); repaint(); }; next.onClick = [this] { proc.presets.step(1); repaint(); };
        addAndMakeVisible(prev); addAndMakeVisible(next);
        prev.setTooltip("Previous preset"); next.setTooltip("Next preset");
    }
    void resized() override { prev.setBounds(0, 0, 30, getHeight()); next.setBounds(getWidth() - 30, 0, 30, getHeight()); }
    void paint(Graphics& g) override {
        auto r = getLocalBounds().toFloat().reduced(34, 0).reduced(.5f);
        g.setColour(Colour(0xff0f1318)); g.fillRoundedRectangle(r, 5);
        g.setColour(Colour(0xff3a4350)); g.drawRoundedRectangle(r, 5, 1);
        g.setColour(col::text); g.setFont(font(14.0f)); g.drawText(proc.presetName, r.toNearestInt(), juce::Justification::centred);
        Path p; float ax = r.getRight() - 16, ay = r.getCentreY();
        p.startNewSubPath(ax - 5, ay - 2.5f); p.lineTo(ax, ay + 2.5f); p.lineTo(ax + 5, ay - 2.5f);
        g.strokePath(p, juce::PathStrokeType(1.6f));
        auto& e = proc.presets.entries(); int i = proc.presets.currentIndex();
        if (i >= 0 && i < (int) e.size()) { g.setColour(col::dim); g.setFont(font(10.0f)); g.drawText(e[(size_t) i].category.toUpperCase(), r.reduced(12, 0).toNearestInt(), juce::Justification::centredLeft); }
    }
    void mouseDown(const juce::MouseEvent&) override {
        juce::PopupMenu m; auto& e = proc.presets.entries();
        juce::StringArray cats; for (auto& x : e) cats.addIfNotAlreadyThere(x.category);
        for (auto& c : cats) {
            juce::PopupMenu sub;
            for (int i = 0; i < (int) e.size(); ++i) if (e[(size_t) i].category == c) sub.addItem(i + 1, e[(size_t) i].name, true, i == proc.presets.currentIndex());
            m.addSubMenu(c, sub);
        }
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMinimumWidth(getWidth() - 68), [this](int r) { if (r > 0) { proc.presets.load(r - 1); repaint(); } });
    }
private:
    VelvetSpaceProcessor& proc; juce::TextButton prev, next;
};

class VisualsLed : public juce::ToggleButton {
public:
    void paintButton(Graphics& g, bool over, bool) override {
        auto r = getLocalBounds().toFloat().reduced(.5f); bool on = getToggleState();
        g.setColour(over ? Colour(0xff1b2129) : Colour(0xff12171d)); g.fillRoundedRectangle(r, 5);
        g.setColour(on ? col::lime.withAlpha(.7f) : Colour(0xff3a4350)); g.drawRoundedRectangle(r, 5, 1);
        g.setColour(on ? col::text : col::dim); g.setFont(font(10.5f)); g.drawText("VISUALS", r.reduced(9, 0), juce::Justification::centredLeft);
        auto lc = Point<float>(r.getRight() - 14, r.getCentreY());
        if (on) { g.setColour(col::lime.withAlpha(.25f)); g.fillEllipse(lc.x - 7, lc.y - 7, 14, 14); }
        g.setColour(on ? col::lime : Colour(0xff2a313b)); g.fillEllipse(lc.x - 4, lc.y - 4, 8, 8);
    }
};

// ================================================================ root UI
struct EditorUi : public juce::Component, private juce::Timer {
    EditorUi(VelvetSpaceProcessor& p, VelvetSpaceEditor& e) : ctx(p), ed(e), presetBar(p), fft(11) {
        setLookAndFeel(&lnf);
        addAndMakeVisible(presetBar);
        save.setButtonText("SAVE PRESET"); menu.setButtonText(juce::String::fromUTF8("\xe2\x80\xa2\xe2\x80\xa2\xe2\x80\xa2"));
        allTab.setButtonText("ALL IN ONE"); modTab.setButtonText("MODULES");
        for (auto* b : { &save, &menu, &allTab, &modTab, &showMatrix }) addAndMakeVisible(b);
        showMatrix.setButtonText("SHOW MATRIX");
        save.onClick = [this] { savePreset(); };
        menu.onClick = [this] { showMenu(); };
        menu.setTooltip("Preset and window options");
        allTab.setClickingTogglesState(true); modTab.setClickingTogglesState(true);
        allTab.setRadioGroupId(1); modTab.setRadioGroupId(1); allTab.setToggleState(true, juce::dontSendNotification);
        allTab.onClick = [this] { setTab(0); }; modTab.onClick = [this] { setTab(1); };
        visualsBtn.setClickingTogglesState(true); visualsBtn.setToggleState(true, juce::dontSendNotification);
        visualsBtn.onClick = [this] { ctx.visuals = visualsBtn.getToggleState(); repaint(); };
        visualsBtn.setTooltip("Animate graphs and meters");
        addAndMakeVisible(visualsBtn);
        output = std::make_unique<HSlider>(ctx, Output, col::lime, "OUTPUT"); addAndMakeVisible(*output); lives.push_back(output.get());
        spaceP = std::make_unique<SpacePanel>(ctx, lives); colorP = std::make_unique<ColorPanel>(ctx, lives);
        motionP = std::make_unique<MotionPanel>(ctx, lives); pumpP = std::make_unique<PumpPanel>(ctx, lives);
        matrixP = std::make_unique<MatrixPanel>(ctx, lives);
        for (juce::Component* c : { (juce::Component*) spaceP.get(), (juce::Component*) colorP.get(), (juce::Component*) motionP.get(), (juce::Component*) pumpP.get(), (juce::Component*) matrixP.get() }) addAndMakeVisible(c);
        matrixP->hide.onClick = [this] { ed.matrixShown = false; ed.applySize(); layoutMatrix(); };
        showMatrix.onClick = [this] { ed.matrixShown = true; ed.applySize(); layoutMatrix(); };
        const char* sub[] { "SPACE", "COLOR", "MOTION", "PUMP", "MOD / GLOBAL", "VISUALS" };
        for (int i = 0; i < 6; ++i) {
            pageBtn[i].setButtonText(sub[i]); pageBtn[i].setClickingTogglesState(true); pageBtn[i].setRadioGroupId(2);
            pageBtn[i].onClick = [this, i] { setPage(i); }; addChildComponent(pageBtn[i]);
            pages[i] = std::make_unique<ModulePage>(ctx, i, lives); addChildComponent(*pages[i]);
        }
        pageBtn[0].setToggleState(true, juce::dontSendNotification);
        for (int i = 0; i < 2048; ++i) window[(size_t) i] = .5f - .5f * std::cos(2 * pi * i / 2047.0f);
        ctx.grHist.fill(1); ctx.gateHist.fill(1);
        setTab(0); layoutMatrix();
        startTimerHz(30);
    }
    ~EditorUi() override { stopTimer(); setLookAndFeel(nullptr); }

    void setTab(int t) {
        tab = t;
        for (juce::Component* c : { (juce::Component*) spaceP.get(), (juce::Component*) colorP.get(), (juce::Component*) motionP.get(), (juce::Component*) pumpP.get() }) c->setVisible(t == 0);
        for (int i = 0; i < 6; ++i) { pageBtn[i].setVisible(t == 1); pages[i]->setVisible(t == 1 && i == page); }
        resized();
    }
    void setPage(int p) { page = p; setTab(1); }
    void layoutMatrix() { matrixP->setVisible(ed.matrixShown); showMatrix.setVisible(!ed.matrixShown); }

    void paint(Graphics& g) override {
        g.fillAll(col::bg0);
        juce::ColourGradient hg(Colour(0xff1a2029), 0, 0, Colour(0xff11151b), 0, 78, false); g.setGradientFill(hg); g.fillRect(0, 0, getWidth(), 78);
        g.setColour(col::matrix.withAlpha(.25f)); g.fillRect(0, 78, getWidth(), 1);
        // logo
        Rectangle<float> lg(18, 16, 44, 44);
        g.setColour(Colour(0xff1c222a)); g.fillRoundedRectangle(lg, 9);
        g.setColour(col::matrix); g.drawRoundedRectangle(lg, 9, 1.6f);
        g.setFont(font(20.0f)); g.drawText("V", lg.toNearestInt(), juce::Justification::centred);
        g.setColour(col::text); g.setFont(font(24.0f)); g.drawText("VELVET SPACE", 74, 16, 250, 28, juce::Justification::centredLeft);
        g.setColour(col::lime); g.setFont(font(9.5f)); g.drawText("VELVET AUDIO  /  REVERB LAB  /  v2.0", 76, 42, 250, 14, juce::Justification::centredLeft);
        g.setColour(col::dim); g.setFont(font(9.5f)); g.drawText("PRESETS", 330, 12, 100, 12, juce::Justification::centredLeft);
        if (ctx.p.sidechainActive.load()) { g.setColour(col::pump); g.setFont(font(9.5f)); g.drawText("SIDECHAIN CONNECTED", 1040, 86, 300, 14, juce::Justification::centredRight); }
    }
    void resized() override {
        presetBar.setBounds(330, 26, 690, 32);
        save.setBounds(1028, 26, 104, 32); menu.setBounds(1136, 26, 34, 32);
        visualsBtn.setBounds(1176, 28, 76, 28);
        output->setBounds(1260, 14, 86, 44); output->valueW = 34;
        allTab.setBounds(16, 84, 104, 24); modTab.setBounds(124, 84, 94, 24);
        const int y = 116, h = 470;
        spaceP->setBounds(16, y, 322, h); colorP->setBounds(346, y, 324, h); motionP->setBounds(678, y, 322, h); pumpP->setBounds(1008, y, 336, h);
        for (int i = 0; i < 6; ++i) { pageBtn[i].setBounds(230 + i * 110, 84, 104, 24); pages[i]->setBounds(16, y, 1328, h); }
        matrixP->setBounds(16, 594, 1328, 260);
        showMatrix.setBounds(1220, 594, 124, 26);
    }

    void timerCallback() override {
        auto& p = ctx.p;
        // histories
        ctx.histPos = (ctx.histPos + 1) % Ctx::Hist; size_t hp = (size_t) ((ctx.histPos + Ctx::Hist - 1) % Ctx::Hist);
        ctx.grHist[hp] = p.mDuck.load(); ctx.gateHist[hp] = p.mGate.load(); ctx.envHist[hp] = p.mEnv.load(); ctx.midiHist[hp] = p.mMidiEnv.load();
        for (int l = 0; l < Lfos; ++l) ctx.lfoHist[(size_t) l][hp] = p.mLfo[(size_t) l].load();
        ctx.trail[(size_t) ctx.trailPos] = { p.mOx.load(), p.mOz.load() }; ctx.trailPos = (ctx.trailPos + 1) % (int) ctx.trail.size();
        if (ctx.visuals) computeSpectrum();
        for (auto* l : lives) if (auto* c = dynamic_cast<juce::Component*>(l); c && c->isShowing()) l->updateLive();
        if (tab == 0) { spaceP->tick(); colorP->tick(); motionP->tick(); pumpP->tick(); }
        else pages[page]->tick();
        if (ed.matrixShown) matrixP->tick();
        if (p.presetName != shownPreset || p.presets.currentIndex() != shownIndex) { shownPreset = p.presetName; shownIndex = p.presets.currentIndex(); presetBar.repaint(); }
        bool sc = p.sidechainActive.load(); if (sc != shownSc) { shownSc = sc; repaint(1000, 84, 344, 18); }
    }
    void computeSpectrum() {
        float sr = (float) (ctx.p.getSampleRate() > 0 ? ctx.p.getSampleRate() : 48000.0);
        auto run = [&](const ScopeRing& ring, std::array<float, Ctx::Spec>& out) {
            ring.copyLatest(fftBuf.data(), 2048);
            for (int i = 0; i < 2048; ++i) fftBuf[(size_t) i] *= window[(size_t) i];
            std::fill(fftBuf.begin() + 2048, fftBuf.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform(fftBuf.data());
            for (int k = 0; k < Ctx::Spec; ++k) {
                float f0 = 20 * std::pow(1000.0f, (float) k / Ctx::Spec), f1 = 20 * std::pow(1000.0f, (float) (k + 1) / Ctx::Spec);
                int b0 = juce::jlimit(1, 1023, (int) (f0 / sr * 2048)), b1 = juce::jlimit(b0, 1023, (int) (f1 / sr * 2048));
                float m = 0; for (int b = b0; b <= b1; ++b) m = juce::jmax(m, fftBuf[(size_t) b]);
                float dB = 20 * std::log10(m * 4 / 2048 + 1e-7f) + 3.0f * std::log2(juce::jmax(20.0f, f0) / 1000.0f);
                auto& o = out[(size_t) k];
                o = dB > o ? .5f * o + .5f * dB : .85f * o + .15f * dB;
                if (!std::isfinite(o)) o = -120;
            }
        };
        run(ctx.p.dryScope, ctx.dryDb); run(ctx.p.wetScope, ctx.wetDb);
    }

    void savePreset() {
        auto* w = new juce::AlertWindow("Save preset", "Name your preset. It will appear under User in the preset list.", juce::MessageBoxIconType::NoIcon, this);
        w->setLookAndFeel(&lnf);
        w->addTextEditor("name", ctx.p.presetName == "Clean Canvas" ? juce::String("My preset") : ctx.p.presetName);
        w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey)); w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<EditorUi> safe(this);
        w->enterModalState(true, juce::ModalCallbackFunction::create([safe, w](int r) {
            if (r == 1 && safe != nullptr) { safe->ctx.p.presets.saveUser(w->getTextEditorContents("name")); safe->presetBar.repaint(); }
        }), true);
    }
    void showMenu() {
        juce::PopupMenu m; auto& e = ctx.p.presets.entries(); int cur = ctx.p.presets.currentIndex();
        bool user = cur >= 0 && cur < (int) e.size() && !e[(size_t) cur].factory;
        m.addSectionHeader("PRESETS");
        m.addItem(1, "Delete current user preset", user);
        m.addItem(2, "Import preset...");
        m.addItem(3, "Export current settings...");
        m.addItem(4, "Show preset folder");
        m.addItem(5, "Reset to Clean Canvas");
        m.addSectionHeader("WINDOW SIZE");
        float sc[] { .75f, .9f, 1.0f, 1.15f, 1.3f };
        for (int i = 0; i < 5; ++i) m.addItem(10 + i, juce::String(juce::roundToInt(sc[i] * 100)) + " %", true, std::abs(ed.scale - sc[i]) < .01f);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menu), [this, sc](int r) {
            auto& pm = ctx.p.presets;
            if (r == 1) pm.deleteUser(pm.currentIndex());
            else if (r == 2) {
                chooser = std::make_unique<juce::FileChooser>("Import preset", PresetManager::userFolder(), "*.vspreset");
                chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) ctx.p.presets.importFile(fc.getResult()); });
            } else if (r == 3) {
                chooser = std::make_unique<juce::FileChooser>("Export preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(ctx.p.presetName + ".vspreset"), "*.vspreset");
                chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) ctx.p.presets.exportFile(fc.getResult()); });
            } else if (r == 4) PresetManager::userFolder().startAsProcess();
            else if (r == 5) pm.load(0);
            else if (r >= 10 && r < 15) { ed.scale = sc[r - 10]; ed.applySize(); }
            presetBar.repaint();
        });
    }

    Lnf lnf;
    Ctx ctx;
    VelvetSpaceEditor& ed;
    std::vector<LiveCtl*> lives;
    PresetBar presetBar;
    juce::TextButton save, menu, allTab, modTab, showMatrix;
    VisualsLed visualsBtn;
    std::unique_ptr<HSlider> output;
    std::unique_ptr<SpacePanel> spaceP; std::unique_ptr<ColorPanel> colorP; std::unique_ptr<MotionPanel> motionP; std::unique_ptr<PumpPanel> pumpP;
    std::unique_ptr<MatrixPanel> matrixP;
    juce::TextButton pageBtn[6]; std::unique_ptr<ModulePage> pages[6];
    int tab = 0, page = 0, shownIndex = -1; juce::String shownPreset; bool shownSc = false;
    juce::dsp::FFT fft; std::array<float, 4096> fftBuf {}; std::array<float, 2048> window {};
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TooltipWindow tips { this, 600 };
};

// ================================================================ editor
VelvetSpaceEditor::VelvetSpaceEditor(VelvetSpaceProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    ui = std::make_unique<EditorUi>(p, *this);
    addAndMakeVisible(*ui);
    applySize();
}
VelvetSpaceEditor::~VelvetSpaceEditor() { ui = nullptr; }
void VelvetSpaceEditor::applySize() {
    int h = matrixShown ? BaseH : CollapsedH;
    setSize(juce::roundToInt(BaseW * scale), juce::roundToInt(h * scale));
}
void VelvetSpaceEditor::paint(Graphics& g) { g.fillAll(col::bg0); }
void VelvetSpaceEditor::resized() {
    if (!ui) return;
    ui->setBounds(0, 0, BaseW, BaseH);
    ui->setTransform(juce::AffineTransform::scale(scale));
}
