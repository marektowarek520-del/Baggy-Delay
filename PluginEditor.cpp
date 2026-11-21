#include "PluginEditor.h"

using juce::Colour;
using juce::Colours;

static const Colour kNeon   (0xFF39FF14);
static const Colour kRed    (0xFFFF1A1A);
static const Colour kBgTop  (0xFF14001F);
static const Colour kBgBot  (0xFF050008);

//==============================================================================
void SwagLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const Colour accent = kNeon.interpolatedWith (kRed, intensity);

    // glow
    g.setColour (accent.withAlpha (0.10f + 0.15f * intensity));
    g.fillEllipse (bounds.expanded (6.0f));

    // body
    g.setGradientFill (juce::ColourGradient (Colour (0xFF2B2B2B), centre.x, centre.y - radius,
                                             Colour (0xFF050505), centre.x, centre.y + radius, false));
    g.fillEllipse (centre.x - radius * 0.78f, centre.y - radius * 0.78f, radius * 1.56f, radius * 1.56f);
    g.setColour (Colours::black);
    g.drawEllipse (centre.x - radius * 0.78f, centre.y - radius * 0.78f, radius * 1.56f, radius * 1.56f, 3.0f);

    // track + value arc
    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (Colour (0xFF2A2A2A));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer
    juce::Path p;
    p.addRoundedRectangle (-2.5f, -radius * 0.74f, 5.0f, radius * 0.38f, 2.0f);
    g.setColour (Colours::white);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
BaggyDelayEditor::BaggyDelayEditor (BaggyDelayProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (700, 480);
    for (int i = 0; i < 5; ++i)
    {
        auto& k = knobs[i];
        k.slider.setLookAndFeel (&lnf);
        k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        k.label.setColour (juce::Label::textColourId, Colours::white);
        addAndMakeVisible (k.label);

        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);
    }
    startTimerHz (30);
}

BaggyDelayEditor::~BaggyDelayEditor()
{
    for (auto& k : knobs) k.slider.setLookAndFeel (nullptr);
}

void BaggyDelayEditor::resized()
{
    auto area = getLocalBounds().reduced (24).removeFromBottom (130);
    const int w = area.getWidth() / 5;
    for (int i = 0; i < 5; ++i)
    {
        auto cell = area.removeFromLeft (w);
        knobs[i].label.setBounds (cell.removeFromBottom (22));
        knobs[i].slider.setBounds (cell);
    }
}

// średnia ze wszystkich gałek (0..1) -> jak bardzo "najarany" jest stickman
float BaggyDelayEditor::computeTargetIntensity() const
{
    float sum = 0.0f;
    for (int i = 0; i < 5; ++i)
        sum += proc.apvts.getParameter (ids[i])->getValue();
    return juce::jlimit (0.0f, 1.0f, sum / 5.0f * 1.25f);
}

void BaggyDelayEditor::timerCallback()
{
    t += 1.0f / 30.0f;
    intensity += 0.12f * (computeTargetIntensity() - intensity);
    lnf.intensity = intensity;

    // mrugnięcie (wolniejsze i cięższe przy większej intensywności)
    blink = juce::jmax (0.0f, blink - 0.12f);
    if (rng.nextFloat() < 0.015f + 0.01f * intensity) blink = 1.0f;

    // dym
    if (intensity > 0.2f && rng.nextFloat() < 0.25f + 0.6f * intensity)
        smoke.push_back ({ 0.0f, 0.0f, (rng.nextFloat() - 0.3f) * 0.8f, -(0.8f + rng.nextFloat()),
                           1.0f, 5.0f + rng.nextFloat() * 6.0f });
    for (auto& s : smoke)
    {
        s.x += s.vx + std::sin (t * 3.0f + s.y * 0.05f) * 0.4f;
        s.y += s.vy;
        s.size += 0.35f;
        s.life -= 0.012f;
    }
    smoke.erase (std::remove_if (smoke.begin(), smoke.end(), [] (const Smoke& s) { return s.life <= 0.0f; }), smoke.end());

    repaint();
}

