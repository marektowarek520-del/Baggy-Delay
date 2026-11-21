#include "PluginProcessor.h"
#include "PluginEditor.h"

BaggyDelayProcessor::BaggyDelayProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BaggyDelayProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<P> (juce::ParameterID { "time", 1 }, "Time",
           juce::NormalisableRange<float> (1.0f, 2000.0f, 0.01f, 0.4f), 350.0f,
           juce::AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<P> (juce::ParameterID { "feedback", 1 }, "Feedback",
           juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.4f));
    l.add (std::make_unique<P> (juce::ParameterID { "tone", 1 }, "Tone",
           juce::NormalisableRange<float> (500.0f, 16000.0f, 1.0f, 0.4f), 6000.0f,
           juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<P> (juce::ParameterID { "width", 1 }, "Width",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    l.add (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix",
           juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    return l;
}

void BaggyDelayProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    bufferSize = (int) (sr * 2.1) + 4;
    for (auto& b : buffer) b.assign ((size_t) bufferSize, 0.0f);
    writePos = 0;
    lpState[0] = lpState[1] = 0.0f;

    auto init = [this] (juce::SmoothedValue<float>& s, const char* id)
    { s.reset (sr, 0.05); s.setCurrentAndTargetValue (*apvts.getRawParameterValue (id)); };
    init (smTime, "time"); init (smFeedback, "feedback"); init (smTone, "tone");
    init (smWidth, "width"); init (smMix, "mix");
}

bool BaggyDelayProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void BaggyDelayProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = audio.getNumSamples();
    if (bufferSize == 0 || audio.getNumChannels() < 2) return;

    smTime.setTargetValue     (*apvts.getRawParameterValue ("time"));
    smFeedback.setTargetValue (*apvts.getRawParameterValue ("feedback"));
    smTone.setTargetValue     (*apvts.getRawParameterValue ("tone"));
    smWidth.setTargetValue    (*apvts.getRawParameterValue ("width"));
    smMix.setTargetValue      (*apvts.getRawParameterValue ("mix"));

    auto* L = audio.getWritePointer (0);
    auto* R = audio.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float timeMs = smTime.getNextValue();
        const float fb     = smFeedback.getNextValue();
        const float cutoff = smTone.getNextValue();
        const float width  = smWidth.getNextValue();
        const float mix    = smMix.getNextValue();

        const float delaySamples = juce::jlimit (1.0f, (float) (bufferSize - 3), timeMs * 0.001f * (float) sr);
        const float coef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sr);

        float wet[2];
        for (int ch = 0; ch < 2; ++ch)
        {
            float rp = (float) writePos - delaySamples;
            while (rp < 0.0f) rp += (float) bufferSize;
            const int i0 = (int) rp;
            const int i1 = (i0 + 1) % bufferSize;
            const float frac = rp - (float) i0;
            wet[ch] = buffer[ch][(size_t) i0] * (1.0f - frac) + buffer[ch][(size_t) i1] * frac;
        }

        const float dryL = L[i], dryR = R[i];
        const float fbL = juce::jmap (width, wet[0], wet[1]);   // width = ping-pong
        const float fbR = juce::jmap (width, wet[1], wet[0]);

        lpState[0] += coef * (fbL - lpState[0]);
        lpState[1] += coef * (fbR - lpState[1]);

        buffer[0][(size_t) writePos] = juce::jlimit (-4.0f, 4.0f, dryL + fb * lpState[0]);
        buffer[1][(size_t) writePos] = juce::jlimit (-4.0f, 4.0f, dryR + fb * lpState[1]);
        writePos = (writePos + 1) % bufferSize;

        L[i] = dryL * (1.0f - mix) + wet[0] * mix;
        R[i] = dryR * (1.0f - mix) + wet[1] * mix;
    }
}

juce::AudioProcessorEditor* BaggyDelayProcessor::createEditor() { return new BaggyDelayEditor (*this); }

void BaggyDelayProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void BaggyDelayProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BaggyDelayProcessor(); }
