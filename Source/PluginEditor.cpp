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
void PocketLook::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hover,bool down){
    if(button.getButtonText()=="expand")return;
    const auto t=tokens();auto r=button.getLocalBounds().toFloat().reduced(1);
    if(button.getButtonText()=="freeze")r=r.withSizeKeepingCentre(r.getWidth()*20.2746f/32.f,r.getHeight()*20.2746f/32.f);
    g.setColour(hover?t.raised.brighter(.12f):t.raised);g.fillRoundedRectangle(r,2.5f);
    g.setColour(down?t.ink:t.out.withAlpha(.75f));g.drawRoundedRectangle(r,2.5f,1.2f);
}
void PocketLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool){
    auto r=b.getLocalBounds().toFloat();auto name=b.getButtonText();
    if(name=="freeze")r=r.withSizeKeepingCentre(r.getWidth()*20.2746f/32.f,r.getHeight()*20.2746f/32.f);
    const auto controlInk=isDark()?tokens().out:ink();
    auto c=r.getCentre();float s=juce::jmin(r.getWidth(),r.getHeight())/40;juce::Path p;
    if(name=="expand"){
        const float direction=b.getToggleState()?-1.f:1.f;
        p.startNewSubPath(c.x-10*s,c.y-3*s*direction);p.lineTo(c.x,c.y+3*s*direction);p.lineTo(c.x+10*s,c.y-3*s*direction);juce::Path outline;juce::PathStrokeType(1.7f*s).createStrokedPath(outline,p);
        juce::DropShadow(ink().withAlpha(.30f),7,{0,0}).drawForPath(g,outline);
        juce::DropShadow(ink().withAlpha(.55f),3,{0,0}).drawForPath(g,outline);
        stroke(g,p,ink().brighter(.2f),1.7f*s);return;
    }
    if(name=="power"){p.addCentredArc(0,1,8,8,0,.65f,juce::MathConstants<float>::twoPi-.65f,true);p.startNewSubPath(0,-10);p.lineTo(0,-1);p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,controlInk,1.7f*s);return;}

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
        stroke(g,p,(b.getToggleState()?accent():controlInk).withAlpha(b.isEnabled()?1.f:.35f),1.35f*s);return;
    }
    constexpr int teeth=10;for(int i=0;i<teeth*4;++i){float a=float(i)*juce::MathConstants<float>::twoPi/float(teeth*4)-juce::MathConstants<float>::halfPi;float radius=(i%4==1||i%4==2)?10.f:7.7f;auto pt=juce::Point<float>(std::cos(a)*radius,std::sin(a)*radius);if(i==0)p.startNewSubPath(pt);else p.lineTo(pt);}p.closeSubPath();p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,controlInk,1.65f*s);g.setColour(controlInk);g.drawEllipse(c.x-3.2f*s,c.y-3.2f*s,6.4f*s,6.4f*s,1.65f*s);
}
void PocketLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float minPos,float maxPos,juce::Slider::SliderStyle style,juce::Slider& slider){
    auto t=tokens();
    const float scale=float(slider.getWidth())/670.f;
    const float cy=float(y)+float(h)*.5f,left=style==juce::Slider::TwoValueHorizontal?minPos:float(x),right=style==juce::Slider::TwoValueHorizontal?maxPos:pos;
    const auto light=theme==PocketTheme::SolidDark?juce::Colour(0xffe2f5f8):(slider.getName()=="key"?t.key:t.out);
    g.setColour(t.glass.darker(.6f));g.fillRoundedRectangle(float(x),cy-4*scale,float(w),8*scale,3*scale);
    g.setColour(t.border.withAlpha(.6f));g.drawLine(float(x),cy+4*scale,float(x+w),cy+4*scale,.6f*scale);
    g.setColour(light.withAlpha(.18f));g.fillRoundedRectangle(left,cy-6*scale,juce::jmax(.1f,right-left),12*scale,4*scale);
    g.setColour(light);g.fillRect(left,cy-2.5f*scale,juce::jmax(.1f,right-left),5*scale);
    auto thumb=[&](float px){auto r=juce::Rectangle<float>(13*scale,13*scale).withCentre({px,cy});
        g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(r.translated(scale,2*scale),2*scale);
        g.setGradientFill(juce::ColourGradient(t.out.interpolatedWith(t.raised,.6f),r.getX(),r.getY(),t.glass,r.getRight(),r.getBottom(),false));g.fillRoundedRectangle(r,2*scale);
        g.setColour(t.ink.withAlpha(.25f));g.drawRoundedRectangle(r,2*scale,.65f*scale);};
    if(style==juce::Slider::TwoValueHorizontal){thumb(minPos);thumb(maxPos);}else thumb(pos);
}
ModernDial::ModernDial(PocketLook& l,juce::String t,juce::String sub,juce::String u,juce::uint32 a,bool inf,bool infMin,bool compactDial,juce::String infLabel):look(l),title(t),subtitle(sub),unit(u),infinity(inf),compact(compactDial){juce::ignoreUnused(a,infMin,infLabel);setSliderStyle(juce::Slider::RotaryVerticalDrag);setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);setName(t);setWantsKeyboardFocus(true);}
void ModernDial::paint(juce::Graphics& g){
    const auto t=look.tokens();const float designWidth=compact?132.f:240.f,designHeight=compact?135.f:250.f;
    const float factor=float(getWidth())/designWidth;
    const float scale=g.getInternalContext().getPhysicalPixelScaleFactor();
    const int pw=juce::jmax(1,juce::roundToInt(getWidth()*scale)),ph=juce::jmax(1,juce::roundToInt(getHeight()*scale));
    const juce::Point<float> centre=compact?juce::Point<float>{66.26f,75.40f}:juce::Point<float>{119.27f,135.27f};
    const float radius=compact?49.68f:97.5f,faceRadius=compact?31.34f:60.f;
    if(!body.isValid()||body.getWidth()!=pw||body.getHeight()!=ph||bodyTheme!=look.theme||std::abs(bodyScale-scale)>.001f){
        body=juce::Image(juce::Image::ARGB,pw,ph,true,juce::SoftwareImageType());bodyTheme=look.theme;bodyScale=scale;
        ringValue=std::numeric_limits<double>::quiet_NaN();juce::Graphics bg(body);bg.addTransform(juce::AffineTransform::scale(scale*factor));
        if(look.theme==PocketTheme::SolidDark){
            static const auto large=juce::ImageFileFormat::loadFrom(BinaryData::DialLarge_png,BinaryData::DialLarge_pngSize);
            static const auto small=juce::ImageFileFormat::loadFrom(BinaryData::DialSmall_png,BinaryData::DialSmall_pngSize);
            bg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);bg.drawImage(compact?small:large,{0,0,designWidth,designHeight},juce::RectanglePlacement::stretchToFit);
            // Cool, deeper material grade is cached with the SVG-derived body.
            bg.setGradientFill(juce::ColourGradient(juce::Colour(0xff142837).withAlpha(.16f),centre.x-radius,centre.y-radius,juce::Colour(0xff020b13).withAlpha(.40f),centre.x+radius,centre.y+radius,false));
            bg.fillEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius);
        }else{
            auto rim=juce::Rectangle<float>(2*radius,2*radius).withCentre(centre);
            bg.setGradientFill(juce::ColourGradient(t.raised.brighter(.2f),centre.x-radius,centre.y-radius,t.glass.darker(.15f),centre.x+radius,centre.y+radius,false));bg.fillEllipse(rim);
            bg.setColour(t.glass.darker(.5f));bg.drawEllipse(rim,compact?5.f:6.f);
            auto face=juce::Rectangle<float>(2*faceRadius,2*faceRadius).withCentre(centre);
            bg.setGradientFill(juce::ColourGradient(t.glass,centre.x-faceRadius,centre.y-faceRadius,t.raised,centre.x+faceRadius,centre.y+faceRadius,false));bg.fillEllipse(face);
            bg.setColour(t.border);bg.drawEllipse(face,1.5f);
        }
        const char* data=title=="Influence"?BinaryData::TitleInfluence_svg:(title=="Duration"?BinaryData::TitleDuration_svg:(title=="Output"?BinaryData::TitleOutput_svg:BinaryData::TitleMS_svg));
        const int length=title=="Influence"?BinaryData::TitleInfluence_svgSize:(title=="Duration"?BinaryData::TitleDuration_svgSize:(title=="Output"?BinaryData::TitleOutput_svgSize:BinaryData::TitleMS_svgSize));
        heading=juce::Drawable::createFromImageData(data,size_t(length));
        if(heading){heading->replaceColour(juce::Colour(0xffced6e2),t.ink);heading->draw(bg,1.f);}
    }
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImageTransformed(body,juce::AffineTransform::scale(1.f/bodyScale));
    // Cache the blurred ring independently; audio-driven metering never rebuilds it.
    const float proportion=float(valueToProportionOfLength(getValue()));
    if(!ringImage.isValid()||ringValue!=double(proportion)||ringImage.getWidth()!=pw||ringImage.getHeight()!=ph){
        ringValue=double(proportion);ringImage=juce::Image(juce::Image::ARGB,pw,ph,true,juce::SoftwareImageType());juce::Graphics rg(ringImage);rg.addTransform(juce::AffineTransform::scale(scale*factor));
        const float start=juce::MathConstants<float>::pi,end=start+juce::MathConstants<float>::twoPi*proportion;
        const auto colour=look.theme==PocketTheme::SolidDark?juce::Colour(0xfff5f8ff):(title=="Influence"||title=="Output"?t.out:t.neutral);
        juce::Path arc;
        if(proportion>=.99999f)arc.addEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius);
        else if(proportion>0)arc.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
        // Rasterised once per value/size/theme, no idle glow animation.
        if(!arc.isEmpty()){
            juce::Path outline;juce::PathStrokeType(compact?4.2f:4.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded).createStrokedPath(outline,arc);
            juce::DropShadow(colour.withAlpha(.5f),14,{0,0}).drawForPath(rg,outline);
            juce::DropShadow(colour.withAlpha(.9f),5,{0,0}).drawForPath(rg,outline);
            stroke(rg,arc,colour,compact?4.2f:4.6f);
        }
        const float inner=faceRadius+8.f,outer=inner+(compact?9.f:16.f);
        rg.setColour(t.ink.withAlpha(.75f));rg.drawLine(centre.x+inner*std::sin(end),centre.y-inner*std::cos(end),centre.x+outer*std::sin(end),centre.y-outer*std::cos(end),1.2f);
    }
    g.drawImageTransformed(ringImage,juce::AffineTransform::scale(1.f/bodyScale));
    juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(factor));
    if(emphasis>.001f){g.setColour(t.ink.withAlpha(emphasis*.10f));g.drawEllipse(centre.x-faceRadius,centre.y-faceRadius,2*faceRadius,2*faceRadius,1.f);}
    if(title=="Influence"&&gr>.001f){juce::Path meter;meter.addCentredArc(centre.x,centre.y,radius-7,radius-7,0,juce::MathConstants<float>::pi,juce::MathConstants<float>::pi+juce::MathConstants<float>::twoPi*gr,true);stroke(g,meter,t.out.withAlpha(.55f),1.f);}
    const auto value=displayedValue();
    g.setFont(pocketFont(valueTextHeight(value)/factor));g.setColour(t.ink);
    g.drawText(value,juce::Rectangle<float>{centre.x-faceRadius,centre.y-(compact?15.f:19.f),2*faceRadius,compact?25.f:35.f},juce::Justification::centred);
    const auto detail=unit=="balance"?balanceLabel():(unit=="dB"?juce::String("dB"):(isAutoValue()?juce::String("AUTO"):subtitle));
    g.setFont(pocketFont(compact?11.f:13.f));g.setColour(t.muted);g.drawText(detail,juce::Rectangle<float>{centre.x-faceRadius,centre.y+(compact?8.f:17.f),2*faceRadius,20.f},juce::Justification::centred);
}

