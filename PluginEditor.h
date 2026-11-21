#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Look & feel: ciemne gałki z neonowym łukiem, który czerwienieje razem ze stickmanem
class SwagLookAndFeel : public juce::LookAndFeel_V4
{
public:
    float intensity = 0.0f;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

struct Smoke { float x, y, vx, vy, life, size; };

class BaggyDelayEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BaggyDelayEditor (BaggyDelayProcessor&);
    ~BaggyDelayEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawStickman (juce::Graphics&, juce::Rectangle<float> area);
    float computeTargetIntensity() const;

    BaggyDelayProcessor& proc;
    SwagLookAndFeel lnf;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    Knob knobs[5];
    const char* ids[5]    = { "time", "feedback", "tone", "width", "mix" };
    const char* names[5]  = { "TIME", "FEEDBACK", "TONE", "WIDTH", "MIX" };

    float intensity = 0.0f;
    float t = 0.0f;
    float blink = 0.0f;
    std::vector<Smoke> smoke;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaggyDelayEditor)
};
