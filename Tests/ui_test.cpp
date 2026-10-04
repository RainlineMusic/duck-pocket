#include "PluginEditor.h"
#include <iostream>
#include <cstdlib>
struct DuckUiTestAccess {
 static void theme(DuckPocketAudioProcessorEditor& e,PocketTheme t){e.setTheme(t,false);}
 static ModernDial& balance(DuckPocketAudioProcessorEditor& e){return e.midSide;}
 static bool layout(DuckPocketAudioProcessorEditor& e){
  const auto bounds=e.getLocalBounds();
  const juce::Component* controls[]{&e.influence,&e.duration,&e.outputGain,&e.midSide,&e.sidechainRange,&e.processingRange,&e.settingsButton,&e.bypassButton,&e.freezeButton};
  for(auto* c:controls)if(c->isVisible()&&!bounds.contains(c->getBounds()))return false;
  for(size_t i=0;i<9;++i)for(size_t j=i+1;j<9;++j)if(controls[i]->getBounds().intersects(controls[j]->getBounds()))return false;
  return e.gainArea.getWidth()==e.scopeArea.getWidth()&&e.gainArea.getHeight()==e.scopeArea.getHeight();
 }
 static void freeze(DuckPocketAudioProcessorEditor& e){e.freezeButton.triggerClick();}
 static bool frozen(DuckPocketAudioProcessorEditor& e){return e.gainFrozen&&e.scopeFrozen;}
 static bool bypassOverlay(DuckPocketAudioProcessorEditor& e){return e.blurredSnapshot.isValid()&&e.blurArea.getBottom()<=e.getHeight();}
 static void collapse(DuckPocketAudioProcessorEditor& e,bool open){e.setFiltersExpanded(open,false);}
 static juce::Rectangle<int> resizeGrip(DuckPocketAudioProcessorEditor& e){for(auto* c:e.getChildren())if(dynamic_cast<juce::ResizableCornerComponent*>(c))return c->getBounds();return {};}
 static bool rangesVisible(DuckPocketAudioProcessorEditor& e){return e.sidechainRange.isVisible()&&e.processingRange.isVisible();}
 static bool scopeHasSignal(DuckPocketAudioProcessorEditor& e){
  const auto& core=e.softwarePlots[1].core;if(!core.isValid())return false;
  juce::Image::BitmapData data(core,juce::Image::BitmapData::readOnly);
  for(int y=0;y<data.height;++y)for(int x=0;x<data.width;++x)if(data.getPixelColour(x,y).getAlpha()>0)return true;return false;
 }
 static bool resumeReset(DuckPocketAudioProcessorEditor& e){return e.gainResume==0&&e.scopeResume==0;}
 static void tick(DuckPocketAudioProcessorEditor& e){e.frameTick();}
 static void settle(DuckPocketAudioProcessorEditor& e){e.resizeStamp=0;}
 static std::array<std::uint64_t,2> plots(DuckPocketAudioProcessorEditor& e){return {e.softwarePlots[0].prepares,e.softwarePlots[1].prepares};}
 static std::uint64_t paints(DuckPocketAudioProcessorEditor& e){return e.paintCount;}
 static std::uint64_t caches(DuckPocketAudioProcessorEditor& e){return e.chromeBuildCount;}
#if DUCK_ENABLE_OPENGL
 static void gl(DuckPocketAudioProcessorEditor& e,bool enabled){e.setOpenGL(enabled,false);}
 static void simulateFailedGl(DuckPocketAudioProcessorEditor& e){e.setOpaque(false);e.glowRenderer=std::make_unique<PocketGlowRenderer>();e.glowRenderer->ready.store(false);e.glowRenderer->presented.store(true);e.glowRenderer->failed.store(true);e.signalPeak=e.currentReduction=0;}
 static bool ready(DuckPocketAudioProcessorEditor& e){return e.glowRenderer&&e.glowRenderer->ready.load();}
 static bool failed(DuckPocketAudioProcessorEditor& e){return !e.glowRenderer||e.glowRenderer->failed.load();}
 static bool fellBack(DuckPocketAudioProcessorEditor& e){return !e.glowRenderer&&e.isOpaque();}
 static int gpuHeight(DuckPocketAudioProcessorEditor& e){return e.glowRenderer?e.glowRenderer->presentedLogicalHeight.load():0;}
 static std::uint64_t gpuRevision(DuckPocketAudioProcessorEditor& e){return e.glowRenderer?e.glowRenderer->presentedRevision.load():0;}
 static bool themePresented(DuckPocketAudioProcessorEditor& e,std::uint64_t previous){return gpuRevision(e)>previous;}
 static bool rendered(DuckPocketAudioProcessorEditor& e){return e.glowRenderer&&e.glowRenderer->presented.load()&&e.glowRenderer->frames.load()>0;}
 static std::uint64_t blurredFrames(DuckPocketAudioProcessorEditor& e){return e.glowRenderer?e.glowRenderer->blurredFrames.load():0;}
 static void trigger(DuckPocketAudioProcessorEditor& e){e.repaint();if(e.glowRenderer)e.glowRenderer->context.triggerRepaint();}
#endif
};
static void check(bool v,const char* message){if(!v){std::cerr<<"FAIL "<<message<<'\n';std::abort();}}
static void pump(int milliseconds){juce::MessageManager::getInstance()->runDispatchLoopUntil(milliseconds);}
int main(int argc,char** argv){const bool glSmoke=argc>1&&juce::String(argv[1])=="--gl-smoke";if(glSmoke)std::cerr<<"GL_PROBE_START\n";juce::ScopedJuceInitialiser_GUI init;if(glSmoke)std::cerr<<"GL_PROBE_GUI_READY\n";const juce::File output(glSmoke?juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("duck-gl-smoke"):juce::File(argc>1?argv[1]:"screenshots"));if(!glSmoke)output.createDirectory();
 DuckPocketAudioProcessor p;p.setRateAndBufferSizeDetails(48000,64);p.prepareToPlay(48000,64);
 if(glSmoke)std::cerr<<"GL_PROBE_PROCESSOR_READY\n";
 std::unique_ptr<DuckPocketAudioProcessorEditor> e(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));const bool native=argc>2&&juce::String(argv[2])=="--native";if(native){e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);}e->setSize(800,885);DuckUiTestAccess::settle(*e);
 if(glSmoke)std::cerr<<"GL_PROBE_EDITOR_READY\n";
 if(glSmoke){
#if DUCK_ENABLE_OPENGL
#if JUCE_WINDOWS
  const double start=juce::Time::getMillisecondCounterHiRes();
  for(int cycle=0;cycle<100;++cycle){
   if(cycle>0){e.reset(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));e->setSize(800,885);}
   e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);
   check(e->getPeer()!=nullptr,"native Windows editor peer created");
   DuckUiTestAccess::gl(*e,true);
   check(DuckUiTestAccess::fellBack(*e),"Windows OpenGL request stays on the opaque native renderer");
   if(cycle==0||cycle==99){const auto frame=e->createComponentSnapshot(e->getLocalBounds());check(frame.isValid()&&frame.getPixelAt(300,650).getAlpha()==255,"native Windows renderer paints opaque graph glass");}
   e->removeFromDesktop();e.reset();
  }
  std::cout<<"PASS 100 native Windows editor peers with OpenGL disabled, total_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';