//==============================================================================
void BaggyDelayEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (kBgTop.interpolatedWith (Colour (0xFF2A0000), intensity * 0.6f), 0, 0,
                                             kBgBot, 0, (float) getHeight(), false));
    g.fillAll();

    // ziarno / grunge
    juce::Random grain ((int) (t * 12.0f));
    g.setColour (Colours::white.withAlpha (0.035f));
    for (int i = 0; i < 700; ++i)
        g.fillRect (grain.nextFloat() * (float) getWidth(), grain.nextFloat() * (float) getHeight(), 1.5f, 1.5f);

    // tytuł
    g.setColour (kNeon.interpolatedWith (kRed, intensity));
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("BAGGY DELAY", 24, 14, 300, 36, juce::Justification::left);
    g.setColour (Colours::white.withAlpha (0.45f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("ultra wide jeans edition", 26, 46, 300, 16, juce::Justification::left);

    // pasek "stoned level"
    auto bar = juce::Rectangle<float> ((float) getWidth() - 190.0f, 24.0f, 160.0f, 10.0f);
    g.setColour (Colour (0xFF222222));
    g.fillRoundedRectangle (bar, 5.0f);
    g.setColour (kNeon.interpolatedWith (kRed, intensity));
    g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * intensity), 5.0f);
    g.setColour (Colours::white.withAlpha (0.6f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("VIBE LEVEL", bar.translated (0.0f, 12.0f).toNearestInt(), juce::Justification::left);

    drawStickman (g, juce::Rectangle<float> (0.0f, 50.0f, (float) getWidth(), (float) getHeight() - 190.0f));
}

void BaggyDelayEditor::drawStickman (juce::Graphics& g, juce::Rectangle<float> a)
{
    const float cx = a.getCentreX();
    const float groundY = a.getBottom() - 6.0f;
    const float bob = std::sin (t * (3.0f + 3.0f * intensity)) * (2.0f + 4.0f * intensity);
    const float sway = std::sin (t * (1.5f + 1.5f * intensity)) * (4.0f + 8.0f * intensity);

    const Colour outline = Colours::black;
    const Colour denim   (0xFF2F5BA8);
    const Colour denimDk (0xFF1E3C73);
    const Colour skin    (0xFFF5F5F5);

    // cień na ziemi
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (cx - 130.0f, groundY - 6.0f, 260.0f, 14.0f);

    const float headR   = 34.0f;
    const float headY   = a.getY() + 52.0f + bob;
    const float neckY   = headY + headR;
    const float torsoT  = neckY + 4.0f;
    const float torsoB  = torsoT + 78.0f;
    const float hipY    = torsoB - 6.0f;

    // ---- ultra szerokie jeansy ----
    for (int side = -1; side <= 1; side += 2)
    {
        const float s = (float) side;
        juce::Path leg;
        leg.startNewSubPath (cx + s * 4.0f,   hipY);
        leg.lineTo (cx + s * 40.0f,  hipY);
        leg.lineTo (cx + s * (118.0f + sway * 0.3f), groundY - 14.0f);
        leg.lineTo (cx + s * (112.0f + sway * 0.3f), groundY - 2.0f);
        leg.lineTo (cx + s * 12.0f,  groundY - 2.0f);
        leg.lineTo (cx + s * 8.0f,   groundY - 14.0f);
        leg.closeSubPath();
        g.setColour (denim);
        g.fillPath (leg);
        g.setColour (outline);
        g.strokePath (leg, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // fałdy materiału
        g.setColour (denimDk);
        for (int f = 0; f < 4; ++f)
        {
            const float fy = hipY + 45.0f + (float) f * 14.0f;
            const float x0 = cx + s * (20.0f + (float) f * 16.0f);
            g.drawLine (x0, fy, x0 + s * 18.0f, fy + 22.0f, 2.0f);
        }
        // sneakersy wystające spod nogawek
        g.setColour (Colours::white);
        g.fillRoundedRectangle (cx + s * 62.0f - 28.0f, groundY - 8.0f, 56.0f, 12.0f, 6.0f);
        g.setColour (outline);
        g.drawRoundedRectangle (cx + s * 62.0f - 28.0f, groundY - 8.0f, 56.0f, 12.0f, 6.0f, 2.5f);
    }

    // ---- pasek ----
    g.setColour (Colour (0xFF111111));
    g.fillRect (cx - 42.0f, hipY - 4.0f, 84.0f, 8.0f);
    g.setColour (Colour (0xFFFFD700));
    g.fillRect (cx - 6.0f, hipY - 4.0f, 12.0f, 8.0f);

    // ---- ręce (za bluzą) ----
    const float handSwing = std::sin (t * (2.0f + 2.0f * intensity)) * (10.0f + 15.0f * intensity);
    g.setColour (outline);
    g.drawLine (cx - 44.0f, torsoT + 16.0f, cx - 78.0f, torsoT + 62.0f + handSwing, 6.0f);
    g.drawLine (cx + 44.0f, torsoT + 16.0f, cx + 78.0f, torsoT + 62.0f - handSwing, 6.0f);
    g.setColour (skin);
    g.fillEllipse (cx - 88.0f, torsoT + 56.0f + handSwing, 18.0f, 18.0f);
    g.fillEllipse (cx + 70.0f, torsoT + 56.0f - handSwing, 18.0f, 18.0f);
    g.setColour (outline);
    g.drawEllipse (cx - 88.0f, torsoT + 56.0f + handSwing, 18.0f, 18.0f, 2.5f);
    g.drawEllipse (cx + 70.0f, torsoT + 56.0f - handSwing, 18.0f, 18.0f, 2.5f);

    // ---- oversize bluza ----
    juce::Path hoodie;
    hoodie.addRoundedRectangle (cx - 52.0f, torsoT, 104.0f, torsoB - torsoT, 18.0f);
    g.setColour (Colour (0xFF1A1A1A));
    g.fillPath (hoodie);
    g.setColour (outline);
    g.strokePath (hoodie, juce::PathStrokeType (3.0f));
    g.setColour (Colour (0xFF39FF14).interpolatedWith (kRed, intensity).withAlpha (0.9f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("SWAG", juce::Rectangle<float> (cx - 40.0f, torsoT + 36.0f, 80.0f, 18.0f).toNearestInt(), juce::Justification::centred);

    // ---- łańcuch ----
    juce::Path chain;
    chain.startNewSubPath (cx - 20.0f, neckY);
    chain.quadraticTo (cx, neckY + 40.0f, cx + 20.0f, neckY);
    g.setColour (Colour (0xFFFFD700));
    g.strokePath (chain, juce::PathStrokeType (2.5f));
    g.fillEllipse (cx - 8.0f, neckY + 22.0f + bob * 0.2f, 16.0f, 16.0f);
    g.setColour (Colours::black);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("$", juce::Rectangle<float> (cx - 8.0f, neckY + 22.0f + bob * 0.2f, 16.0f, 16.0f).toNearestInt(), juce::Justification::centred);

    // ---- głowa ----
    g.setColour (skin);
    g.fillEllipse (cx - headR, headY - headR, headR * 2.0f, headR * 2.0f);
    g.setColour (outline);
    g.drawEllipse (cx - headR, headY - headR, headR * 2.0f, headR * 2.0f, 3.5f);

    // czerwona poświata wokół oczu
    if (intensity > 0.05f)
    {
        juce::ColourGradient glow (kRed.withAlpha (0.45f * intensity), cx, headY - 2.0f,
                                   kRed.withAlpha (0.0f), cx, headY - 2.0f + 70.0f, true);
        g.setGradientFill (glow);
        g.fillEllipse (cx - 70.0f, headY - 60.0f, 140.0f, 120.0f);
    }

    // ---- oczy ----
    const Colour sclera = Colours::white.interpolatedWith (kRed, intensity);
    const float eyeW = 20.0f, eyeH = 15.0f;
    const float lid = juce::jlimit (0.0f, 1.0f, 0.15f + 0.55f * intensity + blink);   // przymknięte powieki
    for (int side = -1; side <= 1; side += 2)
    {
        const float ex = cx + (float) side * 13.0f;
        const float ey = headY - 4.0f;
        const juce::Rectangle<float> eye (ex - eyeW * 0.5f, ey - eyeH * 0.5f, eyeW, eyeH);

        g.setColour (sclera);
        g.fillEllipse (eye);

        // żyłki
        if (intensity > 0.25f)
        {
            g.setColour (Colour (0xFF8B0000).withAlpha (juce::jmin (1.0f, intensity)));
            for (int v = 0; v < 5; ++v)
            {
                const float ang = (float) v * 1.25f + 0.4f;
                g.drawLine (ex + std::cos (ang) * eyeW * 0.5f, ey + std::sin (ang) * eyeH * 0.5f,
                            ex + std::cos (ang) * 3.0f,        ey + std::sin (ang) * 2.0f, 0.9f);
            }
        }

        // źrenica
        g.setColour (Colours::black);
        const float pr = 4.0f - 1.2f * intensity;
        g.fillEllipse (ex - pr + std::sin (t * 0.7f) * 1.5f, ey - pr + 1.0f, pr * 2.0f, pr * 2.0f);

        // powieka (zasłania górę oka)
        g.saveState();
        g.reduceClipRegion (eye.expanded (2.0f).toNearestInt());
        g.setColour (skin);
        g.fillRect (eye.getX() - 2.0f, eye.getY() - 3.0f, eyeW + 4.0f, 3.0f + eyeH * lid);
        g.setColour (outline);
        g.drawLine (eye.getX() - 1.0f, eye.getY() + eyeH * lid, eye.getRight() + 1.0f, eye.getY() + eyeH * lid, 2.0f);
        g.restoreState();
        g.setColour (outline);
        g.drawEllipse (eye, 1.5f);
    }

    // brwi
    g.setColour (outline);
    g.drawLine (cx - 22.0f, headY - 14.0f, cx - 6.0f, headY - 12.0f + 2.0f * intensity, 2.5f);
    g.drawLine (cx + 6.0f,  headY - 12.0f + 2.0f * intensity, cx + 22.0f, headY - 14.0f, 2.5f);

    // uśmieszek + blunt
    juce::Path mouth;
    mouth.startNewSubPath (cx - 10.0f, headY + 18.0f);
    mouth.quadraticTo (cx + 2.0f, headY + 24.0f + 3.0f * intensity, cx + 14.0f, headY + 16.0f);
    g.strokePath (mouth, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float jx0 = cx + 8.0f, jy0 = headY + 18.0f;
    const float jx1 = cx + 40.0f, jy1 = headY + 12.0f;
    g.setColour (Colour (0xFFEADFB4));
    g.drawLine (jx0, jy0, jx1, jy1, 4.0f);
    g.setColour (Colour (0xFFFF8C1A).interpolatedWith (Colours::yellow, std::sin (t * 12.0f) * 0.5f + 0.5f));
    g.fillEllipse (jx1 - 2.5f, jy1 - 3.0f, 6.0f, 6.0f);

    // czapka / beanie
    juce::Path cap;
    cap.addCentredArc (cx, headY - 2.0f, headR + 1.5f, headR + 1.5f, 0.0f,
                       juce::degreesToRadians (-100.0f), juce::degreesToRadians (100.0f), true);
    cap.closeSubPath();
    juce::Path capClip;
    capClip.addRectangle (cx - headR - 6.0f, headY - headR - 8.0f, headR * 2.0f + 12.0f, headR * 0.78f + 8.0f);
    g.saveState();
    g.reduceClipRegion (juce::Rectangle<float> (cx - headR - 6.0f, headY - headR - 8.0f,
                                                headR * 2.0f + 12.0f, headR * 0.62f + 8.0f).toNearestInt());
    g.setColour (Colour (0xFF0D0D0D));
    g.fillEllipse (cx - headR - 1.5f, headY - headR - 1.5f, (headR + 1.5f) * 2.0f, (headR + 1.5f) * 2.0f);
    g.restoreState();

    // ---- dym ----
    const float sx = jx1 + 3.0f, sy = jy1;
    for (auto& s : smoke)
    {
        g.setColour (Colours::white.withAlpha (juce::jlimit (0.0f, 0.35f, s.life * 0.35f)));
        g.fillEllipse (sx + s.x - s.size * 0.5f, sy + s.y - s.size * 0.5f, s.size, s.size);
    }
}