DuckPocketAudioProcessorEditor::DuckPocketAudioProcessorEditor(DuckPocketAudioProcessor& p):AudioProcessorEditor(&p),audioProcessor(p){
    juce::PropertiesFile::Options o;o.applicationName="DuckPocket";o.filenameSuffix="settings";o.folderName="RainlineMusic";o.osxLibrarySubFolder="Application Support";preferences=std::make_unique<juce::PropertiesFile>(o);
    // Solid Dark is the default theme; a stored preference still wins.
    auto saved=preferences->getValue("duckPocket.ui.theme",preferences->getValue("phasePocket.ui.theme","solidDark"));setTheme(saved=="neon"?PocketTheme::Neon:(saved=="amber"?PocketTheme::Amber:PocketTheme::SolidDark),false);
    gainWindow=preferences->getDoubleValue("duckPocket.ui.graphWindow",preferences->getDoubleValue("duckPocket.ui.gainWindow",preferences->getDoubleValue("phasePocket.ui.gainWindow",1.)));scopeWindow=gainWindow;
    setLookAndFeel(&look);setOpaque(true);setResizable(true,true);
    for(auto* c:std::initializer_list<juce::Component*>{&influence,&duration,&outputGain,&sidechainRange,&processingRange,&midSide,&settingsButton,&bypassButton,&freezeButton,&expandButton})addAndMakeVisible(c);
    influenceAttach=std::make_unique<SliderAttachment>(p.parameters,"amount",influence);durationIsRelative=p.parameters.getRawParameterValue("relativeDuration")->load()>.5f;duration.setDurationMode(durationIsRelative);durationAttach=std::make_unique<SliderAttachment>(p.parameters,durationIsRelative?"durationPercent":"duration",duration);duration.setDoubleClickReturnValue(true,durationIsRelative?100:2000);outputAttach=std::make_unique<SliderAttachment>(p.parameters,"outputGain",outputGain);outputGain.setDoubleClickReturnValue(true,0);influence.setDoubleClickReturnValue(true,100);midSide.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);midSide.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);midSide.setDoubleClickReturnValue(true,0);msAttach=std::make_unique<SliderAttachment>(p.parameters,"msBalance",midSide);midSide.onValueChange=[this]{midSide.repaint();};
    bypassButton.setClickingTogglesState(true);bypassAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bypass",bypassButton);settingsButton.onClick=[this]{showSettingsMenu();};
    // One button freezes and resumes both graphs at once.
    freezeButton.setClickingTogglesState(true);freezeButton.setTooltip("Freeze both graphs");
    freezeButton.onClick=[this]{setFrozen(freezeButton.getToggleState());};
    auto setupRange=[](juce::Slider& s){s.setSliderStyle(juce::Slider::TwoValueHorizontal);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setRange(20,20000,0.);};setupRange(sidechainRange);setupRange(processingRange);sidechainRange.setName("key");processingRange.setName("out");midSide.setName("neutral");
    lowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scLow"),[this](float){syncRange();});highAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scHigh"),[this](float){syncRange();});lowAttach->sendInitialUpdate();highAttach->sendInitialUpdate();
    sidechainRange.onDragStart=[this]{rangeGesture=true;lowAttach->beginGesture();highAttach->beginGesture();};sidechainRange.onValueChange=[this]{float a=float(sidechainRange.getMinValue()),b=float(sidechainRange.getMaxValue());repaint(scaled(48,792,704,48));if(rangeGesture){lowAttach->setValueAsPartOfGesture(a);highAttach->setValueAsPartOfGesture(b);}else{lowAttach->setValueAsCompleteGesture(a);highAttach->setValueAsCompleteGesture(b);}};sidechainRange.onDragEnd=[this]{lowAttach->endGesture();highAttach->endGesture();rangeGesture=false;syncRange();};sidechainRange.onResetMin=[this]{lowAttach->setValueAsCompleteGesture(20);syncRange();};sidechainRange.onResetMax=[this]{highAttach->setValueAsCompleteGesture(20000);syncRange();};
    processLowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processLow"),[this](float){syncProcessingRange();});processHighAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processHigh"),[this](float){syncProcessingRange();});processLowAttach->sendInitialUpdate();processHighAttach->sendInitialUpdate();
    processingRange.onDragStart=[this]{processRangeGesture=true;processLowAttach->beginGesture();processHighAttach->beginGesture();};processingRange.onValueChange=[this]{float a=float(processingRange.getMinValue()),b=float(processingRange.getMaxValue());repaint(scaled(48,839,704,48));if(processRangeGesture){processLowAttach->setValueAsPartOfGesture(a);processHighAttach->setValueAsPartOfGesture(b);}else{processLowAttach->setValueAsCompleteGesture(a);processHighAttach->setValueAsCompleteGesture(b);}};processingRange.onDragEnd=[this]{processLowAttach->endGesture();processHighAttach->endGesture();processRangeGesture=false;syncProcessingRange();};processingRange.onResetMin=[this]{processLowAttach->setValueAsCompleteGesture(20);syncProcessingRange();};processingRange.onResetMax=[this]{processHighAttach->setValueAsCompleteGesture(20000);syncProcessingRange();};
    duration.setTooltip("Key length: 100% = AUTO; shorter percentages use the last measured event. First event uses AUTO. Legacy sessions retain milliseconds.");outputGain.setTooltip("Output gain: -12 to +6 dB; double-click resets to 0 dB");influence.setTooltip("Ducking depth");bypassButton.setTooltip("Enable / bypass processing");settingsButton.setTooltip("Settings");sidechainRange.setTooltip("Detector filter, 20 Hz to 20 kHz; Alt-click or double-click resets the nearest handle");midSide.setTooltip("0% MS = neutral; turn left for 0-100% MID, right for 0-100% SIDE. Double-click resets. Mono input contains Mid only");processingRange.setTooltip("Frequency range affected by ducking; 20 Hz to 20 kHz keeps the plain wideband duck");
    filtersExpanded=preferences->getBoolValue("duckPocket.ui.filtersExpanded.v2",false);
    expandButton.setClickingTogglesState(true);expandButton.setToggleState(filtersExpanded,juce::dontSendNotification);expandButton.setTooltip("Show / hide Sidechain Filter and Processing Range");expandButton.onClick=[this]{setFiltersExpanded(expandButton.getToggleState());};
    int width=p.editorWidth.load();
    if(width<400||width>1500)width=preferences->getIntValue("duckPocket.ui.compactWidth",615);
    width=juce::jlimit(400,1500,width);
    setResizeLimits(400,juce::roundToInt(designHeight()*.5f),1500,juce::roundToInt(designHeight()*1.875f));getConstrainer()->setFixedAspectRatio(800./designHeight());setSize(width,juce::roundToInt(width*designHeight()/800.f));
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
void DuckPocketAudioProcessorEditor::saveSize(){if(!ready||!preferences)return;audioProcessor.editorWidth.store(getWidth());preferences->setValue("duckPocket.ui.compactWidth",getWidth());preferences->saveIfNeeded();resizeStamp=0;}
void DuckPocketAudioProcessorEditor::invalidateChrome(){chromeValid=false;repaint();}
void DuckPocketAudioProcessorEditor::setTheme(PocketTheme t,bool persist){if(t==PocketTheme::SolidWhite)t=PocketTheme::SolidDark;look.theme=t;if(persist&&preferences){preferences->setValue("duckPocket.ui.theme",t==PocketTheme::Neon?"neon":(t==PocketTheme::Amber?"amber":(t==PocketTheme::SolidDark?"solidDark":"solidDark")));preferences->saveIfNeeded();}chromeValid=false;for(auto& layer:softwarePlots)layer.reset();
#if DUCK_ENABLE_OPENGL
    for(auto& phosphor:gpuPhosphor)phosphor.reset();
#endif
    repaint();for(auto* c:getChildren())c->repaint();if(bypassMix>0)juce::MessageManager::callAsync([safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this)]{if(safe)safe->captureBlurSnapshot();});}