#else
  const double start=juce::Time::getMillisecondCounterHiRes();
  for(int cycle=0;cycle<100;++cycle){
   if(cycle>0){e.reset(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));e->setSize(800,885);}
   if(cycle==0)std::cerr<<"GL_PROBE_ATTACH_PEER\n";
   e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);
   if(cycle==0)std::cerr<<"GL_PROBE_PEER_READY\n";
   check(e->getPeer()!=nullptr,"native editor peer created");DuckUiTestAccess::gl(*e,true);
   if(cycle==0)std::cerr<<"GL_PROBE_CONTEXT_ATTACHED\n";
   for(int poll=0;poll<100&&!DuckUiTestAccess::ready(*e)&&!DuckUiTestAccess::failed(*e);++poll){DuckUiTestAccess::trigger(*e);pump(20);}
   if(cycle==0)std::cerr<<"GL_PROBE_CONTEXT_READY="<<DuckUiTestAccess::ready(*e)<<" FAILED="<<DuckUiTestAccess::failed(*e)<<'\n';
   if(!DuckUiTestAccess::ready(*e)&&DuckUiTestAccess::failed(*e)){
    DuckUiTestAccess::tick(*e);
    check(DuckUiTestAccess::fellBack(*e),"unsupported OpenGL returns to opaque software renderer");
    const auto fallback=e->createComponentSnapshot(e->getLocalBounds());
    check(fallback.isValid()&&fallback.getPixelAt(300,650).getAlpha()==255,"native fallback paints opaque graph glass");
    e->removeFromDesktop();e.reset();
    std::cout<<"SKIP native OpenGL unavailable; software fallback verified after cycle="<<cycle<<'\n';return 0;
   }
   check(DuckUiTestAccess::ready(*e),"native OpenGL context and shaders created");
   if(cycle==0){
    juce::AudioBuffer<float> samples(4,64);juce::MidiBuffer midi;
    for(int block=0;block<200;++block){for(int i=0;i<64;++i){const double time=double(block*64+i)/48000.;const float out=float(.25*std::sin(time*6.2831853*83));const float key=float(.8*std::sin(time*6.2831853*110));samples.setSample(0,i,out);samples.setSample(1,i,out);samples.setSample(2,i,key);samples.setSample(3,i,key);}p.processBlock(samples,midi);}
    DuckUiTestAccess::tick(*e);
   }
   for(int poll=0;poll<75&&!(cycle==0?DuckUiTestAccess::blurredFrames(*e)>0:DuckUiTestAccess::rendered(*e));++poll){DuckUiTestAccess::trigger(*e);pump(20);}
   check(DuckUiTestAccess::rendered(*e),"native GPU composed a graph frame");
   if(cycle==0)check(DuckUiTestAccess::blurredFrames(*e)>0,"native GPU blur rendered a live signal");
   if(cycle==0){const auto previous=DuckUiTestAccess::gpuRevision(*e);DuckUiTestAccess::theme(*e,PocketTheme::Amber);for(int poll=0;poll<75&&!DuckUiTestAccess::themePresented(*e,previous);++poll)pump(20);check(DuckUiTestAccess::themePresented(*e,previous),"paused GPU presents the new theme without audio");}
   if(cycle==0){for(int fold=0;fold<10;++fold){const int width=e->getWidth();DuckUiTestAccess::collapse(*e,(fold%2)==0);
    for(int poll=0;poll<75&&DuckUiTestAccess::gpuHeight(*e)!=e->getHeight();++poll){DuckUiTestAccess::trigger(*e);pump(20);}
    check(e->getWidth()==width,"native fold preserves width");check(DuckUiTestAccess::gpuHeight(*e)==e->getHeight(),"native GPU uses live drawable height after paused fold");}}
   DuckUiTestAccess::gl(*e,false);e->removeFromDesktop();e.reset();
  }
  std::cout<<"PASS native OpenGL glow and 100 editor peer lifecycles, total_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';
