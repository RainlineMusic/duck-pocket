#include "PluginEditor.h"
namespace {
juce::Font uiFont(float size){return pocketFont(size);}
// Written as raw UTF-8 bytes on purpose: a \u escape in a narrow literal gets
// transcoded to the compiler's execution charset (MSVC without /utf-8 turns it
// into '?'), which is exactly how the dials lost their infinity sign.
constexpr const char* kInfinity="\xe2\x88\x9e";
constexpr const char* kMinusInfinity="-\xe2\x88\x9e";
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size,juce::Colour c,int align=juce::Justification::centredLeft,float glow=0.f){
    g.setFont(uiFont(juce::jmax(13.2f,size)));
    if(glow>0.f){g.setColour(c.withAlpha(glow));const float o[8][2]={{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,1},{-1,1},{1,-1}};for(auto& d:o)g.drawText(s,r.translated(d[0],d[1]),align);}
    g.setColour(c);g.drawText(s,r,align);
}
void stroke(juce::Graphics& g,const juce::Path& p,juce::Colour c,float width){g.setColour(c);g.strokePath(p,juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
juce::String hz(double v){return v>=1000?juce::String(v/1000,1)+" kHz":juce::String(juce::roundToInt(v))+" Hz";}
juce::String timeLabel(double seconds){const int ms=juce::roundToInt(seconds*1000);return ms<1000?juce::String(ms)+" ms":juce::String(seconds,seconds==std::floor(seconds)?0:2)+" s";}
constexpr std::array<double,6> windows{{.1,.25,.5,1.,2.,5.}};
juce::Path smoothPath(const std::vector<juce::Point<float>>& points){
    juce::Path p;if(points.empty())return p;p.startNewSubPath(points.front());
    for(size_t i=1;i+1<points.size();++i){auto mid=(points[i]+points[i+1])*.5f;p.quadraticTo(points[i],mid);}
    if(points.size()>1){p.lineTo(points.back());}
    return p;
}
}
juce::Colour PocketLook::pick(juce::uint32 neon,juce::uint32 dark,juce::uint32 white) const {
    if(theme==PocketTheme::Neon)return juce::Colour(neon);
    if(theme==PocketTheme::SolidDark)return juce::Colour(dark);
    if(theme==PocketTheme::SolidWhite)return juce::Colour(white);
    // Amber: preserve the semantic brightness/alpha of the dark palette while
    // moving it onto the warm brown/orange ramp from the reference UI.
    const auto source=juce::Colour(dark?dark:neon);
    const float b=source.getPerceivedBrightness();
    const float sat=juce::jmap(b,0.f,1.f,.72f,.20f);
    const float value=juce::jlimit(.025f,1.f,b*.93f+.018f);
    return juce::Colour::fromHSV(.078f,sat,value,source.getFloatAlpha());
}
juce::Colour PocketLook::ink() const{return tokens().ink;}
juce::Colour PocketLook::muted() const{return tokens().muted;}
juce::Colour PocketLook::accent() const{return theme==PocketTheme::Amber?juce::Colour(0xffff7126):pick(0xff35d6dc,0xffe8e8e8,0xff2f74d0);}
juce::Colour PocketLook::accent2() const{return theme==PocketTheme::Amber?juce::Colour(0xffffd164):(isNeon()?juce::Colour(0xff35d6dc):accent());}
juce::Colour PocketLook::themedAccent(juce::uint32 neon) const {return isAmber()?(juce::Colour(neon).getHue()>.3f?juce::Colour(0xffffd164):juce::Colour(0xffff7126)):juce::Colour(neon);}
juce::Font PocketLook::getTextButtonFont(juce::TextButton&,int){return uiFont(15);}
void PocketLook::drawButtonBackground(juce::Graphics& g,juce::Button&,const juce::Colour&,bool hover,bool down){auto r=g.getClipBounds().toFloat().reduced(2);if(hasGlow()){g.setColour(juce::Colours::black.withAlpha(.35f));g.fillRoundedRectangle(r.translated(0,2),9);g.setGradientFill(juce::ColourGradient(pick(hover?0xff26354c:0xff172130,0,0),0,r.getY(),pick(0xff070d16,0,0),0,r.getBottom(),false));g.fillRoundedRectangle(r,9);}else{auto fill=pick(0,hover?0xff3b3b3b:0xff292929,hover?0xffffffff:0xfff4f4f4);if(down)fill=fill.contrasting(.08f);g.setColour(fill);g.fillRoundedRectangle(r,8);}g.setColour(pick(0xff34445b,0xff555555,0xffc2c3c6));g.drawRoundedRectangle(r,8,.8f);}
void PocketLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool){
    auto r=b.getLocalBounds().toFloat();auto name=b.getButtonText();
    auto c=r.getCentre();float s=juce::jmin(r.getWidth(),r.getHeight())/40;juce::Path p;
    if(name=="power"){p.addCentredArc(0,1,8,8,0,.65f,juce::MathConstants<float>::twoPi-.65f,true);p.startNewSubPath(0,-10);p.lineTo(0,-1);p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,ink(),1.7f*s);return;}

    if(name=="freeze"){
        // small snowflake: three crossed arms with barbs, lit up while frozen
        for(int arm=0;arm<3;++arm){
            const float a=juce::MathConstants<float>::halfPi+float(arm)*juce::MathConstants<float>::pi/3.f;
            const float dx=std::cos(a),dy=std::sin(a);
            p.startNewSubPath(-dx*10,-dy*10);p.lineTo(dx*10,dy*10);
            for(float sign:{-1.f,1.f})for(float at:{5.5f,9.f}){
                const float bx=dx*at*sign,by=dy*at*sign;
                for(float spread:{.62f,-.62f}){
                    const float ax=std::cos(a+spread),ay=std::sin(a+spread);
                    p.startNewSubPath(bx,by);p.lineTo(bx+ax*3.2f*sign,by+ay*3.2f*sign);
                }
            }
        }
        p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));
        stroke(g,p,(b.getToggleState()?accent():ink()).withAlpha(b.isEnabled()?1.f:.35f),1.35f*s);return;
    }
    constexpr int teeth=10;for(int i=0;i<teeth*4;++i){float a=float(i)*juce::MathConstants<float>::twoPi/float(teeth*4)-juce::MathConstants<float>::halfPi;float radius=(i%4==1||i%4==2)?10.f:7.7f;auto pt=juce::Point<float>(std::cos(a)*radius,std::sin(a)*radius);if(i==0)p.startNewSubPath(pt);else p.lineTo(pt);}p.closeSubPath();p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,ink(),1.65f*s);g.setColour(ink());g.drawEllipse(c.x-3.2f*s,c.y-3.2f*s,6.4f*s,6.4f*s,1.65f*s);
}
void PocketLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float minPos,float maxPos,juce::Slider::SliderStyle style,juce::Slider& slider){
    auto t=tokens();const auto c=slider.getName()=="key"?t.key:(slider.getName()=="out"?t.out:t.neutral);
    const float cy=float(y)+float(h)*.5f,left=style==juce::Slider::TwoValueHorizontal?minPos:float(x),right=style==juce::Slider::TwoValueHorizontal?maxPos:pos;
    g.setColour(t.glass);g.fillRoundedRectangle(float(x),cy-3,float(w),6,3);
    g.setColour(c);g.fillRoundedRectangle(left,cy-2,juce::jmax(.1f,right-left),4,2);
    g.setColour(t.muted.withAlpha(.5f));for(int i=0;i<=10;++i){float tx=float(x)+float(w)*float(i)/10.f;g.drawLine(tx,cy+12,tx,cy+(i%5==0?18.f:15.f),.7f);}
    auto thumb=[&](float px){g.setColour(juce::Colours::black.withAlpha(.22f));g.fillEllipse(px-6,cy-4,12,12);g.setGradientFill(juce::ColourGradient(t.ink,px-4,cy-5,t.neutral,px+5,cy+5,false));g.fillEllipse(px-5.5f,cy-5.5f,11,11);g.setColour(c);g.drawEllipse(px-5.5f,cy-5.5f,11,11,1);};
    if(style==juce::Slider::TwoValueHorizontal){thumb(minPos);thumb(maxPos);}else thumb(pos);
}
ModernDial::ModernDial(PocketLook& l,juce::String t,juce::String sub,juce::String u,juce::uint32 a,bool inf,bool infMin,bool compactDial,juce::String infLabel):look(l),title(t),subtitle(sub),unit(u),infinity(inf),compact(compactDial){juce::ignoreUnused(a,infMin,infLabel);setSliderStyle(juce::Slider::RotaryVerticalDrag);setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);setName(t);setWantsKeyboardFocus(true);}
void ModernDial::paint(juce::Graphics& g){
    const auto t=look.tokens();const float size=float(juce::jmin(getWidth(),getHeight()));
    const auto c=getLocalBounds().toFloat().getCentre();const float r=size*(compact?.33f:.355f),ring=r+size*.047f;
    const auto scale=g.getInternalContext().getPhysicalPixelScaleFactor();const int pw=juce::jmax(1,juce::roundToInt(getWidth()*scale)),ph=juce::jmax(1,juce::roundToInt(getHeight()*scale));
    if(!body.isValid()||body.getWidth()!=pw||body.getHeight()!=ph||bodyTheme!=look.theme||std::abs(bodyScale-scale)>.001f){
        body=juce::Image(juce::Image::ARGB,pw,ph,true);bodyTheme=look.theme;bodyScale=scale;juce::Graphics bg(body);bg.addTransform(juce::AffineTransform::scale(scale));
        auto face=juce::Rectangle<float>(2*r,2*r).withCentre(c);
        juce::Path shadow;shadow.addEllipse(face);juce::DropShadow(juce::Colours::black.withAlpha(.28f),5,{1,3}).drawForPath(bg,shadow);
        bg.setGradientFill(juce::ColourGradient(t.raised.brighter(.12f),c.x-r,c.y-r,t.glass,c.x+r,c.y+r,false));bg.fillEllipse(face);
        // Angular satin reflection, baked once with the body, from top-left.
        for(int sector=0;sector<96;++sector){const float a=float(sector)*juce::MathConstants<float>::twoPi/96.f,b=a+juce::MathConstants<float>::twoPi/96.f;const float light=std::pow(juce::jmax(0.f,std::cos(a+juce::MathConstants<float>::pi*.75f)),8.f);juce::Path wedge;wedge.startNewSubPath(c);wedge.lineTo(c.x+(r-5)*std::cos(a),c.y+(r-5)*std::sin(a));wedge.lineTo(c.x+(r-5)*std::cos(b),c.y+(r-5)*std::sin(b));wedge.closeSubPath();bg.setColour(t.ink.withAlpha(light*.045f));bg.fillPath(wedge);}
        bg.setColour(t.border);bg.drawEllipse(face,1.f);bg.setColour(t.glass);bg.drawEllipse(face.reduced(3),2);
        // Restrained machining: highlights face the same top-left light.
        for(int i=0;i<64;++i){const float a=float(i)*juce::MathConstants<float>::twoPi/64.f;
            const float light=.15f+.45f*juce::jmax(0.f,-std::sin(a)+std::cos(a));
            bg.setColour(t.muted.withAlpha(light));bg.drawLine(c.x+(r-2)*std::sin(a),c.y-(r-2)*std::cos(a),c.x+(r-4)*std::sin(a),c.y-(r-4)*std::cos(a),.6f);}
        for(int i=0;i<=30;++i){const float a=juce::MathConstants<float>::pi*(1.25f+1.5f*float(i)/30.f),rr=ring+size*.06f;
            bg.setColour(t.muted.withAlpha(i%5==0?.65f:.28f));bg.drawLine(c.x+rr*std::sin(a),c.y-rr*std::cos(a),c.x+(rr-(i%5==0?4.f:2.f))*std::sin(a),c.y-(rr-(i%5==0?4.f:2.f))*std::cos(a),.65f);}
    }
    g.drawImageTransformed(body,juce::AffineTransform::scale(1.f/bodyScale));
    if(emphasis>.001f){g.setColour(t.ink.withAlpha(emphasis*.16f));g.drawEllipse(juce::Rectangle<float>(2*r,2*r).withCentre(c).reduced(1),1.f);}
    const float proportion=float(valueToProportionOfLength(getValue())),start=juce::MathConstants<float>::pi*1.25f,end=start+juce::MathConstants<float>::pi*1.5f*proportion;
    const bool autoValue=infinity&&proportion>.9995f;
    auto colour=title=="Duration"?t.neutral:t.out;
    juce::Path track,arc;track.addCentredArc(c.x,c.y,ring,ring,0,start,juce::MathConstants<float>::pi*2.75f,true);stroke(g,track,t.glass,compact?3.f:4.f);
    if(proportion>0&&!autoValue){arc.addCentredArc(c.x,c.y,ring,ring,0,start,end,true);stroke(g,arc,colour,compact?2.5f:3.f);}
    auto marker=juce::Point<float>(c.x+ring*std::sin(end),c.y-ring*std::cos(end));
    if(activity>.004f&&!autoValue){g.setColour(colour.withAlpha(juce::jmin(.18f,activity*.15f)));g.fillEllipse(marker.x-7,marker.y-7,14,14);}
    g.setColour(autoValue?t.muted:colour);g.fillEllipse(marker.x-3,marker.y-3,6,6);
    if(title=="Influence"&&gr>.001f){juce::Path meter;meter.addCentredArc(c.x,c.y,ring+9,ring+9,0,start,start+juce::MathConstants<float>::pi*1.5f*gr,true);stroke(g,meter,t.out.withAlpha(.7f),1.5f);}
    juce::String value=unit=="dB"?juce::String(getValue(),2):juce::String(getValue(),unit=="%"?0:0)+(unit=="%"?"%":" ms");
    if(autoValue&&unit!="%")value="AUTO";
    g.setFont(pocketFont(compact?13.2f:15.f,false,true));g.setColour(t.ink);g.drawText(title,juce::Rectangle<float>{c.x-r,c.y-(compact?22.f:33.f),2*r,20},juce::Justification::centred);
    g.setFont(pocketFont(compact?17.f:(title=="Influence"?29.f:25.f),true));g.drawText(value,juce::Rectangle<float>{c.x-r,c.y-(compact?4.f:12.f),2*r,32},juce::Justification::centred);
    g.setFont(pocketFont(13.2f));g.setColour(t.muted);g.drawText(autoValue?juce::String(unit=="%"?"AUTO":"LEGACY"):subtitle,juce::Rectangle<float>{c.x-r,c.y+(compact?18.f:24.f),2*r,18},juce::Justification::centred);
}
DuckPocketAudioProcessorEditor::DuckPocketAudioProcessorEditor(DuckPocketAudioProcessor& p):AudioProcessorEditor(&p),audioProcessor(p){
    juce::PropertiesFile::Options o;o.applicationName="DuckPocket";o.filenameSuffix="settings";o.folderName="RainlineMusic";o.osxLibrarySubFolder="Application Support";preferences=std::make_unique<juce::PropertiesFile>(o);
    // Solid Dark is the default theme; a stored preference still wins.
    auto saved=preferences->getValue("duckPocket.ui.theme",preferences->getValue("phasePocket.ui.theme","solidDark"));setTheme(saved=="neon"?PocketTheme::Neon:(saved=="amber"?PocketTheme::Amber:(saved=="solidWhite"?PocketTheme::SolidWhite:PocketTheme::SolidDark)),false);
    gainWindow=preferences->getDoubleValue("duckPocket.ui.graphWindow",preferences->getDoubleValue("duckPocket.ui.gainWindow",preferences->getDoubleValue("phasePocket.ui.gainWindow",1.)));scopeWindow=gainWindow;
    setLookAndFeel(&look);setOpaque(true);setResizable(true,true);
    for(auto* c:std::initializer_list<juce::Component*>{&influence,&duration,&outputGain,&sidechainRange,&processingRange,&midSide,&settingsButton,&bypassButton,&freezeButton})addAndMakeVisible(c);
    influenceAttach=std::make_unique<SliderAttachment>(p.parameters,"amount",influence);durationIsRelative=p.parameters.getRawParameterValue("relativeDuration")->load()>.5f;duration.setDurationMode(durationIsRelative);durationAttach=std::make_unique<SliderAttachment>(p.parameters,durationIsRelative?"durationPercent":"duration",duration);duration.setDoubleClickReturnValue(true,durationIsRelative?100:2000);outputAttach=std::make_unique<SliderAttachment>(p.parameters,"outputGain",outputGain);outputGain.setDoubleClickReturnValue(true,0);influence.setDoubleClickReturnValue(true,100);midSide.setSliderStyle(juce::Slider::LinearHorizontal);midSide.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);midSide.setDoubleClickReturnValue(true,0);msAttach=std::make_unique<SliderAttachment>(p.parameters,"msBalance",midSide);midSide.onValueChange=[this]{repaint(scaled(648,568,280,168));};
    bypassButton.setClickingTogglesState(true);bypassAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bypass",bypassButton);settingsButton.onClick=[this]{showSettingsMenu();};
    // One button freezes and resumes both graphs at once.
    freezeButton.setClickingTogglesState(true);freezeButton.setTooltip("Freeze both graphs");
    freezeButton.onClick=[this]{setFrozen(freezeButton.getToggleState());};
    auto setupRange=[](juce::Slider& s){s.setSliderStyle(juce::Slider::TwoValueHorizontal);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setRange(20,20000,0.);};setupRange(sidechainRange);setupRange(processingRange);sidechainRange.setName("key");processingRange.setName("out");midSide.setName("neutral");
    lowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scLow"),[this](float){syncRange();});highAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scHigh"),[this](float){syncRange();});lowAttach->sendInitialUpdate();highAttach->sendInitialUpdate();
    sidechainRange.onDragStart=[this]{rangeGesture=true;lowAttach->beginGesture();highAttach->beginGesture();};sidechainRange.onValueChange=[this]{float a=float(sidechainRange.getMinValue()),b=float(sidechainRange.getMaxValue());repaint(scaled(24,568,304,168));if(rangeGesture){lowAttach->setValueAsPartOfGesture(a);highAttach->setValueAsPartOfGesture(b);}else{lowAttach->setValueAsCompleteGesture(a);highAttach->setValueAsCompleteGesture(b);}};sidechainRange.onDragEnd=[this]{lowAttach->endGesture();highAttach->endGesture();rangeGesture=false;syncRange();};sidechainRange.onResetMin=[this]{lowAttach->setValueAsCompleteGesture(20);syncRange();};sidechainRange.onResetMax=[this]{highAttach->setValueAsCompleteGesture(20000);syncRange();};
    processLowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processLow"),[this](float){syncProcessingRange();});processHighAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processHigh"),[this](float){syncProcessingRange();});processLowAttach->sendInitialUpdate();processHighAttach->sendInitialUpdate();
    processingRange.onDragStart=[this]{processRangeGesture=true;processLowAttach->beginGesture();processHighAttach->beginGesture();};processingRange.onValueChange=[this]{float a=float(processingRange.getMinValue()),b=float(processingRange.getMaxValue());repaint(scaled(328,568,304,168));if(processRangeGesture){processLowAttach->setValueAsPartOfGesture(a);processHighAttach->setValueAsPartOfGesture(b);}else{processLowAttach->setValueAsCompleteGesture(a);processHighAttach->setValueAsCompleteGesture(b);}};processingRange.onDragEnd=[this]{processLowAttach->endGesture();processHighAttach->endGesture();processRangeGesture=false;syncProcessingRange();};processingRange.onResetMin=[this]{processLowAttach->setValueAsCompleteGesture(20);syncProcessingRange();};processingRange.onResetMax=[this]{processHighAttach->setValueAsCompleteGesture(20000);syncProcessingRange();};
    duration.setTooltip("Key length: 100% = AUTO; shorter percentages use the last measured event. First event uses AUTO. Legacy sessions retain milliseconds.");outputGain.setTooltip("Output gain: -12 to +6 dB; double-click resets to 0 dB");influence.setTooltip("Ducking depth");bypassButton.setTooltip("Enable / bypass processing");settingsButton.setTooltip("Settings");sidechainRange.setTooltip("Detector filter, 20 Hz to 20 kHz; Alt-click or double-click resets the nearest handle");midSide.setTooltip("Stereo Mid/Side ducking balance; mono input contains Mid only");processingRange.setTooltip("Frequency range affected by ducking; 20 Hz to 20 kHz keeps the plain wideband duck");
    int width=p.editorWidth.load();if(width<800||width>1500)width=preferences->getIntValue("duckPocket.ui.width",960);width=juce::jlimit(800,1500,width);
    setResizeLimits(800,633,1500,1188);getConstrainer()->setFixedAspectRatio(960./760.);setSize(width,juce::roundToInt(width*760./960.));
    preferences->removeValue("duckPocket.ui.expanded");
    // A fixed column budget bounds path construction inside the host GUI.
    pathPoints.reserve(1200);pathTop.reserve(1202);pathBottom.reserve(1202);bucketLo.reserve(1200);bucketHi.reserve(1200);bucketScratch.reserve(1200);
    ready=true;p.editorWidth.store(width);PocketTrace discard;while(p.popTrace(discard)){}p.editorOpen.store(true);frameTick();
    // VBlank-driven rendering: long windows have a 30 fps budget; short
    // windows retain the 60 fps interaction rate.
    vblank=std::make_unique<juce::VBlankAttachment>(this,[this]{
        const double now=juce::Time::getMillisecondCounterHiRes();
        const double minimumGap=gainWindow>=2.?1000./30.:1000./60.;
        if(nextFrameMs>0&&now+.75<nextFrameMs)return;
        nextFrameMs=nextFrameMs>0&&now-nextFrameMs<minimumGap?nextFrameMs+minimumGap:now+minimumGap;
        frameTick();
    });
}
DuckPocketAudioProcessorEditor::~DuckPocketAudioProcessorEditor(){vblank.reset();
#if DUCK_ENABLE_OPENGL
    if(glowRenderer){glowRenderer->stop();}
    glowRenderer.reset();
#endif
    saveSize();audioProcessor.editorOpen.store(false);if(rangeGesture){lowAttach->endGesture();highAttach->endGesture();}if(processRangeGesture){processLowAttach->endGesture();processHighAttach->endGesture();}setLookAndFeel(nullptr);}