void DuckPocketAudioProcessorEditor::setHistoryWindow(double seconds){gainWindow=scopeWindow=seconds;preferences->setValue("duckPocket.ui.graphWindow",seconds);preferences->saveIfNeeded();invalidateChrome();}
// A single snowflake button freezes and resumes both graphs together.
void DuckPocketAudioProcessorEditor::setFrozen(bool frozen){
    triggerStamp=-1;gainFrozen=scopeFrozen=frozen;frozenGain.clear();frozenSummary.clear();
    for(auto& layer:softwarePlots)layer.reset();
#if DUCK_ENABLE_OPENGL
    for(auto& phosphor:gpuPhosphor)phosphor.reset();
#endif
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
void DuckPocketAudioProcessorEditor::showSettingsMenu(){juce::PopupMenu root,window,theme;for(size_t i=0;i<windows.size();++i)window.addItem(int(i)+1,timeLabel(windows[i]),true,std::abs(gainWindow-windows[i])<1e-6);theme.addItem(201,"Neon",true,look.theme==PocketTheme::Neon);theme.addItem(204,"Amber",true,look.theme==PocketTheme::Amber);theme.addItem(202,"Solid Dark",true,look.theme==PocketTheme::SolidDark);root.addSubMenu("Graph window",window);root.addSeparator();root.addSubMenu("Theme",theme);
#if DUCK_ENABLE_OPENGL && ! JUCE_WINDOWS
root.addSeparator();root.addItem(401,"OpenGL (experimental)",true,glowRenderer!=nullptr);
#endif
root.addSeparator();root.addItem(301,"Percentage Duration",true,durationIsRelative);auto safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this);root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton),[safe](int id){if(!safe||id==0)return;
#if DUCK_ENABLE_OPENGL && ! JUCE_WINDOWS
if(id==401){safe->setOpenGL(safe->glowRenderer==nullptr);return;}
#endif
if(id>=1&&id<=6)safe->setHistoryWindow(windows[size_t(id-1)]);else if(id==301){auto* prm=safe->audioProcessor.parameters.getParameter("relativeDuration");prm->beginChangeGesture();prm->setValueNotifyingHost(safe->durationIsRelative?0.f:1.f);prm->endChangeGesture();safe->syncDurationMode();}else if(id==201)safe->setTheme(PocketTheme::Neon);else if(id==204)safe->setTheme(PocketTheme::Amber);else if(id==202)safe->setTheme(PocketTheme::SolidDark);});}
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
void DuckPocketAudioProcessorEditor::syncRange(){if(rangeGesture)return;float a=paramValue(audioProcessor.parameters,"scLow"),b=paramValue(audioProcessor.parameters,"scHigh");sidechainRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(48,792,704,48));}
void DuckPocketAudioProcessorEditor::syncProcessingRange(){if(processRangeGesture)return;float a=paramValue(audioProcessor.parameters,"processLow"),b=paramValue(audioProcessor.parameters,"processHigh");processingRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(48,839,704,48));}
juce::Rectangle<int> DuckPocketAudioProcessorEditor::scaled(float x,float y,float w,float h) const {float s=float(getWidth())/800;return{juce::roundToInt(x*s),juce::roundToInt(y*s),juce::roundToInt(w*s),juce::roundToInt(h*s)};}
void DuckPocketAudioProcessorEditor::resized(){
    // Visual freeze bounds: (1830,1693,48,48) mapped from the 1894px reference.
    // A larger transparent hit box preserves clickability at compact sizes.
    freezeButton.setBounds(scaled(766.98f,709.10f,32,32));settingsButton.setBounds(scaled(716,12,31,31));bypassButton.setBounds(scaled(755,12,31,31));
    influence.setBounds(scaled(60,98.71f,240,250));duration.setBounds(scaled(503,98.71f,240,250));
    outputGain.setBounds(scaled(334,88.70f,132,135));midSide.setBounds(scaled(334,227.70f,132,135));
    expandButton.setBounds(scaled(320,753,160,36));
    sidechainRange.setVisible(filtersExpanded);processingRange.setVisible(filtersExpanded);
    sidechainRange.setBounds(scaled(65,815,670,28));processingRange.setBounds(scaled(65,862,670,28));
    blurArea=scaled(0,56,800,designHeight()-56).getIntersection(getLocalBounds());gainArea=scaled(32,396,752,160);scopeArea=scaled(32,583,752,160);
    blurredSnapshot={};if(ready){
        audioProcessor.editorWidth.store(getWidth());
        // Height-only folds need no cache debounce. Real width changes still
        // invalidate chrome so the idle VBlank tick completes the delayed rebuild.
        if(chrome.isValid()&&chrome.getWidth()!=juce::roundToInt(getWidth()*chromeScale)){
            chromeValid=false;resizeStamp=juce::Time::getMillisecondCounterHiRes();
        }
    }
}
void DuckPocketAudioProcessorEditor::setFiltersExpanded(bool expanded,bool persist){
    if(filtersExpanded==expanded)return;
    filtersExpanded=expanded;expandButton.setToggleState(expanded,juce::dontSendNotification);
    if(persist&&preferences)preferences->setValue("duckPocket.ui.filtersExpanded.v2",expanded);
    // Do not constrain the old collapsed bounds against the new minimum height:
    // setResizeLimits would transiently change the width before setSize (especially
    // at 400px), rebuilding both dials/plots. Install limits, then resize once.
    const int width=getWidth();auto* limits=getConstrainer();
    limits->setSizeLimits(400,juce::roundToInt(designHeight()*.5f),1500,juce::roundToInt(designHeight()*1.875f));
    limits->setFixedAspectRatio(800./designHeight());
    setSize(width,juce::roundToInt(width*designHeight()/800.f));repaint();
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
    const juce::Rectangle<float> plot(box.getX()+18,box.getY()+27,box.getWidth()-52,box.getHeight()-53);
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
    auto emit=[&](const juce::Path& path,juce::Colour colour,float width,float strength=1.f){
        colour=colour.withMultipliedAlpha(strength);
        if(emissionGraphics){emissionGraphics->setGradientFill(juce::ColourGradient(colour.withAlpha(.04f),plot.getX(),0,colour,plot.getRight(),0,false));emissionGraphics->strokePath(path,juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
    };
    juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(plot.toNearestInt());
    // Column count must track *physical* pixels, not the fixed 800-wide design
    // rect. `g` already carries the editor's own upscale transform (paint()
    // applies getWidth()/800 before calling us) plus the OS/Retina device
    // scale, so getPhysicalPixelScaleFactor() gives the true logical-to-device
    // ratio in one number. Without this the trace was always built from ~600
    // points (the design width) and then stretched to however big the window
    // or display scale actually was, which is what made it look chunky/low-res
    // once resized above ~800px or viewed on a HiDPI screen.
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
            g.setGradientFill(juce::ColourGradient(label.withAlpha(.10f),0,plot.getY(),label.withAlpha(.43f),0,plot.getBottom(),false));g.fillPath(fill);
            juce::Path path;if(longWindow){path.startNewSubPath(pathPoints.front());for(size_t i=1;i<pathPoints.size();++i)path.lineTo(pathPoints[i]);}else path=smoothPath(pathPoints);
            if(!emissionGraphics&&glow&&signalPeak>.004f&&!frozen)stroke(g,path,label.withAlpha(.10f),4.2f);
            g.setGradientFill(juce::ColourGradient(label.brighter(.35f).withAlpha(.08f),plot.getX(),0,label.brighter(.35f),plot.getRight(),0,false));g.strokePath(path,juce::PathStrokeType(1.65f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));float depth=0;for(float value:bucketLo)if(value<=1.f)depth=juce::jmax(depth,1.f-value);emit(path,label.brighter(.35f),5.f,juce::jlimit(0.f,1.f,depth));
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
            g.setGradientFill(juce::ColourGradient(colour.withAlpha(.035f),plot.getX(),0,colour.brighter(.18f).withAlpha(.65f),plot.getRight(),0,false));g.fillPath(body);g.setGradientFill(juce::ColourGradient(colour.withAlpha(.07f),plot.getX(),0,colour.brighter(.35f),plot.getRight(),0,false));g.strokePath(body,juce::PathStrokeType(1.1f));float strength=0;for(int column=juce::jmax(0,juce::roundToInt((x0-plot.getX())/span));column<columns&&xOf(column)<=x1;++column)if(bucketHi[size_t(column)]>=bucketLo[size_t(column)])strength=juce::jmax(strength,std::abs(bucketLo[size_t(column)]),std::abs(bucketHi[size_t(column)]));emit(body,colour.brighter(.35f),4.f,juce::jlimit(0.f,1.f,strength));
        }
    }
}
void DuckPocketAudioProcessorEditor::paintChrome(juce::Graphics& g){
    const float physicalScale=juce::jlimit(.75f,4.f,g.getInternalContext().getPhysicalPixelScaleFactor());
    const int w=juce::jmax(1,juce::roundToInt(getWidth()*physicalScale)),h=juce::jmax(1,juce::roundToInt(expandedDesignHeight*float(getWidth())/800.f*physicalScale));
    const bool resizing=resizeStamp>0&&juce::Time::getMillisecondCounterHiRes()-resizeStamp<100;
    if((!chromeValid||!chrome.isValid()||chrome.getWidth()!=w||chrome.getHeight()!=h||std::abs(chromeScale-physicalScale)>.001f)&&(!resizing||!chrome.isValid())){
        ++chromeBuildCount;chrome=juce::Image(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());chromeScale=physicalScale;juce::Graphics cg(chrome);cg.addTransform(juce::AffineTransform::scale(physicalScale*float(getWidth())/800.f));const auto t=look.tokens();
        cg.setGradientFill(juce::ColourGradient(t.chassis,400,240,t.chassis.darker(.2f),0,expandedDesignHeight,true));cg.fillRect(0.f,0.f,800.f,expandedDesignHeight);
        juce::Random noise(0xD0C);for(int i=0;i<6500;++i){cg.setColour((i%2?juce::Colours::white:juce::Colours::black).withAlpha(.012f));cg.fillRect(float(noise.nextInt(800)),float(noise.nextInt(int(expandedDesignHeight))),1.f,1.f);}
        auto logo=juce::Drawable::createFromImageData(BinaryData::Logo_svg,BinaryData::Logo_svgSize);
        if(logo){logo->replaceColour(juce::Colour(0xffced6e2),t.brand);logo->drawAt(cg,11.0264f,5.2672f,1.f);}
        for(float y:{53.f,390.f,556.f,581.f,744.f}){
            cg.setColour(juce::Colours::black.withAlpha(.5f));cg.fillRect(0.f,y,800.f,3.f);
            cg.setColour(t.ink.withAlpha(.05f));cg.drawLine(0,y+3,800,y+3,.7f);
        }
        auto frame=[&](juce::Rectangle<float> box,bool gain){
            auto plot=juce::Rectangle<float>(box.getX()+18,box.getY()+27,box.getWidth()-52,box.getHeight()-53);
            const auto recess=juce::Rectangle<float>{0,box.getY(),800,box.getHeight()};
            const auto glass=t.glass.darker(.3f);
            cg.setGradientFill(juce::ColourGradient(glass.brighter(.035f),0,recess.getY(),glass.darker(.18f),0,recess.getBottom(),false));cg.fillRect(recess);
            // Cached inner bevel: the graph bed sits below the chassis.
            cg.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.7f),0,recess.getY(),juce::Colours::transparentBlack,0,recess.getY()+14,false));cg.fillRect(recess.withHeight(14));
            cg.setColour(t.ink.withAlpha(.075f));cg.drawLine(0,recess.getBottom()-.5f,800,recess.getBottom()-.5f,.7f);
            cg.setGradientFill(juce::ColourGradient(t.glass,plot.getX(),plot.getY(),glass,plot.getX(),plot.getBottom(),false));cg.fillRect(plot.expanded(8,0));
            text(cg,gain?"GAIN HISTORY":"OSCILLOSCOPE",{box.getX()+18,box.getY()+4,240,22},13,t.ink);
            if(gain)text(cg,"GAIN REDUCTION",{box.getRight()-220,box.getY()+4,188,22},11,t.muted,juce::Justification::centredRight);
            else {text(cg,"OUT",{box.getRight()-86,box.getY()+4,35,22},11,t.out);text(cg,"KEY",{box.getRight()-48,box.getY()+4,35,22},11,t.key);}
            const float top=plot.getY(),bottom=plot.getBottom(),midY=plot.getCentreY(),height=plot.getHeight(),left=plot.getX(),right=plot.getRight(),endS=height*.10f,ringS=height*.04f;
            auto arc=[&](float cx,float bend){juce::Path path;path.startNewSubPath(cx,top);path.quadraticTo(cx+2*bend,midY,cx,bottom);stroke(cg,path,t.major,.8f);};
            arc(left,-endS);arc(left,endS);arc(right,endS);arc(right,-endS);arc(left+plot.getWidth()*.25f,ringS);arc(left+plot.getWidth()*.75f,-ringS);
            cg.setColour(t.major);cg.drawLine(plot.getCentreX(),top,plot.getCentreX(),bottom,.8f);cg.drawLine(left,top,right,top,.8f);cg.drawLine(left,bottom,right,bottom,.8f);
            for(int i=1;i<4;++i){const float u=float(i)/4.f,y=top+u*height,offset=4*endS*u*(1-u);cg.setColour(t.major);cg.drawLine(left+offset,y,right-offset,y,.8f);}
            cg.setGradientFill(juce::ColourGradient(t.glass.withAlpha(.22f),left,midY,t.glass.withAlpha(0.f),right,midY,false));cg.fillRect(plot);
            text(cg,gain?"100%":"+1",{right+7,top-9,43,18},11,t.muted);
            text(cg,gain?"0%":"-1",{right+7,bottom-9,43,18},11,t.muted);
            const double window=gain?gainWindow:scopeWindow;text(cg,"-"+timeLabel(window),{left,box.getBottom()-24,140,20},11,t.muted);text(cg,"NOW",{right-80,box.getBottom()-24,80,20},11,t.muted,juce::Justification::centredRight);
        };
        frame({32,396,752,160},true);frame({32,583,752,160},false);
        chromeValid=true;
    }
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
#if DUCK_ENABLE_OPENGL
    juce::Graphics::ScopedSaveState clip(g);
    if(glowRenderer&&glowRenderer->ready.load()&&glowRenderer->presented.load()&&!capturingBlur&&bypassMix<.5f){
        // Component painting overlays OpenGL. Transparent plot apertures expose
        // the GPU-rendered glass/grid/glow; crisp CPU cores remain above it.
        g.excludeClipRegion(scaled(50,423,700,107));g.excludeClipRegion(scaled(50,610,700,107));
    }
