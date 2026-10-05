#pragma once
#include <JuceHeader.h>
#include "PocketDSP.h"

struct PocketTrace {
    float keyLo=0, keyHi=0, outLo=0, outHi=0, gain=1;
    double time=0;
    std::uint32_t generation=0;
};

class DuckPocketAudioProcessor final : public juce::AudioProcessor,
                                      private juce::AudioProcessorValueTreeState::Listener,
                                      private juce::AsyncUpdater {
public:
    DuckPocketAudioProcessor();
    ~DuckPocketAudioProcessor() override;
    int getLookaheadMs() const noexcept;
    void selectLookahead(int index);
    void playDuck() noexcept { duckRequested.store(true,std::memory_order_release); }
    bool usesExtendedAttack() const noexcept {return extendedAttack->load()>.5f;}
    juce::RangedAudioParameter& attackParameter(){return *parameters.getParameter(usesExtendedAttack()?"attackMs":"attack");}
    bool popTrace(PocketTrace&);

    std::atomic<bool> displayBypass{false}, editorOpen{false};
    std::atomic<int> editorWidth{0};
    std::atomic<std::uint32_t> traceGeneration{0};

    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    using juce::AudioProcessor::processBlock;
    using juce::AudioProcessor::processBlockBypassed;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processAudio(b,m,true); }
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return parameters.getParameter("bypass"); }
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return getLatencySamples()/juce::jmax(1.,getSampleRate()); }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();

    juce::AudioProcessorValueTreeState parameters;

private:
    void prepareDuck(double sampleRate);
    void mixDuck(juce::AudioBuffer<float>&);
    juce::AudioBuffer<float> duckAudio;
    std::atomic<bool> duckRequested{false};
    int duckPosition=0;
    pocket::Engine engine;
    std::atomic<float>* lookaheadChoice=nullptr,*attackMs=nullptr,*extendedAttack=nullptr;
    std::atomic<double> preparedRate{48000.};
    void parameterChanged(const juce::String&,float) override;
    void handleAsyncUpdate() override;
    std::atomic<float>* amount=nullptr,*duration=nullptr,*low=nullptr,*high=nullptr,*bypass=nullptr,*balance=nullptr,*processLow=nullptr,*processHigh=nullptr,*outputGain=nullptr,*durationPercent=nullptr,*relativeDuration=nullptr,*mix=nullptr,*attack=nullptr,*legacyAttack=nullptr;
    void processAudio(juce::AudioBuffer<float>&,juce::MidiBuffer&,bool);

    // 2400 trace packets/s gives 240 samples in the shortest (100 ms) graph.
    // The large SPSC queue tolerates UI stalls without touching the audio thread.
    juce::AbstractFifo fifo{8192};
    std::array<PocketTrace,8192> traces{};
    PocketTrace capture;
    double traceTime=0;
    int captured=0,decimation=20;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuckPocketAudioProcessor)
};