void DuckPocketAudioProcessorEditor::saveSize(){if(!ready||!preferences)return;audioProcessor.editorWidth.store(getWidth());preferences->setValue("duckPocket.ui.width",getWidth());preferences->saveIfNeeded();resizeStamp=0;}
void DuckPocketAudioProcessorEditor::invalidateChrome(){chromeValid=false;repaint();}
void DuckPocketAudioProcessorEditor::setTheme(PocketTheme t,bool persist){look.theme=t;if(persist&&preferences){preferences->setValue("duckPocket.ui.theme",t==PocketTheme::Neon?"neon":(t==PocketTheme::Amber?"amber":(t==PocketTheme::SolidDark?"solidDark":"solidWhite")));preferences->saveIfNeeded();}chromeValid=false;for(auto& layer:softwarePlots)layer.reset();repaint();for(auto* c:getChildren())c->repaint();if(bypassMix>0)juce::MessageManager::callAsync([safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this)]{if(safe)safe->captureBlurSnapshot();});}
void DuckPocketAudioProcessorEditor::setHistoryWindow(double seconds){gainWindow=scopeWindow=seconds;preferences->setValue("duckPocket.ui.graphWindow",seconds);preferences->saveIfNeeded();invalidateChrome();}
// A single snowflake button freezes and resumes both graphs together.
void DuckPocketAudioProcessorEditor::setFrozen(bool frozen){
    triggerStamp=-1;gainFrozen=scopeFrozen=frozen;frozenGain.clear();frozenSummary.clear();
    if(frozen){
        frozenGain.reserve(size_t(filled));
        for(int i=0;i<filled;++i)frozenGain.push_back(history[size_t((cursor-filled+i+historyCapacity)%historyCapacity)]);
        frozenSummary.reserve(size_t(summaryFilled));
        for(int i=0;i<summaryFilled;++i)frozenSummary.push_back(summaryHistory[size_t((summaryCursor-summaryFilled+i+summaryCapacity)%summaryCapacity)]);
    }else{
        const double resume=filled?history[size_t((cursor+historyCapacity-1)%historyCapacity)].time:0.;
        gainResume=scopeResume=resume;
    }
    freezeButton.setToggleState(frozen,juce::dontSendNotification);freezeButton.repaint();
    repaint(gainArea);repaint(scopeArea);
}
void DuckPocketAudioProcessorEditor::showSettingsMenu(){juce::PopupMenu root,window,theme;for(size_t i=0;i<windows.size();++i)window.addItem(int(i)+1,timeLabel(windows[i]),true,std::abs(gainWindow-windows[i])<1e-6);theme.addItem(201,"Neon",true,look.theme==PocketTheme::Neon);theme.addItem(204,"Amber",true,look.theme==PocketTheme::Amber);theme.addItem(202,"Solid Dark",true,look.theme==PocketTheme::SolidDark);theme.addItem(203,"Solid White",true,look.theme==PocketTheme::SolidWhite);root.addSubMenu("Graph window",window);root.addSeparator();root.addSubMenu("Theme",theme);
#if DUCK_ENABLE_OPENGL
root.addSeparator();root.addItem(401,"OpenGL (experimental)",true,glowRenderer!=nullptr);
#endif
root.addSeparator();root.addItem(301,"Percentage Duration",true,durationIsRelative);auto safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this);root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton),[safe](int id){if(!safe||id==0)return;
#if DUCK_ENABLE_OPENGL
if(id==401){safe->setOpenGL(safe->glowRenderer==nullptr);return;}
#endif
if(id>=1&&id<=6)safe->setHistoryWindow(windows[size_t(id-1)]);else if(id==301){auto* prm=safe->audioProcessor.parameters.getParameter("relativeDuration");prm->beginChangeGesture();prm->setValueNotifyingHost(safe->durationIsRelative?0.f:1.f);prm->endChangeGesture();safe->syncDurationMode();}else if(id==201)safe->setTheme(PocketTheme::Neon);else if(id==204)safe->setTheme(PocketTheme::Amber);else if(id==202)safe->setTheme(PocketTheme::SolidDark);else if(id==203)safe->setTheme(PocketTheme::SolidWhite);});}
void DuckPocketAudioProcessorEditor::syncDurationMode(){
    const bool relative=audioProcessor.parameters.getRawParameterValue("relativeDuration")->load()>.5f;
    if(relative==durationIsRelative)return;
    durationIsRelative=relative;durationAttach.reset();duration.setDurationMode(relative);
    durationAttach=std::make_unique<SliderAttachment>(audioProcessor.parameters,relative?"durationPercent":"duration",duration);
    duration.setDoubleClickReturnValue(true,relative?100:2000);
}
// Read the values straight from the parameters. The raw atomics are updated by a
// separate listener that can run *after* this attachment callback, so when a host
// changes both ends at once (factory default, preset load) one end was stale.
static float paramValue(juce::AudioProcessorValueTreeState& apvts,const char* id){auto* prm=apvts.getParameter(id);return prm->convertFrom0to1(prm->getValue());}
void DuckPocketAudioProcessorEditor::syncRange(){if(rangeGesture)return;float a=paramValue(audioProcessor.parameters,"scLow"),b=paramValue(audioProcessor.parameters,"scHigh");sidechainRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(24,568,304,168));}
void DuckPocketAudioProcessorEditor::syncProcessingRange(){if(processRangeGesture)return;float a=paramValue(audioProcessor.parameters,"processLow"),b=paramValue(audioProcessor.parameters,"processHigh");processingRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(328,568,304,168));}
juce::Rectangle<int> DuckPocketAudioProcessorEditor::scaled(float x,float y,float w,float h) const {float s=float(getWidth())/960;return{juce::roundToInt(x*s),juce::roundToInt(y*s),juce::roundToInt(w*s),juce::roundToInt(h*s)};}
void DuckPocketAudioProcessorEditor::resized(){
    settingsButton.setBounds(scaled(838,22,40,40));bypassButton.setBounds(scaled(888,22,40,40));
    influence.setBounds(scaled(714,100,188,188));duration.setBounds(scaled(720,372,176,176));outputGain.setBounds(scaled(763,288,90,76));
    freezeButton.setBounds(scaled(608,513,28,28));sidechainRange.setBounds(scaled(48,624,256,36));processingRange.setBounds(scaled(352,624,256,36));midSide.setBounds(scaled(656,624,256,36));
    blurArea=scaled(12,80,936,668);gainArea=scaled(24,88,640,224);scopeArea=scaled(24,328,640,224);
    blurredSnapshot={};chromeValid=false;if(ready){audioProcessor.editorWidth.store(getWidth());resizeStamp=juce::Time::getMillisecondCounterHiRes();}
}
void DuckPocketAudioProcessorEditor::panel(juce::Graphics& g,juce::Rectangle<float> r){
    const auto t=look.tokens();juce::Path shadow;shadow.addRoundedRectangle(r,10);juce::DropShadow(juce::Colours::black.withAlpha(.20f),5,{0,2}).drawForPath(g,shadow);
    g.setGradientFill(juce::ColourGradient(t.raised.brighter(.035f),r.getX(),r.getY(),t.raised.darker(.035f),r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,10);
    g.setColour(t.border);g.drawRoundedRectangle(r,10,.8f);g.setColour(look.isDark()?juce::Colours::white.withAlpha(.055f):juce::Colours::white.withAlpha(.45f));g.drawLine(r.getX()+10,r.getY()+.5f,r.getRight()-10,r.getY()+.5f,.8f);
}
/*
    Only the moving traces are painted here. Everything static (panels, grids,
    labels, gradients and their glow passes) lives in the cached chrome image, so
    a timer tick no longer re-rasterises the whole window. That was the real cost
    of running several open editors in one host: each one repainted every pixel,
    with glow, 60 times a second on the shared message thread.
*/
void DuckPocketAudioProcessorEditor::graph(juce::Graphics& g,juce::Rectangle<float> box,bool gain){
    const bool glow=look.hasGlow();
    const juce::Rectangle<float> plot(box.getX()+18,box.getY()+47,box.getWidth()-70,box.getHeight()-87);
    const bool frozen=gain?gainFrozen:scopeFrozen;
    // The two-millisecond rollup preserves min/max peaks while reducing the
    // five-second scan from ~12k records to ~2.5k records per graph.
    const bool longWindow=(gain?gainWindow:scopeWindow)>=2.;
    const auto& snap=longWindow?frozenSummary:frozenGain;
    const int count=frozen?int(snap.size()):(longWindow?summaryFilled:filled);
    if(count<2)return;
    auto at=[&](int i)->const PocketTrace& {return frozen?snap[size_t(i)]:(longWindow?summaryHistory[size_t((summaryCursor-count+i+summaryCapacity)%summaryCapacity)]:history[size_t((cursor-count+i+historyCapacity)%historyCapacity)]);};
    const double window=gain?gainWindow:scopeWindow,resume=gain?gainResume:scopeResume;
    // Smoothed display clock (see frameTick) instead of "time of the newest packet",
    // which advances in audio-block sized steps and made the whole graph stutter.
    const double now=(frozen||displayTime<=0.)?at(count-1).time:displayTime;
    const auto label=look.tokens().out;
    auto emit=[&](const juce::Path& path,juce::Colour colour,float width){
        if(emissionGraphics){emissionGraphics->setGradientFill(juce::ColourGradient(colour.withAlpha(.18f),plot.getX(),0,colour,plot.getRight(),0,false));emissionGraphics->strokePath(path,juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
    };
    juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(plot.toNearestInt());
    // Column count must track *physical* pixels, not the fixed 960-wide design
    // rect. `g` already carries the editor's own upscale transform (paint()
    // applies getWidth()/960 before calling us) plus the OS/Retina device
    // scale, so getPhysicalPixelScaleFactor() gives the true logical-to-device
    // ratio in one number. Without this the trace was always built from ~600
    // points (the design width) and then stretched to however big the window
    // or display scale actually was, which is what made it look chunky/low-res
    // once resized above ~960px or viewed on a HiDPI screen.
    const float physicalScale=juce::jlimit(1.f,8.f,g.getInternalContext().getPhysicalPixelScaleFactor());
    const int columns=juce::jlimit(2,1200,juce::roundToInt(plot.getWidth()*physicalScale));
    const float span=plot.getWidth()/float(columns-1);
    // Buckets are locked to absolute time, so every bucket always holds the same
    // samples while it scrolls; the fractional part of "now" shifts them by sub-pixel
    // amounts. Previously the bucket edges moved with "now", so min/max values of a
    // column changed from frame to frame (worst at long windows, where one column
    // covers dozens of packets) and the graph shimmered.
    const double bucketSeconds=window/double(columns-1);
    const long long newestIdx=static_cast<long long>(std::floor(now/bucketSeconds));
    const float frac=float(now/bucketSeconds-double(newestIdx));
    auto xOf=[&](int c){return plot.getX()+(float(c)+.5f-frac)*span;};
    auto bucketAge=[&](double t){return newestIdx-static_cast<long long>(std::floor(t/bucketSeconds));};

    // Fill holes between real trace buckets. At the 100 ms view there are fewer
    // audio packets than physical pixels; interpolation prevents long stair-step
    // diagonals without inventing peaks (the real bucket extrema remain anchors).
    auto interpolate=[&](bool pair){
        int previous=-1;
        for(int c=0;c<columns;++c){
            const bool valid=pair?bucketHi[size_t(c)]>=bucketLo[size_t(c)]:bucketLo[size_t(c)]<=1.f;
            if(!valid)continue;
            if(previous>=0&&c>previous+1){
                const float lo0=bucketLo[size_t(previous)],lo1=bucketLo[size_t(c)];
                const float hi0=pair?bucketHi[size_t(previous)]:0.f,hi1=pair?bucketHi[size_t(c)]:0.f;
                for(int x=previous+1;x<c;++x){const float t=float(x-previous)/float(c-previous);bucketLo[size_t(x)]=lo0+t*(lo1-lo0);if(pair)bucketHi[size_t(x)]=hi0+t*(hi1-hi0);}
            }
            previous=c;
        }
    };

    if(gain){
        bucketLo.assign(size_t(columns),2.f);
        for(int i=count-1;i>=0;--i){const auto& v=at(i);const long long k=bucketAge(v.time);if(k>columns-1)break;if(k<1)continue;if(!frozen&&v.time<resume)continue;if(!std::isfinite(v.gain))continue;const size_t c=size_t(columns-1-int(k));bucketLo[c]=juce::jmin(bucketLo[c],juce::jlimit(0.f,1.f,v.gain));}
        interpolate(false);
        // Display-only 3-tap reconstruction filter. It removes the one-pixel
        // bucket phase changes that shimmer while the trace scrolls; audio and
        // stored extrema are untouched.
        bucketScratch=bucketLo;
        for(int c=1;c+1<columns;++c)if(bucketLo[size_t(c-1)]<=1.f&&bucketLo[size_t(c)]<=1.f&&bucketLo[size_t(c+1)]<=1.f)bucketScratch[size_t(c)]=.25f*bucketLo[size_t(c-1)]+.5f*bucketLo[size_t(c)]+.25f*bucketLo[size_t(c+1)];
        bucketLo.swap(bucketScratch);pathPoints.clear();
        for(int c=0;c<columns;++c)if(bucketLo[size_t(c)]<=1.f)pathPoints.push_back({xOf(c),plot.getBottom()-bucketLo[size_t(c)]*plot.getHeight()});
        if(pathPoints.size()>1){
            juce::Path fill;fill.startNewSubPath(pathPoints.front());for(const auto& point:pathPoints)fill.lineTo(point);fill.lineTo(pathPoints.back().x,plot.getY());fill.lineTo(pathPoints.front().x,plot.getY());fill.closeSubPath();
            g.setGradientFill(juce::ColourGradient(label.withAlpha(.02f),0,plot.getY(),label.withAlpha(.20f),0,plot.getBottom(),false));g.fillPath(fill);
            juce::Path path;if(longWindow){path.startNewSubPath(pathPoints.front());for(size_t i=1;i<pathPoints.size();++i)path.lineTo(pathPoints[i]);}else path=smoothPath(pathPoints);
            if(!emissionGraphics&&glow&&signalPeak>.004f&&!frozen)stroke(g,path,label.withAlpha(.10f),4.2f);
            g.setGradientFill(juce::ColourGradient(label.withAlpha(.20f),plot.getX(),0,label,plot.getRight(),0,false));g.strokePath(path,juce::PathStrokeType(1.65f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));emit(path,label,2.5f);
        }
        return;
    }

    for(int kind=1;kind>=0;--kind){
        bucketLo.assign(size_t(columns),1.f);bucketHi.assign(size_t(columns),-1.f);
        bool any=false;
        for(int i=count-1;i>=0;--i){const auto& v=at(i);const long long k=bucketAge(v.time);if(k>columns-1)break;if(k<1)continue;if(!frozen&&v.time<resume)continue;const float lo=kind?v.keyLo:v.outLo,hi=kind?v.keyHi:v.outHi;if(!std::isfinite(lo)||!std::isfinite(hi))continue;const size_t c=size_t(columns-1-int(k));bucketLo[c]=juce::jmin(bucketLo[c],juce::jlimit(-1.f,1.f,lo));bucketHi[c]=juce::jmax(bucketHi[c],juce::jlimit(-1.f,1.f,hi));any=true;}
        if(!any)continue;
        interpolate(true);
        bucketScratch=bucketLo;
        for(int c=1;c+1<columns;++c)if(bucketHi[size_t(c-1)]>=bucketLo[size_t(c-1)]&&bucketHi[size_t(c)]>=bucketLo[size_t(c)]&&bucketHi[size_t(c+1)]>=bucketLo[size_t(c+1)])bucketScratch[size_t(c)]=.25f*bucketLo[size_t(c-1)]+.5f*bucketLo[size_t(c)]+.25f*bucketLo[size_t(c+1)];
        bucketLo.swap(bucketScratch);bucketScratch=bucketHi;
        for(int c=1;c+1<columns;++c)if(bucketHi[size_t(c-1)]>=bucketLo[size_t(c-1)]&&bucketHi[size_t(c)]>=bucketLo[size_t(c)]&&bucketHi[size_t(c+1)]>=bucketLo[size_t(c+1)])bucketScratch[size_t(c)]=.25f*bucketHi[size_t(c-1)]+.5f*bucketHi[size_t(c)]+.25f*bucketHi[size_t(c+1)];
        bucketHi.swap(bucketScratch);pathTop.clear();pathBottom.clear();
        // No centre trace in silence: only columns whose envelope is visibly above
        // the noise floor are drawn. Each audible run becomes its own shape that
        // tapers to the centre line at both ends, so nothing is drawn across gaps.
        constexpr float silenceThreshold=.004f;
        const float midY=plot.getCentreY();
        auto active=[&](int c){const float lo=bucketLo[size_t(c)],hi=bucketHi[size_t(c)];return hi>=lo&&juce::jmax(std::abs(lo),std::abs(hi))>silenceThreshold;};
        const auto colour=kind?look.tokens().key:look.tokens().out;
        for(int c=0;c<columns;){
            if(!active(c)){++c;continue;}
            int e=c;while(e+1<columns&&active(e+1))++e;
            pathTop.clear();pathBottom.clear();
            const float x0=xOf(c-1),x1=xOf(e+1);
            pathTop.push_back({x0,midY});pathBottom.push_back({x0,midY});
            for(int i=c;i<=e;++i){const float x=xOf(i);pathTop.push_back({x,midY-bucketHi[size_t(i)]*plot.getHeight()*.5f});pathBottom.push_back({x,midY-bucketLo[size_t(i)]*plot.getHeight()*.5f});}
            pathTop.push_back({x1,midY});pathBottom.push_back({x1,midY});
            c=e+1;
            juce::Path body;
            body.startNewSubPath(pathTop.front());
            if(longWindow){for(size_t i=1;i<pathTop.size();++i)body.lineTo(pathTop[i]);}
            else {for(size_t i=1;i+1<pathTop.size();++i)body.quadraticTo(pathTop[i],(pathTop[i]+pathTop[i+1])*.5f);body.lineTo(pathTop.back());}
            body.lineTo(pathBottom.back());
            if(longWindow){for(size_t i=pathBottom.size()-1;i>0;--i)body.lineTo(pathBottom[i-1]);}
            else for(size_t i=pathBottom.size()-1;i>1;--i)body.quadraticTo(pathBottom[i-1],(pathBottom[i-1]+pathBottom[i-2])*.5f);
            body.lineTo(pathBottom.front());body.closeSubPath();
            if(!emissionGraphics&&glow&&!longWindow&&!frozen){g.setColour(colour.withAlpha(.12f));g.strokePath(body,juce::PathStrokeType(3.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
            // No outline: the wave itself is filled with the colour the outline used to have.
            g.setGradientFill(juce::ColourGradient(colour.withAlpha(.12f),plot.getX(),0,colour.withAlpha(kind?.7f:.78f),plot.getRight(),0,false));g.fillPath(body);emit(body,colour,2.f);
        }
    }
}
void DuckPocketAudioProcessorEditor::paintChrome(juce::Graphics& g){
    const float physicalScale=juce::jlimit(.75f,4.f,g.getInternalContext().getPhysicalPixelScaleFactor());
    const int w=juce::jmax(1,juce::roundToInt(getWidth()*physicalScale)),h=juce::jmax(1,juce::roundToInt(getHeight()*physicalScale));
    const bool resizing=resizeStamp>0&&juce::Time::getMillisecondCounterHiRes()-resizeStamp<100;
    if((!chromeValid||!chrome.isValid()||chrome.getWidth()!=w||chrome.getHeight()!=h||std::abs(chromeScale-physicalScale)>.001f)&&(!resizing||!chrome.isValid())){
        ++chromeBuildCount;chrome=juce::Image(juce::Image::ARGB,w,h,true);chromeScale=physicalScale;juce::Graphics cg(chrome);cg.addTransform(juce::AffineTransform::scale(physicalScale*float(getWidth())/960.f));const auto t=look.tokens();
        cg.setGradientFill(juce::ColourGradient(t.chassis.brighter(.045f),0,0,t.chassis,960,760,false));cg.fillRect(0,0,960,760);
        // Original deterministic texture, cached once; never an ambient animation.
        juce::Random noise(0xD0C);for(int i=0;i<6500;++i){cg.setColour((i%2?juce::Colours::white:juce::Colours::black).withAlpha(.02f));cg.fillRect(float(noise.nextInt(960)),float(noise.nextInt(760)),1.f,1.f);}
        juce::Path duck;duck.startNewSubPath(33,46);duck.cubicTo(34,30,47,27,53,35);duck.cubicTo(62,35,62,48,54,52);duck.lineTo(65,57);duck.lineTo(52,58);duck.cubicTo(47,65,35,61,33,54);duck.lineTo(24,52);duck.lineTo(33,46);cg.setColour(t.brand);cg.fillPath(duck);
        cg.setFont(pocketFont(27,false,true));cg.setColour(t.ink);cg.drawText("DUCK POCKET",juce::Rectangle<float>{80,22,480,42},juce::Justification::centredLeft);
        text(cg,"SIDECHAIN DUCKER",{574,26,232,32},11,t.muted,juce::Justification::centredRight);
        auto frame=[&](juce::Rectangle<float> box,bool gain){
            panel(cg,box);auto plot=juce::Rectangle<float>(box.getX()+18,box.getY()+47,box.getWidth()-70,box.getHeight()-87);
            cg.setGradientFill(juce::ColourGradient(t.glass.darker(.08f),plot.getX(),plot.getY(),t.glass.brighter(.025f),plot.getRight(),plot.getBottom(),false));cg.fillRoundedRectangle(plot.expanded(10,5),6);
            text(cg,gain?"GAIN HISTORY":"OSCILLOSCOPE",{box.getX()+18,box.getY()+10,240,26},13,t.ink);
            if(gain)text(cg,"GAIN",{box.getRight()-180,box.getY()+10,140,26},11,t.muted,juce::Justification::centredRight);
            else {text(cg,"OUT",{box.getRight()-160,box.getY()+10,55,26},11,t.out);text(cg,"KEY",{box.getRight()-100,box.getY()+10,55,26},11,t.key);}
            const float top=plot.getY(),bottom=plot.getBottom(),midY=plot.getCentreY(),height=plot.getHeight(),left=plot.getX(),right=plot.getRight(),endS=height*.10f,ringS=height*.04f;
            auto arc=[&](float cx,float bend){juce::Path p;p.startNewSubPath(cx,top);p.quadraticTo(cx+2*bend,midY,cx,bottom);stroke(cg,p,t.major,.7f);};
            arc(left,-endS);arc(left,endS);arc(right,endS);arc(right,-endS);arc(left+plot.getWidth()*.25f,ringS);arc(left+plot.getWidth()*.75f,-ringS);
            cg.setColour(t.major);cg.drawLine(plot.getCentreX(),top,plot.getCentreX(),bottom,.7f);cg.drawLine(left,top,right,top,.7f);cg.drawLine(left,bottom,right,bottom,.7f);
            for(int i=1;i<8;++i){const float u=float(i)/8.f,y=top+u*height,offset=4*endS*u*(1-u);cg.setColour(i%2?t.minor:t.major);cg.drawLine(left+offset,y,right-offset,y,.65f);}
            // Atmospheric depth comes from a static fade of the grid, never labels.
            cg.setGradientFill(juce::ColourGradient(t.glass.withAlpha(.32f),left,midY,t.glass.withAlpha(0.f),right,midY,false));cg.fillRect(plot);
            for(int i=0;i<=4;++i){const float y=top+height*float(i)/4.f;juce::String label=gain?juce::String(100-i*25)+"%":juce::String(1.-.5*i,1);text(cg,label,{right+7,y-9,45,18},11,t.muted);}
            const double window=gain?gainWindow:scopeWindow;text(cg,"-"+timeLabel(window),{left,box.getBottom()-28,140,20},11,t.muted);text(cg,"NOW",{right-80,box.getBottom()-28,80,20},11,t.muted,juce::Justification::centredRight);
        };
        frame({24,88,640,224},true);frame({24,328,640,224},false);panel(cg,{680,88,256,464});panel(cg,{24,568,912,168});
        cg.setColour(t.border);for(float x:{328.f,632.f})cg.drawLine(x,584,x,712,.7f);cg.drawLine(700,284,916,284,.7f);cg.drawLine(700,368,916,368,.7f);
        chromeValid=true;
    }
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
#if DUCK_ENABLE_OPENGL
    juce::Graphics::ScopedSaveState clip(g);
    if(glowRenderer&&glowRenderer->presented.load()&&!capturingBlur&&bypassMix<.5f){
        // Component painting overlays OpenGL. Transparent plot apertures expose
        // the GPU-rendered glass/grid/glow; crisp CPU cores remain above it.
        g.excludeClipRegion(scaled(42,135,570,137));g.excludeClipRegion(scaled(42,375,570,137));
    }
#endif
    g.drawImage(chrome,getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);
}

void DuckPocketAudioProcessorEditor::paintDynamicLabels(juce::Graphics& g){
    const auto primary=look.ink(),secondary=look.muted();
    text(g,"Sidechain filter",{48,586,256,28},15,primary);
    text(g,"Processing range",{352,586,256,28},15,primary);
    text(g,"Mid / Side",{656,586,256,28},15,primary);
    for(int i=0;i<2;++i){auto& range=i?processingRange:sidechainRange;const float x=48.f+304.f*float(i);
        text(g,hz(range.getMinValue()),{x,672,128,24},12,secondary);
        text(g,hz(range.getMaxValue()),{x+128,672,128,24},12,secondary,juce::Justification::centredRight);}
    const float v=float(midSide.getValue());
    const int mid=juce::roundToInt((1.f-juce::jmax(0.f,v))*100.f),side=juce::roundToInt((1.f+juce::jmin(0.f,v))*100.f);
    text(g,"Mid: "+juce::String(mid)+"%",{656,672,128,24},12,secondary);
    text(g,"Side: "+juce::String(side)+"%",{784,672,128,24},12,secondary,juce::Justification::centredRight);
}

void DuckPocketAudioProcessorEditor::paint(juce::Graphics& g){
    ++paintCount;paintChrome(g);juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(float(getWidth())/960));

#if DUCK_ENABLE_OPENGL
    if(glowRenderer&&glowRenderer->ready.load()&&!capturingBlur&&bypassMix<.5f){
        auto frame=std::make_shared<PocketGlowRenderer::Frame>();frame->width=getWidth();frame->height=getHeight();frame->chromeRevision=chromeBuildCount;
        for(int i=0;i<2;++i){const float y=i?328.f:88.f;const juce::Rectangle<float> plot(42,y+47,570,137);auto& layer=frame->plots[size_t(i)];layer.bounds=scaled(42,y+47,570,137);
            const float device=float(chrome.getWidth())/float(getWidth());auto crop=(layer.bounds.toFloat()*device).toNearestInt().getIntersection(chrome.getBounds());layer.background=chrome.getClippedImage(crop);
            const float raster=g.getInternalContext().getPhysicalPixelScaleFactor()*.5f;
            layer.emission=juce::Image(juce::Image::ARGB,juce::jmax(1,juce::roundToInt(plot.getWidth()*raster)),juce::jmax(1,juce::roundToInt(plot.getHeight()*raster)),true,juce::SoftwareImageType());
            juce::Graphics eg(layer.emission);eg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(raster));emissionGraphics=&eg;graph(g,{24,y,640,224},i==0);emissionGraphics=nullptr;
            layer.intensity=(gainFrozen||scopeFrozen)?0.f:juce::jlimit(0.f,.32f,signalPeak*.25f+currentReduction*.07f);
        }
        glowRenderer->publish(std::move(frame));
    }else
#endif
    {for(int i=0;i<2;++i){const float y=i?328.f:88.f;const juce::Rectangle<float> plot(42,y+47,570,137);auto& layer=softwarePlots[size_t(i)];
        const float device=g.getInternalContext().getPhysicalPixelScaleFactor();layer.prepare(juce::jmax(1,juce::roundToInt(plot.getWidth()*device)),juce::jmax(1,juce::roundToInt(plot.getHeight()*device)));
        juce::Graphics cg(layer.core);cg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(device));
        juce::Graphics eg(layer.emission);const float raster=float(layer.emission.getWidth())/plot.getWidth();eg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(raster));emissionGraphics=&eg;graph(cg,{24,y,640,224},i==0);emissionGraphics=nullptr;
        const float chromeDevice=float(chrome.getWidth())/float(getWidth());auto crop=(scaled(42,y+47,570,137).toFloat()*chromeDevice).toNearestInt().getIntersection(chrome.getBounds());
        const float intensity=(gainFrozen||scopeFrozen)?0.f:juce::jlimit(0.f,look.hasGlow()?.22f:.07f,signalPeak*.18f+currentReduction*.045f);
        layer.paint(g,chrome.getClippedImage(crop),plot,intensity,displayTime,i?scopeWindow:gainWindow,i==1);
    }}
    if(triggerStamp>=0&&!gainFrozen){const float flash=1.f-float((juce::Time::getMillisecondCounterHiRes()-triggerStamp)/180.);if(flash>0){g.setColour(look.tokens().out.withAlpha(flash*.45f));g.fillRect(610.f,135.f,2.f,137.f);}}
    paintDynamicLabels(g);
}
void DuckPocketAudioProcessorEditor::captureBlurSnapshot(){
    if(capturingBlur||blurArea.isEmpty())return;
    capturingBlur=true;const float old=bypassMix;bypassMix=0;
    auto source=createComponentSnapshot(blurArea,true,1.f);
    bypassMix=old;capturingBlur=false;
    if(!source.isValid()||source.getWidth()<8||source.getHeight()<8)return;
    const int w=juce::jmax(16,source.getWidth()/6),h=juce::jmax(16,source.getHeight()/6);
    juce::Image small(juce::Image::ARGB,w,h,true);
    {juce::Graphics sg(small);sg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);sg.drawImage(source,juce::Rectangle<float>(0,0,float(w),float(h)),juce::RectanglePlacement::stretchToFit);}
    juce::Image soft(juce::Image::ARGB,w,h,true);
    juce::ImageConvolutionKernel kernel(9);kernel.createGaussianBlur(2.2f);kernel.applyToImage(soft,small,small.getBounds());
    blurredSnapshot=soft;
}
void DuckPocketAudioProcessorEditor::paintOverChildren(juce::Graphics& g){
    if(capturingBlur||bypassMix<.5f||!blurredSnapshot.isValid())return;
    juce::Graphics::ScopedSaveState save(g);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(blurredSnapshot,blurArea.toFloat(),juce::RectanglePlacement::stretchToFit);
    g.setColour((look.isDark()?juce::Colours::black:juce::Colours::white).withAlpha(.18f));g.fillRoundedRectangle(blurArea.toFloat(),12);
    const auto centre=blurArea.toFloat().translated(0,-float(blurArea.getHeight())*.05f);
    const float size=juce::jmax(34.f,float(getWidth())/18.f);
    const auto ink=look.isDark()?juce::Colours::white:juce::Colour(0xff202124);
    g.setColour((look.isDark()?juce::Colours::black:juce::Colours::white).withAlpha(.55f));g.setFont(uiFont(size));g.drawText("BYPASSED",centre.translated(0,2),juce::Justification::centred);
    text(g,"BYPASSED",centre,size,ink,juce::Justification::centred,look.isDark()?.1f:0.f);
}
void DuckPocketAudioProcessorEditor::parentHierarchyChanged(){
#if DUCK_ENABLE_OPENGL
    if(getPeer()&&preferences&&preferences->getBoolValue("duckPocket.ui.opengl",false)&&!glowRenderer)setOpenGL(true,false);
#endif
}
#if DUCK_ENABLE_OPENGL
void DuckPocketAudioProcessorEditor::setOpenGL(bool enabled,bool persist){
    if(glowRenderer){glowRenderer->stop();glowRenderer.reset();}
    glWasReady=false;setOpaque(!enabled);
    if(enabled&&getPeer()){glowRenderer=std::make_unique<PocketGlowRenderer>();glAttachTime=juce::Time::getMillisecondCounterHiRes();glowRenderer->attach(*this);}
    if(persist&&preferences){preferences->setValue("duckPocket.ui.opengl",enabled);preferences->saveIfNeeded();}
    invalidateChrome();
}
#endif
void DuckPocketAudioProcessorEditor::frameTick(){
#if DUCK_ENABLE_OPENGL
    if(glowRenderer){const bool readyGL=glowRenderer->ready.load();if(readyGL&&!glWasReady){glWasReady=true;repaint();}
        if(glowRenderer->failed.load()||(!readyGL&&juce::Time::getMillisecondCounterHiRes()-glAttachTime>2000)){setOpenGL(false);}}
#endif
    if(resizeStamp>0&&juce::Time::getMillisecondCounterHiRes()-resizeStamp>400)saveSize();
    if(triggerStamp>=0){repaint(gainArea);if(juce::Time::getMillisecondCounterHiRes()-triggerStamp>=180)triggerStamp=-1;}
    syncDurationMode();
    if(!chromeValid&&resizeStamp>0&&juce::Time::getMillisecondCounterHiRes()-resizeStamp>100)repaint();
    const auto epoch=audioProcessor.traceGeneration.load(std::memory_order_relaxed);
    if(epoch!=traceGeneration){traceGeneration=epoch;cursor=filled=summaryCursor=summaryFilled=0;summaryBin=-1;displayTime=lastClock=lastLatest=gapMax=0;lastPaintedTime=-1;lastVisibleSignalTime=-1;signalPeak=currentReduction=0;for(auto& layer:softwarePlots)layer.reset();triggerStamp=-1;repaint(gainArea);repaint(scopeArea);}
    PocketTrace v;bool fresh=false;
    while(audioProcessor.popTrace(v)){
        if(v.generation!=traceGeneration||!std::isfinite(v.time))continue;
        history[size_t(cursor)]=v;cursor=(cursor+1)%historyCapacity;filled=juce::jmin(filled+1,historyCapacity);
        const auto bin=static_cast<long long>(std::floor(v.time*500.));
        if(bin!=summaryBin){summaryBin=bin;summaryHistory[size_t(summaryCursor)]=v;summaryCursor=(summaryCursor+1)%summaryCapacity;summaryFilled=juce::jmin(summaryFilled+1,summaryCapacity);}
        else {auto& s=summaryHistory[size_t((summaryCursor+summaryCapacity-1)%summaryCapacity)];s.keyLo=juce::jmin(s.keyLo,v.keyLo);s.keyHi=juce::jmax(s.keyHi,v.keyHi);s.outLo=juce::jmin(s.outLo,v.outLo);s.outHi=juce::jmax(s.outHi,v.outHi);s.gain=juce::jmin(s.gain,v.gain);s.time=v.time;}
        if(1.f-v.gain-currentReduction>.035f){const auto stamp=juce::Time::getMillisecondCounterHiRes();if(triggerStamp<0||stamp-triggerStamp>180)triggerStamp=stamp;}
        signalPeak=juce::jmax(std::abs(v.keyLo),std::abs(v.keyHi),std::abs(v.outLo),std::abs(v.outHi));currentReduction=1.f-v.gain;
        if(signalPeak>.004f||currentReduction>.001f)lastVisibleSignalTime=v.time;
        fresh=true;
    }
    if(filled>0){
        // Display clock: runs on wall-clock time and is gently steered towards the
        // newest audio timestamp minus a small safety lag. Audio arrives in bursts
        // (one block at a time); following it directly made the scroll speed uneven.
        const double latest=history[size_t((cursor+historyCapacity-1)%historyCapacity)].time;
        const double clock=juce::Time::getMillisecondCounterHiRes()*.001;
        const double dt=lastClock>0.?juce::jlimit(0.,.1,clock-lastClock):0.;lastClock=clock;
        if(fresh){if(lastLatest>0.)gapMax=juce::jmax(latest-lastLatest,gapMax*.995);lastLatest=latest;}
        const double target=latest-juce::jlimit(.015,.15,gapMax*.5+.01);
        if(displayTime<=0.||std::abs(target-displayTime)>.4)displayTime=target;
        else{displayTime+=dt;displayTime+=(target-displayTime)*.06;}
        displayTime=juce::jmin(displayTime,latest);
    }
    influence.setMeter(currentReduction,signalPeak);outputGain.setMeter(0,signalPeak);duration.setMeter(0,0);
    const bool target=audioProcessor.parameters.getRawParameterValue("bypass")->load()>.5f||audioProcessor.displayBypass.load();
    if(target!=bypassTarget){
        bypassTarget=target;
        for(auto* c:{static_cast<juce::Component*>(&sidechainRange),static_cast<juce::Component*>(&processingRange),static_cast<juce::Component*>(&midSide)})c->setEnabled(!target);
        if(target){captureBlurSnapshot();bypassMix=1;}else{bypassMix=0;blurredSnapshot=juce::Image();}
        repaint();return;
    }
    // Repaint the two plot rectangles only, and skip a frozen graph entirely.
    if(bypassMix>0)return;
    const bool moved=std::abs(displayTime-lastPaintedTime)>1e-7;
    if(!fresh&&!moved)return;
    if(fresh&&signalPeak<=.004f&&currentReduction<=.001f&&displayTime-lastVisibleSignalTime>gainWindow+.2)return;
    lastPaintedTime=displayTime;
    if(!gainFrozen)repaint(gainArea);
    if(!scopeFrozen)repaint(scopeArea);
}