#endif
    // Keep one expanded-height chrome cache. Folding clips the bottom; it never
    // stretches the graph bed or regenerates top materials at the same width/DPI.
    // Use the cached image's actual pixel aspect, avoiding an extra fractional
    // Y scale on CoreGraphics when width*905/800 rounds to physical pixels.
    g.fillAll(look.tokens().chassis);
    g.drawImage(chrome,{0,0,float(getWidth()),float(chrome.getHeight())*float(getWidth())/float(chrome.getWidth())},juce::RectanglePlacement::stretchToFit);
}

void DuckPocketAudioProcessorEditor::paintDynamicLabels(juce::Graphics& g){
    for(int i=0;i<2;++i){auto& range=i?processingRange:sidechainRange;const float y=i?839.f:792.f;
        text(g,i?"Processing Range":"Sidechain Filter",{230,y,340,26},15,look.ink(),juce::Justification::centred);
        text(g,hz(range.getMinValue()),{65,y,160,26},14,look.ink());
        text(g,hz(range.getMaxValue()),{575,y,160,26},14,look.ink(),juce::Justification::centredRight);
    }
}

void DuckPocketAudioProcessorEditor::paint(juce::Graphics& g){
    ++paintCount;paintChrome(g);juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(float(getWidth())/800));

#if DUCK_ENABLE_OPENGL
    if(glowRenderer&&glowRenderer->ready.load()&&!capturingBlur&&bypassMix<.5f){
        if(g.clipRegionIntersects({50,423,700,107})||g.clipRegionIntersects({50,610,700,107})){
        auto frame=std::make_shared<PocketGlowRenderer::Frame>();frame->width=getWidth();frame->height=getHeight();frame->chromeRevision=chromeBuildCount;
        for(int i=0;i<2;++i){const float y=i?583.f:396.f;const juce::Rectangle<float> plot(50,y+27,700,107);auto& layer=frame->plots[size_t(i)];
            if(!g.clipRegionIntersects(plot.toNearestInt())&&lastGpuFrame&&lastGpuFrame->chromeRevision==frame->chromeRevision){layer=lastGpuFrame->plots[size_t(i)];continue;}
            layer.bounds=scaled(50,y+27,700,107);
            const float device=float(chrome.getWidth())/float(getWidth());auto crop=(layer.bounds.toFloat()*device).toNearestInt().getIntersection(chrome.getBounds());layer.background=chrome.getClippedImage(crop);
            const float raster=g.getInternalContext().getPhysicalPixelScaleFactor()*.5f;
            layer.emission=juce::Image(juce::Image::ARGB,juce::jmax(1,juce::roundToInt(plot.getWidth()*raster)),juce::jmax(1,juce::roundToInt(plot.getHeight()*raster)),true,juce::SoftwareImageType());
            juce::Graphics eg(layer.emission);eg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(raster));emissionGraphics=&eg;graph(g,{32,y,752,160},i==0);emissionGraphics=nullptr;
            layer.intensity=(gainFrozen||scopeFrozen)?0.f:1.f;
            if(i==1){if(scopeFrozen)gpuPhosphor[size_t(i)].reset();else gpuPhosphor[size_t(i)].apply(layer.emission,displayTime,scopeWindow);}
        }
        lastGpuFrame=frame;glowRenderer->publish(std::move(frame));
        }
    }else
