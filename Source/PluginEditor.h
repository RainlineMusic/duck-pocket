#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UIStyle.h"
#include "GlowRenderer.h"
#include "SoftwareGlow.h"


class PocketLook final:public juce::LookAndFeel_V4 {
public:
    PocketTheme theme=PocketTheme::SolidDark;
    PocketTokens tokens() const{return PocketTokens::forTheme(theme); }
    bool isDark() const{return theme!=PocketTheme::SolidWhite;}
    bool isNeon() const{return theme==PocketTheme::Neon;}
    bool isAmber() const{return theme==PocketTheme::Amber;}
    bool hasGlow() const{return isNeon()||isAmber();}
    juce::Colour pick(juce::uint32 neon,juce::uint32 dark,juce::uint32 white) const;
    juce::Colour ink() const;
    juce::Colour muted() const;
    juce::Colour accent() const;
    juce::Colour accent2() const;
    juce::Colour themedAccent(juce::uint32 neon) const;
    juce::Font getTextButtonFont(juce::TextButton&,int) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
};

class ResettableRangeSlider final:public juce::Slider {
public:
    // Separate per-handle callbacks: double-click (or alt-click) only resets
    // whichever thumb is nearer the click, not both ends of the range at once.
    std::function<void()> onResetMin,onResetMax;
    void mouseDoubleClick(const juce::MouseEvent& e) override {resetNearestThumb(e);}
    // Alt-click resets too (JUCE's built-in alt-click only handles a single value).
    void mouseDown(const juce::MouseEvent& e) override {
        altReset=!e.mods.isPopupMenu()&&e.mods.withoutMouseButtons()==juce::ModifierKeys(juce::ModifierKeys::altModifier);
        if(altReset){resetNearestThumb(e);return;}
        juce::Slider::mouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {if(!altReset)juce::Slider::mouseDrag(e);}
    void mouseUp(const juce::MouseEvent& e) override {if(altReset){altReset=false;return;}juce::Slider::mouseUp(e);}
    double valueToProportionOfLength(double value) override {return std::log(juce::jlimit(20.,20000.,value)/20.)/std::log(1000.);}
    double proportionOfLengthToValue(double proportion) override {return 20.*std::pow(1000.,juce::jlimit(0.,1.,proportion));}
private:
    bool altReset=false;
    // Compares the click x to each thumb's own pixel position (via this
    // slider's own value->proportion mapping, same one the LookAndFeel uses
    // to place the thumbs) and fires only the callback for the nearer one.
    void resetNearestThumb(const juce::MouseEvent& e){
        const float w=float(getWidth());
        if(w<=0.f)return;
        const float minX=float(valueToProportionOfLength(getMinValue()))*w;
        const float maxX=float(valueToProportionOfLength(getMaxValue()))*w;
        const bool nearMin=std::abs(e.position.x-minX)<=std::abs(e.position.x-maxX);
        if(nearMin){if(onResetMin)onResetMin();}else{if(onResetMax)onResetMax();}
    }
};

class ModernDial final:public juce::Slider,private juce::Timer {
public:
    // trailing infLabel overrides the "infinity" display text at full deflection
    // (e.g. "AUTO"); empty means keep the default infinity glyph.
    ModernDial(PocketLook&,juce::String,juce::String,juce::String,juce::uint32,bool=false,bool=false,bool=false,juce::String={});
    ~ModernDial() override {stopTimer();}
    bool isAutoValue() const {return infinity&&getValue()>=getMaximum();}
    float valueTextHeight(const juce::String& value) const {
        const float preferred=compact?17.f:(title=="Influence"?29.f:25.f);
        if(!compact)return preferred;
        const float diameter=float(juce::jmin(getWidth(),getHeight()))*.66f;
        const float width=juce::GlyphArrangement::getStringWidth(pocketFont(preferred,true),value);
        return juce::jlimit(11.f,preferred,preferred*diameter/juce::jmax(1.f,width+1.f));
    }
    void mouseEnter(const juce::MouseEvent& e) override {juce::Slider::mouseEnter(e);animate(.65f);}
    void mouseExit(const juce::MouseEvent& e) override {juce::Slider::mouseExit(e);animate(0);}
    void mouseDown(const juce::MouseEvent& e) override {juce::Slider::mouseDown(e);animate(1);}
    void mouseUp(const juce::MouseEvent& e) override {juce::Slider::mouseUp(e);animate(isMouseOver()?.65f:0);}
    void paint(juce::Graphics&) override;
    void setMeter(float reduction,float signal){if(std::abs(gr-reduction)>.001f||std::abs(activity-signal)>.005f){gr=reduction;activity=signal;repaint();}}
    void setDurationMode(bool relative){unit=relative?"%":"ms";subtitle=relative?"Key length":"Legacy length";repaint();}
private:
    PocketLook& look;
    juce::String title,subtitle,unit;
    bool infinity,compact;
    juce::Image body;
    PocketTheme bodyTheme=PocketTheme::Neon;
    float bodyScale=0,gr=0,activity=0,emphasis=0,targetEmphasis=0;
    void animate(float target){targetEmphasis=target;startTimerHz(60);}
    void timerCallback() override {emphasis+=(targetEmphasis-emphasis)*.3f;if(std::abs(targetEmphasis-emphasis)<.01f){emphasis=targetEmphasis;stopTimer();}repaint();}
};

class DuckPocketAudioProcessorEditor final:public juce::AudioProcessorEditor {
public:
    explicit DuckPocketAudioProcessorEditor(DuckPocketAudioProcessor&);
    ~DuckPocketAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    friend struct DuckUiTestAccess;
#if DUCK_ENABLE_OPENGL
    std::unique_ptr<PocketGlowRenderer> glowRenderer;
    std::shared_ptr<const PocketGlowRenderer::Frame> lastGpuFrame;
    double glAttachTime=0;
    bool glWasReady=false;
    void setOpenGL(bool enabled,bool persist=true);
#endif
    juce::Graphics* emissionGraphics=nullptr;
    std::array<PocketSoftwareGlow,2> softwarePlots;
    double triggerStamp=-1;
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    DuckPocketAudioProcessor& audioProcessor;
    PocketLook look;
    juce::TooltipWindow tooltips{this,700};
    ModernDial influence{look,"Influence","Depth","%",0xff5987ff};
    ModernDial duration{look,"Duration","Sidechain length","ms",0xff32d4cb,true,false,false,"AUTO"};
    ModernDial outputGain{look,"Output","dB","dB",0xfff1e84b,false,false,true};
    ResettableRangeSlider sidechainRange,processingRange;
    juce::Slider midSide;
    juce::TextButton settingsButton{"settings"},bypassButton{"power"},freezeButton{"freeze"};
    std::unique_ptr<SliderAttachment> influenceAttach,durationAttach,outputAttach,msAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
    std::unique_ptr<juce::ParameterAttachment> lowAttach,highAttach,processLowAttach,processHighAttach;
    std::unique_ptr<juce::PropertiesFile> preferences;
    std::unique_ptr<juce::VBlankAttachment> vblank;