#endif
#else
  std::cout<<"SKIP OpenGL is disabled at compile time\n";
#endif
  return 0;
 }
 juce::Image edge(juce::Image::ARGB,1,1,true,juce::SoftwareImageType()),edgeBlur(juce::Image::ARGB,1,1,true,juce::SoftwareImageType());edge.setPixelAt(0,0,juce::Colours::white);
 PocketSoftwareGlow::blur(edge,edgeBlur,true,2);check(edgeBlur.getPixelAt(0,0)==juce::Colours::white,"single-pixel blur clamps edges safely");
 PocketSoftwareGlow::blur(edgeBlur,edge,false,2);check(edge.getPixelAt(0,0)==juce::Colours::white,"vertical blur also clamps edges safely");
 PocketPhosphorTrail phosphor;juce::Image emission(juce::Image::ARGB,100,8,true,juce::SoftwareImageType());
 emission.setPixelAt(60,4,juce::Colours::white);phosphor.apply(emission,1.,1.);
 emission.clear(emission.getBounds());phosphor.apply(emission,1.1,1.);
 const auto tail=emission.getPixelAt(50,4);check(tail.getAlpha()>0&&tail.getAlpha()<255,"phosphor tail scrolls and decays");
 emission.clear(emission.getBounds());phosphor.apply(emission,1.1,1.);check(emission.getPixelAt(50,4)==tail,"same-time repaint retains phosphor tail");
 emission.clear(emission.getBounds());phosphor.apply(emission,.5,1.);check(emission.getPixelAt(50,4).getAlpha()==0,"rewound timeline discards old phosphor");
 emission.setPixelAt(60,4,juce::Colours::white);phosphor.apply(emission,.6,1.);phosphor.reset();emission.clear(emission.getBounds());phosphor.apply(emission,.7,1.);check(emission.getPixelAt(50,4).getAlpha()==0,"reset discards old phosphor");
 PocketLook dialLook;ModernDial durationDial(dialLook,"Duration","","ms",0,true);durationDial.setRange(5,2000,1);durationDial.setSkewFactor(.25);durationDial.setValue(1999,juce::dontSendNotification);check(!durationDial.isAutoValue(),"1999 ms remains finite");durationDial.setValue(2000,juce::dontSendNotification);check(durationDial.isAutoValue(),"2000 ms is AUTO");durationDial.setRange(1,100,1);durationDial.setValue(99,juce::dontSendNotification);check(!durationDial.isAutoValue(),"99 percent remains finite");durationDial.setValue(100,juce::dontSendNotification);check(durationDial.isAutoValue(),"100 percent is AUTO");
 ModernDial outputDial(dialLook,"Output","dB","dB",0,false,false,true);for(int size:{66,101,132,198}){outputDial.setSize(size,size);for(const char* value:{"-12.00","-0.01","0.00","6.00"}){const float height=outputDial.valueTextHeight(value);check(height>=6.f,"Output value stays readable");check(juce::GlyphArrangement::getStringWidth(pocketFont(height*132.f/float(size),true),value)*float(size)/132.f<=float(size)*60.f/132.f+.01f,"Output endpoints fit compact dial");}}
 outputDial.setRange(-12,6);outputDial.setValue(-.0000003,juce::dontSendNotification);check(outputDial.displayedValue()=="0.00","Output floating-point zero has no minus sign");outputDial.setValue(-.01,juce::dontSendNotification);check(outputDial.displayedValue()=="-0.01","negative Output remains negative");
 for(int width:{400,615,800,1500}){e->setSize(width,juce::roundToInt(width*885./800.));check(DuckUiTestAccess::layout(*e),"controls remain contained and separate during resize");}
 DuckUiTestAccess::collapse(*e,false);check(!DuckUiTestAccess::rangesVisible(*e)&&e->getHeight()<int(e->getWidth()*885./800.),"collapse hides both filters and shortens the window");
 DuckUiTestAccess::collapse(*e,true);check(DuckUiTestAccess::rangesVisible(*e),"expand restores both filters");
 p.parameters.getParameter("bypass")->setValueNotifyingHost(1);DuckUiTestAccess::tick(*e);
 DuckUiTestAccess::collapse(*e,false);DuckUiTestAccess::settle(*e);DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::bypassOverlay(*e),"collapse during bypass rebuilds the resized overlay");
 p.parameters.getParameter("bypass")->setValueNotifyingHost(0);DuckUiTestAccess::tick(*e);DuckUiTestAccess::collapse(*e,true);
 for(int width:{400,615,1500}){
  DuckUiTestAccess::collapse(*e,true);e->setSize(width,juce::roundToInt(width*885./800.));DuckUiTestAccess::settle(*e);
  const auto before=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);const auto cache=DuckUiTestAccess::caches(*e);
  const int topHeight=juce::roundToInt(744.f*float(width)/800.f*2.f);
  for(int fold=0;fold<4;++fold){DuckUiTestAccess::collapse(*e,(fold%2)!=0);
   const auto after=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);
   check(e->getWidth()==width,"fold never changes width at the minimum or maximum size");
   check(DuckUiTestAccess::caches(*e)==cache,"fold reuses chrome immediately, without settling or regeneration");
   juce::Image::BitmapData a(before,juce::Image::BitmapData::readOnly),b(after,juce::Image::BitmapData::readOnly);
   const auto grip=(DuckUiTestAccess::resizeGrip(*e).toFloat()*2.f).toNearestInt();
   for(int y=0;y<topHeight;++y)for(int x=0;x<a.width;++x)if(!grip.contains(x,y)&&a.getPixelColour(x,y)!=b.getPixelColour(x,y)){std::cerr<<"fold diff width="<<width<<" fold="<<fold<<" pixel="<<x<<","<<y<<" before="<<a.getPixelColour(x,y).toString()<<" after="<<b.getPixelColour(x,y).toString()<<" height="<<e->getHeight()<<"\n";check(false,"paused top/graph pixels stay identical through fold");}
  }
 }
 e->setSize(800,885);DuckUiTestAccess::settle(*e);
 DuckUiTestAccess::freeze(*e);pump(10);check(DuckUiTestAccess::frozen(*e),"freeze button freezes both graphs");
 DuckUiTestAccess::freeze(*e);pump(10);check(!DuckUiTestAccess::frozen(*e),"freeze button resumes both graphs");
 auto& balance= DuckUiTestAccess::balance(*e);
 check(balance.getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag,"M/S uses a rotary control");
 for(double value:{-1.,-.25,0.,.25,1.}){
  auto* param=p.parameters.getParameter("msBalance");param->setValueNotifyingHost(param->convertTo0to1(float(value)));pump(5);
  check(std::abs(balance.getValue()-value)<.00001,"M/S knob follows host automation");
  check(balance.displayedValue()==juce::String(juce::roundToInt(std::abs(value)*100.))+"%","M/S magnitude display");
  check(balance.balanceLabel()==(value<0?"MID":(value>0?"SIDE":"MS")),"M/S direction display");
 }
 balance.setValue(0,juce::sendNotificationSync);check(std::abs(p.parameters.getRawParameterValue("msBalance")->load())<.00001f,"M/S gesture returns parameter to neutral");
 // Reproduce host stop/reset after a freeze/resume cut-off at >1 second.
 juce::AudioBuffer<float> resumed(4,64);juce::MidiBuffer resumeMidi;
 for(int i=0;i<1200;++i){resumed.clear();p.processBlock(resumed,resumeMidi);}DuckUiTestAccess::tick(*e);
 DuckUiTestAccess::freeze(*e);pump(10);DuckUiTestAccess::freeze(*e);pump(10);
 p.reset();DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::resumeReset(*e),"host timeline reset discards the previous freeze cut-off");
 for(int block=0;block<150;++block){for(int sample=0;sample<64;++sample){const float v=.3f*std::sin(float(block*64+sample)*.01f);for(int channel=0;channel<4;++channel)resumed.setSample(channel,sample,v);}p.processBlock(resumed,resumeMidi);}DuckUiTestAccess::tick(*e);
 e->createComponentSnapshot(e->getLocalBounds());check(DuckUiTestAccess::scopeHasSignal(*e),"audio after stop/reset resumes visible scope immediately");
 const PocketTheme themes[]{PocketTheme::Neon,PocketTheme::SolidDark,PocketTheme::Amber};const char* names[]{"neon","dark","amber"};
 juce::MidiBuffer midi;juce::AudioBuffer<float> b(4,64);
 for(int ti=0;ti<3;++ti)for(int state=0;state<3;++state){p.reset();DuckUiTestAccess::tick(*e);DuckUiTestAccess::theme(*e,themes[ti]);
  for(int block=0;block<750;++block){for(int i=0;i<64;++i){const int n=block*64+i;const double time=double(n)/48000.,hit=std::fmod(time,.25);
   const float out=state==0?0:float((state==1?.035:.4)*std::sin(time*6.2831853*83));const float key=state==0?0:float((state==1?.07:.95)*std::exp(-hit/.055)*std::sin(hit*6.2831853*(55+120*std::exp(-hit/.01))));
   b.setSample(0,i,out);b.setSample(1,i,out*.8f);b.setSample(2,i,key);b.setSample(3,i,key);}
   p.processBlock(b,midi);if(block%12==0)DuckUiTestAccess::tick(*e);}
  DuckUiTestAccess::tick(*e);
  for(int scale:{1,2}){auto start=juce::Time::getMillisecondCounterHiRes();auto image=e->createComponentSnapshot(e->getLocalBounds(),true,float(scale));auto target=output.getChildFile(juce::String(names[ti])+"-"+juce::String(state)+"-"+juce::String(scale)+"x.png");target.deleteFile();auto stream=target.createOutputStream();check(stream&&stream->openedOk(),"PNG file creation");check(juce::PNGImageFormat().writeImageToStream(image,*stream),"PNG encoding");stream->flush();
   std::cout<<names[ti]<<" state="<<state<<" DPI="<<scale<<" capture_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';}
 }
 e->createComponentSnapshot(e->getLocalBounds());auto cached=DuckUiTestAccess::caches(*e);auto warm=juce::Time::getMillisecondCounterHiRes();for(int i=0;i<20;++i){e->createComponentSnapshot(e->getLocalBounds());}std::cout<<"warm_snapshot_ms="<<(juce::Time::getMillisecondCounterHiRes()-warm)/20.<<'\n';check(DuckUiTestAccess::caches(*e)==cached,"chrome reused when unchanged");
 auto before= DuckUiTestAccess::plots(*e);e->createComponentSnapshot({60,108,240,250});check(DuckUiTestAccess::plots(*e)==before,"knob-only clip never rasterises graphs");
 e->createComponentSnapshot({50,423,200,100});auto after=DuckUiTestAccess::plots(*e);check(after[0]==before[0]+1&&after[1]==before[1],"gain-only clip does not rasterise oscilloscope");

#if DUCK_ENABLE_OPENGL
 DuckUiTestAccess::simulateFailedGl(*e);auto fallback=e->createComponentSnapshot(e->getLocalBounds());check(fallback.getPixelAt(300,650).getAlpha()==255,"failed GL leaves opaque software glass");DuckUiTestAccess::gl(*e,false);
 if(native){DuckUiTestAccess::gl(*e,true);pump(400);DuckUiTestAccess::tick(*e);pump(100);std::cout<<"GL created="<<DuckUiTestAccess::ready(*e)<<'\n';
 check(DuckUiTestAccess::ready(*e),"GL context/shaders on local Mesa");
 for(int i=0;i<5;++i){DuckUiTestAccess::gl(*e,false);DuckUiTestAccess::gl(*e,true);pump(100);}
 DuckUiTestAccess::gl(*e,false);}
#endif
 if(native){e->removeFromDesktop();}
 DuckUiTestAccess::theme(*e,PocketTheme::SolidDark);
 for(int width:{400,615}){
  e->setSize(width,juce::roundToInt(width*885./800.));DuckUiTestAccess::settle(*e);
  auto image=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);
  auto stream=output.getChildFile("compact-"+juce::String(width)+"-2x.png").createOutputStream();check(stream&&juce::PNGImageFormat().writeImageToStream(image,*stream),"compact review capture");
 }
 e.reset();std::cout<<"PASS real JUCE theme captures and chrome reuse\n";
}