#endif
    {for(int i=0;i<2;++i){const float y=i?583.f:396.f;const juce::Rectangle<float> plot(50,y+27,700,107);if(!g.clipRegionIntersects(plot.toNearestInt()))continue;auto& layer=softwarePlots[size_t(i)];
        const float device=g.getInternalContext().getPhysicalPixelScaleFactor();layer.prepare(juce::jmax(1,juce::roundToInt(plot.getWidth()*device)),juce::jmax(1,juce::roundToInt(plot.getHeight()*device)));
        juce::Graphics cg(layer.core);cg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(device));
        juce::Graphics eg(layer.emission);const float raster=float(layer.emission.getWidth())/plot.getWidth();eg.addTransform(juce::AffineTransform::translation(-plot.getX(),-plot.getY()).scaled(raster));emissionGraphics=&eg;graph(cg,{32,y,752,160},i==0);emissionGraphics=nullptr;
        const float chromeDevice=float(chrome.getWidth())/float(getWidth());auto crop=(scaled(50,y+27,700,107).toFloat()*chromeDevice).toNearestInt().getIntersection(chrome.getBounds());
        const float intensity=(gainFrozen||scopeFrozen)?0.f:1.f;
        layer.paint(g,chrome.getClippedImage(crop),plot,intensity,displayTime,i?scopeWindow:gainWindow,i==1);
    }}
    if(triggerStamp>=0&&!gainFrozen){const float flash=1.f-float((juce::Time::getMillisecondCounterHiRes()-triggerStamp)/180.);if(flash>0){g.setColour(look.tokens().out.withAlpha(flash*.45f));g.fillRect(748.f,423.f,2.f,107.f);}}
    if(filtersExpanded&&g.clipRegionIntersects({48,792,704,100}))paintDynamicLabels(g);
}
void DuckPocketAudioProcessorEditor::captureBlurSnapshot(){
    if(capturingBlur||blurArea.isEmpty())return;
    capturingBlur=true;const float old=bypassMix;bypassMix=0;
    auto source=createComponentSnapshot(blurArea,true,1.f);
    bypassMix=old;capturingBlur=false;
    if(!source.isValid()||source.getWidth()<8||source.getHeight()<8)return;
    const int w=juce::jmax(16,source.getWidth()/2),h=juce::jmax(16,source.getHeight()/2);
    juce::Image small(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
    {juce::Graphics sg(small);sg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);sg.drawImage(source,juce::Rectangle<float>(0,0,float(w),float(h)),juce::RectanglePlacement::stretchToFit);}
    // JUCE 8.0.4 convolution forms out-of-image edge pointers before checking
    // bounds (UBSan on macOS). Reuse the clamped, premultiplied separable blur.
    juce::Image horizontal(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
    juce::Image soft(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
    const int radius=juce::jmax(1,juce::roundToInt(float(getWidth())/800.f*3.f));
    for(int pass=0;pass<3;++pass){PocketSoftwareGlow::boxBlur(pass==0?small:soft,horizontal,true,radius);PocketSoftwareGlow::boxBlur(horizontal,soft,false,radius);}
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
#if DUCK_ENABLE_OPENGL && ! JUCE_WINDOWS
    if(getPeer()&&preferences&&preferences->getBoolValue("duckPocket.ui.opengl.v2",true)&&!glowRenderer)setOpenGL(true,false);
#endif
}
#if DUCK_ENABLE_OPENGL
void DuckPocketAudioProcessorEditor::setOpenGL(bool enabled,bool persist){
#if JUCE_WINDOWS
    // The hosted WGL peer can access-violate before a renderer callback runs.
    enabled=false;
#endif
    if(enabled&&glowRenderer)return;
    if(glowRenderer){glowRenderer->stop();glowRenderer.reset();}
    lastGpuFrame.reset();for(auto& phosphor:gpuPhosphor)phosphor.reset();glWasReady=false;setOpaque(!enabled);
    if(enabled&&getPeer()){glowRenderer=std::make_unique<PocketGlowRenderer>();glAttachTime=juce::Time::getMillisecondCounterHiRes();glowRenderer->attach(*this);}
    if(persist&&preferences){preferences->setValue("duckPocket.ui.opengl.v2",enabled);preferences->saveIfNeeded();}
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
    if(epoch!=traceGeneration){traceGeneration=epoch;cursor=filled=summaryCursor=summaryFilled=0;summaryBin=-1;displayTime=lastClock=lastLatest=gapMax=0;gainResume=scopeResume=0;lastPaintedTime=-1;lastVisibleSignalTime=-1;signalPeak=currentReduction=0;for(auto& layer:softwarePlots)layer.reset();
#if DUCK_ENABLE_OPENGL
        for(auto& phosphor:gpuPhosphor)phosphor.reset();
#endif
        triggerStamp=-1;repaint(gainArea);repaint(scopeArea);}
    PocketTrace v;bool fresh=false;const float previousPeak=signalPeak;float framePeak=0;
    while(audioProcessor.popTrace(v)){
        if(v.generation!=traceGeneration||!std::isfinite(v.time))continue;
        history[size_t(cursor)]=v;cursor=(cursor+1)%historyCapacity;filled=juce::jmin(filled+1,historyCapacity);
        const auto bin=static_cast<long long>(std::floor(v.time*500.));
        if(bin!=summaryBin){summaryBin=bin;summaryHistory[size_t(summaryCursor)]=v;summaryCursor=(summaryCursor+1)%summaryCapacity;summaryFilled=juce::jmin(summaryFilled+1,summaryCapacity);}
        else {auto& s=summaryHistory[size_t((summaryCursor+summaryCapacity-1)%summaryCapacity)];s.keyLo=juce::jmin(s.keyLo,v.keyLo);s.keyHi=juce::jmax(s.keyHi,v.keyHi);s.outLo=juce::jmin(s.outLo,v.outLo);s.outHi=juce::jmax(s.outHi,v.outHi);s.gain=juce::jmin(s.gain,v.gain);s.time=v.time;}
        if(1.f-v.gain-currentReduction>.035f){const auto stamp=juce::Time::getMillisecondCounterHiRes();if(triggerStamp<0||stamp-triggerStamp>180)triggerStamp=stamp;}
        signalPeak=juce::jmax(std::abs(v.keyLo),std::abs(v.keyHi),std::abs(v.outLo),std::abs(v.outHi));currentReduction=1.f-v.gain;
        framePeak=juce::jmax(framePeak,signalPeak);if(signalPeak>.004f||currentReduction>.001f)lastVisibleSignalTime=v.time;
        fresh=true;
    }
    if(fresh)signalPeak=juce::jmax(framePeak,previousPeak*.78f);
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
    if(bypassMix>0){
        if(!blurredSnapshot.isValid()&&(resizeStamp==0||juce::Time::getMillisecondCounterHiRes()-resizeStamp>100)){
            captureBlurSnapshot();repaint(blurArea);
        }
        return;
    }
    const bool moved=std::abs(displayTime-lastPaintedTime)>1e-7;
    if(!fresh&&!moved)return;
    if(fresh&&signalPeak<=.004f&&currentReduction<=.001f&&displayTime-lastVisibleSignalTime>gainWindow+.2)return;
    lastPaintedTime=displayTime;
    if(!gainFrozen)repaint(gainArea);
    if(!scopeFrozen)repaint(scopeArea);
}
