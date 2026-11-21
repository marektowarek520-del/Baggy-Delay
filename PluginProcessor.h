#pragma once
#include <JuceHeader.h>
#include <vector>

class BaggyDelayProcessor : public juce::AudioProcessor
{
public:
    BaggyDelayProcessor();
    ~BaggyDelayProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Baggy Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    std::vector<float> buffer[2];
    int bufferSize = 0;
    int writePos = 0;
    double sr = 44100.0;
    float lpState[2] { 0.f, 0.f };
    juce::SmoothedValue<float> smTime, smMix, smFeedback, smTone, smWidth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaggyDelayProcessor)
};
