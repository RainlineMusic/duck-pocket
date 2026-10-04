#include "PluginEditor.h"
#include <iostream>
#include <cstdlib>
struct DuckUiTestAccess {
 static void theme(DuckPocketAudioProcessorEditor& e,PocketTheme t){e.setTheme(t,false);}
 static void tick(DuckPocketAudioProcessorEditor& e){e.frameTick();}
 static void settle(DuckPocketAudioProcessorEditor& e){e.resizeStamp=0;}
 static std::array<std::uint64_t,2> plots(DuckPocketAudioProcessorEditor& e){return {e.softwarePlots[0].prepares,e.softwarePlots[1].prepares};}
 static std::uint64_t paints(DuckPocketAudioProcessorEditor& e){return e.paintCount;}
 static std::uint64_t caches(DuckPocketAudioProcessorEditor& e){return e.chromeBuildCount;}
#if DUCK_ENABLE_OPENGL
 static void gl(DuckPocketAudioProcessorEditor& e,bool enabled){e.setOpenGL(enabled,false);}
 static void simulateFailedGl(DuckPocketAudioProcessorEditor& e){e.setOpaque(false);e.glowRenderer=std::make_unique<PocketGlowRenderer>();e.glowRenderer->ready.store(false);e.glowRenderer->presented.store(true);e.glowRenderer->failed.store(true);e.signalPeak=e.currentReduction=0;}
 static bool ready(DuckPocketAudioProcessorEditor& e){return e.glowRenderer&&e.glowRenderer->ready.load();}
#endif
};
static void check(bool v,const char* message){if(!v){std::cerr<<"FAIL "<<message<<'\n';std::abort();}}
static void pump(int milliseconds){juce::MessageManager::getInstance()->runDispatchLoopUntil(milliseconds);}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;const juce::File output(argc>1?argv[1]:"screenshots");output.createDirectory();
 DuckPocketAudioProcessor p;p.setRateAndBufferSizeDetails(48000,64);p.prepareToPlay(48000,64);
 std::unique_ptr<DuckPocketAudioProcessorEditor> e(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));const bool native=argc>2&&juce::String(argv[2])=="--native";if(native){e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);}e->setSize(960,760);DuckUiTestAccess::settle(*e);
 PocketLook dialLook;ModernDial durationDial(dialLook,"Duration","","ms",0,true);durationDial.setRange(5,2000,1);durationDial.setSkewFactor(.25);durationDial.setValue(1999,juce::dontSendNotification);check(!durationDial.isAutoValue(),"1999 ms remains finite");durationDial.setValue(2000,juce::dontSendNotification);check(durationDial.isAutoValue(),"2000 ms is AUTO");durationDial.setRange(1,100,1);durationDial.setValue(99,juce::dontSendNotification);check(!durationDial.isAutoValue(),"99 percent remains finite");durationDial.setValue(100,juce::dontSendNotification);check(durationDial.isAutoValue(),"100 percent is AUTO");
 const PocketTheme themes[]{PocketTheme::Neon,PocketTheme::SolidDark,PocketTheme::SolidWhite,PocketTheme::Amber};const char* names[]{"neon","dark","white","amber"};
 juce::MidiBuffer midi;juce::AudioBuffer<float> b(4,64);
 for(int ti=0;ti<4;++ti)for(int state=0;state<3;++state){p.reset();DuckUiTestAccess::tick(*e);DuckUiTestAccess::theme(*e,themes[ti]);
  for(int block=0;block<750;++block){for(int i=0;i<64;++i){const int n=block*64+i;const double time=double(n)/48000.,hit=std::fmod(time,.25);
   const float out=state==0?0:float((state==1?.035:.4)*std::sin(time*6.2831853*83));const float key=state==0?0:float((state==1?.07:.95)*std::exp(-hit/.055)*std::sin(hit*6.2831853*(55+120*std::exp(-hit/.01))));
   b.setSample(0,i,out);b.setSample(1,i,out*.8f);b.setSample(2,i,key);b.setSample(3,i,key);}
   p.processBlock(b,midi);if(block%12==0)DuckUiTestAccess::tick(*e);}
  DuckUiTestAccess::tick(*e);
  for(int scale:{1,2}){auto start=juce::Time::getMillisecondCounterHiRes();auto image=e->createComponentSnapshot(e->getLocalBounds(),true,float(scale));auto target=output.getChildFile(juce::String(names[ti])+"-"+juce::String(state)+"-"+juce::String(scale)+"x.png");target.deleteFile();auto stream=target.createOutputStream();check(stream&&stream->openedOk(),"PNG file creation");check(juce::PNGImageFormat().writeImageToStream(image,*stream),"PNG encoding");stream->flush();
   std::cout<<names[ti]<<" state="<<state<<" DPI="<<scale<<" capture_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';}
 }
 e->createComponentSnapshot(e->getLocalBounds());auto cached=DuckUiTestAccess::caches(*e);auto warm=juce::Time::getMillisecondCounterHiRes();for(int i=0;i<20;++i){e->createComponentSnapshot(e->getLocalBounds());}std::cout<<"warm_snapshot_ms="<<(juce::Time::getMillisecondCounterHiRes()-warm)/20.<<'\n';check(DuckUiTestAccess::caches(*e)==cached,"chrome reused when unchanged");
 auto before= DuckUiTestAccess::plots(*e);e->createComponentSnapshot({700,100,180,160});check(DuckUiTestAccess::plots(*e)==before,"knob-only clip never rasterises graphs");
 e->createComponentSnapshot({42,135,200,100});auto after=DuckUiTestAccess::plots(*e);check(after[0]==before[0]+1&&after[1]==before[1],"gain-only clip does not rasterise oscilloscope");

#if DUCK_ENABLE_OPENGL
 DuckUiTestAccess::simulateFailedGl(*e);auto fallback=e->createComponentSnapshot(e->getLocalBounds());check(fallback.getPixelAt(300,420).getAlpha()==255,"failed GL leaves opaque software glass");DuckUiTestAccess::gl(*e,false);
 if(native){DuckUiTestAccess::gl(*e,true);pump(400);DuckUiTestAccess::tick(*e);pump(100);std::cout<<"GL created="<<DuckUiTestAccess::ready(*e)<<'\n';
 check(DuckUiTestAccess::ready(*e),"GL context/shaders on local Mesa");
 for(int i=0;i<5;++i){DuckUiTestAccess::gl(*e,false);DuckUiTestAccess::gl(*e,true);pump(100);}
 DuckUiTestAccess::gl(*e,false);}
#endif
 if(native){e->removeFromDesktop();}
 e.reset();std::cout<<"PASS real JUCE theme captures and chrome reuse\n";
}