    static constexpr int historyCapacity=16384;
    static constexpr int summaryCapacity=4096;
    std::array<PocketTrace,historyCapacity> history{};
    std::array<PocketTrace,summaryCapacity> summaryHistory{};
    int cursor=0,filled=0;
    int summaryCursor=0,summaryFilled=0;
    long long summaryBin=-1;
    std::vector<PocketTrace> frozenGain,frozenSummary;
    bool gainFrozen=false,scopeFrozen=false;
    double gainResume=0,scopeResume=0;
    bool ready=false,rangeGesture=false,processRangeGesture=false,capturingBlur=false,bypassTarget=false;
    double resizeStamp=0,nextFrameMs=0; // nextFrameMs = time of the last rendered frame
    double displayTime=0,lastClock=0,lastLatest=0,gapMax=0,lastPaintedTime=-1;
    float bypassMix=0;
    double gainWindow=1.,scopeWindow=1.;
    std::uint32_t traceGeneration=0;
    bool durationIsRelative=false;
    void syncDurationMode();
    juce::Image blurredSnapshot,chrome;
    bool chromeValid=false;
    double lastVisibleSignalTime=-1;
    float signalPeak=0,currentReduction=0;
    std::uint64_t paintCount=0,chromeBuildCount=0;
    float chromeScale=1.f;
    juce::Rectangle<int> blurArea,gainArea,scopeArea;

    // Bounded, reused bucket and point storage for the graph construction.
    std::vector<float> bucketLo,bucketHi,bucketScratch;
    std::vector<juce::Point<float>> pathPoints,pathTop,pathBottom;

    void frameTick();
    void syncRange();
    void syncProcessingRange();
    void saveSize();
    void setTheme(PocketTheme,bool persist=true);
    void showSettingsMenu();
    void setHistoryWindow(double seconds);
    void captureBlurSnapshot();
    void setFrozen(bool frozen);
    void invalidateChrome();
    void paintChrome(juce::Graphics&);
    void paintDynamicLabels(juce::Graphics&);
    void panel(juce::Graphics&,juce::Rectangle<float>);
    void graph(juce::Graphics&,juce::Rectangle<float>,bool);
    juce::Rectangle<int> scaled(float,float,float,float) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuckPocketAudioProcessorEditor)
};
