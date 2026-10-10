#include "PluginEditor.h"
#include <iostream>
struct DuckLicenseTestAccess {static void activate(DuckPocketAudioProcessor& p,bool active=true){p.license->active.store(active);}};
#include <cstdlib>
#include <thread>
#include <chrono>
#if JUCE_WINDOWS
#include <windows.h>
#endif
struct DuckUiTestAccess {
#if JUCE_WINDOWS
 static void renderer(DuckPocketAudioProcessorEditor& e,const juce::String& name){e.setWindowsRenderer(name,false);}
 static std::uint64_t nativeCaches(DuckPocketAudioProcessorEditor& e){return e.nativeChromeBuildCount;}
#endif
 static bool activationShown(DuckPocketAudioProcessorEditor& e){return e.activationPanel.isVisible();}
 static juce::Image activationBackdrop(DuckPocketAudioProcessorEditor& e){return e.activationPanel.backdrop;}
 static bool activationControlsFit(DuckPocketAudioProcessorEditor& e){
  const auto card=e.activationPanel.card;
  for(auto* child:e.activationPanel.getChildren())if(child->isVisible()&&!card.contains(child->getBounds()))return false;
  return e.getLocalBounds().contains(card);
 }
 static void activationMode(DuckPocketAudioProcessorEditor& e,bool offline){e.setActivationMode(offline);}
 static juce::String deviceCode(DuckPocketAudioProcessorEditor& e){return e.deviceCodeInput.getText();}
 static bool fileDropAccepted(DuckPocketAudioProcessorEditor& e){return e.activationPanel.isInterestedInFileDrag(juce::StringArray{"one.ducklicense"});}
 static void invalidFile(DuckPocketAudioProcessorEditor& e){
  const auto file=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("duck-ui-invalid",".ducklicense");file.replaceWithText("not a signed license");e.activationPanel.filesDropped(juce::StringArray{file.getFullPathName()},0,0);file.deleteFile();
 }
 static void invalidKey(DuckPocketAudioProcessorEditor& e){e.licenseInput.setText("invalid");e.activateButton.onClick();}
 static void audition(DuckPocketAudioProcessorEditor& e){e.listenButton.onClick();}
 static void theme(DuckPocketAudioProcessorEditor& e,PocketTheme t){e.setTheme(t,false);}
 static juce::Image ageFade(DuckPocketAudioProcessorEditor& e,bool gain){
  e.gainWindow=e.scopeWindow=1.;e.gainFrozen=e.scopeFrozen=false;e.gainResume=e.scopeResume=0.;
  e.filled=e.cursor=2400;e.displayTime=1.;
  for(int i=0;i<2400;++i){auto& v=e.history[size_t(i)];v.time=double(i)/2400.;v.gain=.5f;v.outLo=v.keyLo=-.5f;v.outHi=v.keyHi=.5f;}
  juce::Image image(juce::Image::ARGB,700,107,true,juce::SoftwareImageType());
  juce::Graphics g(image);const float y=gain?396.f:583.f;g.addTransform(juce::AffineTransform::translation(-50.f,-y-27.f));e.graph(g,{32,y,752,160},gain);return image;
 }
 static HeaderValue& header(DuckPocketAudioProcessorEditor& e,bool output){return output?e.outputGain:e.mix;}
 static ModernDial& attack(DuckPocketAudioProcessorEditor& e){return e.attack;}
 static ModernDial& balance(DuckPocketAudioProcessorEditor& e){return e.midSide;}
 static bool layout(DuckPocketAudioProcessorEditor& e){
  const auto bounds=e.getLocalBounds();
  const juce::Component* controls[]{&e.influence,&e.duration,&e.attack,&e.outputGain,&e.mix,&e.midSide,&e.sidechainRange,&e.processingRange,&e.settingsButton,&e.bypassButton,&e.freezeButton};
  for(auto* c:controls)if(c->isVisible()&&!bounds.contains(c->getBounds()))return false;
  for(size_t i=0;i<11;++i)for(size_t j=i+1;j<11;++j)if(controls[i]->getBounds().intersects(controls[j]->getBounds()))return false;
  return e.gainArea.getWidth()==e.scopeArea.getWidth()&&e.gainArea.getHeight()==e.scopeArea.getHeight();
 }
 static void freeze(DuckPocketAudioProcessorEditor& e){e.freezeButton.triggerClick();}
 static bool frozen(DuckPocketAudioProcessorEditor& e){return e.gainFrozen&&e.scopeFrozen;}
 static bool bypassOverlay(DuckPocketAudioProcessorEditor& e){return e.blurredSnapshot.isValid()&&e.blurArea.getBottom()<=e.getHeight()&&e.blurredSnapshot.getWidth()>=e.blurArea.getWidth()/2;}
 static void collapse(DuckPocketAudioProcessorEditor& e,bool open){e.setFiltersExpanded(open,false);}
 static juce::Rectangle<int> resizeGrip(DuckPocketAudioProcessorEditor& e){for(auto* c:e.getChildren())if(dynamic_cast<juce::ResizableCornerComponent*>(c))return c->getBounds();return {};}
 static bool rangesVisible(DuckPocketAudioProcessorEditor& e){return e.sidechainRange.isVisible()&&e.processingRange.isVisible();}
 static bool scopeHasSignal(DuckPocketAudioProcessorEditor& e){
  juce::Image core(juce::Image::ARGB,700,107,true,juce::SoftwareImageType());
  {juce::Graphics g(core);g.addTransform(juce::AffineTransform::translation(-50.f,-610.f));e.graph(g,{32,583,752,160},false);}
  juce::Image::BitmapData data(core,juce::Image::BitmapData::readOnly);
  for(int y=0;y<data.height;++y)for(int x=0;x<data.width;++x)if(data.getPixelColour(x,y).getAlpha()>0)return true;return false;
 }
 static std::array<int,2> historyCounts(DuckPocketAudioProcessorEditor& e){return {e.filled,e.summaryFilled};}
 static void tick(DuckPocketAudioProcessorEditor& e){e.frameTick();}
 static void settle(DuckPocketAudioProcessorEditor& e){e.resizeStamp=0;}
 static std::array<std::uint64_t,2> plots(DuckPocketAudioProcessorEditor& e){return {e.softwarePlots[0].prepares,e.softwarePlots[1].prepares};}
 static std::uint64_t paints(DuckPocketAudioProcessorEditor& e){return e.paintCount;}
 static bool widthResizePending(DuckPocketAudioProcessorEditor& e){return !e.chromeValid&&e.resizeStamp>0;}
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
// CoreGraphics may round an 8-bit channel by one LSB when the target clip
// changes. This tolerance cannot hide a shifted edge/grid/curve; geometry,
// width and cache identity are also checked independently.
static bool samePixel(juce::Colour a,juce::Colour b){return a.getAlpha()==b.getAlpha()&&std::abs(int(a.getRed())-int(b.getRed()))<=1&&std::abs(int(a.getGreen())-int(b.getGreen()))<=1&&std::abs(int(a.getBlue())-int(b.getBlue()))<=1;}
static void pump(int milliseconds){juce::MessageManager::getInstance()->runDispatchLoopUntil(milliseconds);}
#if JUCE_WINDOWS
// WM_TIMER has lower priority than input. A continuously serviced heartbeat
// catches message-queue starvation that software snapshots cannot reveal.
static double heartbeatLast=0,heartbeatMax=0;
static int heartbeatCount=0;
static void CALLBACK heartbeat(HWND,UINT,UINT_PTR,DWORD){
 const double now=juce::Time::getMillisecondCounterHiRes();
 heartbeatMax=juce::jmax(heartbeatMax,now-heartbeatLast);heartbeatLast=now;++heartbeatCount;
}
#endif
int main(int argc,char** argv){const bool glSmoke=argc>1&&juce::String(argv[1])=="--gl-smoke";if(glSmoke)std::cerr<<"GL_PROBE_START\n";juce::ScopedJuceInitialiser_GUI init;if(glSmoke)std::cerr<<"GL_PROBE_GUI_READY\n";const juce::File output(glSmoke?juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("duck-gl-smoke"):juce::File(argc>1?argv[1]:"screenshots"));if(!glSmoke)output.createDirectory();
 DuckPocketAudioProcessor p;DuckLicenseTestAccess::activate(p);p.setRateAndBufferSizeDetails(48000,64);p.prepareToPlay(48000,64);
 if(glSmoke)std::cerr<<"GL_PROBE_PROCESSOR_READY\n";
 std::unique_ptr<DuckPocketAudioProcessorEditor> e(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));const bool native=argc>2&&juce::String(argv[2])=="--native";if(native){e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);}if(!glSmoke)check(!DuckUiTestAccess::rangesVisible(*e),"fresh filter panel defaults closed");DuckUiTestAccess::collapse(*e,true);e->setSize(800,905);DuckUiTestAccess::settle(*e);
 if(!glSmoke){
  DuckLicenseTestAccess::activate(p,false);DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::activationShown(*e),"activation overlay shown without license");
  const auto backdrop=DuckUiTestAccess::activationBackdrop(*e);check(backdrop.isValid()&&DuckUiTestAccess::activationControlsFit(*e),"centered activation card and cached background");
  DuckUiTestAccess::tick(*e);check(backdrop==DuckUiTestAccess::activationBackdrop(*e),"idle activation reuses the blurred image");
  DuckUiTestAccess::invalidKey(*e);check(!p.isActivated()&&DuckUiTestAccess::activationShown(*e),"invalid key keeps overlay and audio locked");
  auto image=e->createComponentSnapshot(e->getLocalBounds(),true,1.f);auto stream=output.getChildFile("activation.png").createOutputStream();check(stream&&juce::PNGImageFormat().writeImageToStream(image,*stream),"activation capture");
  check(!DuckUiTestAccess::fileDropAccepted(*e),"online mode does not import dragged files");
  DuckUiTestAccess::activationMode(*e,true);check(DuckUiTestAccess::fileDropAccepted(*e),"offline file drop enabled");
  check(pocket::canonicalDeviceCode(DuckUiTestAccess::deviceCode(*e))==p.licenseDeviceCode(),"offline shows the current device code");
  DuckUiTestAccess::invalidFile(*e);check(!p.isActivated()&&DuckUiTestAccess::activationShown(*e),"invalid dropped file stays locked");
  {auto capture=e->createComponentSnapshot(e->getLocalBounds(),true,1.f);auto file=output.getChildFile("activation-offline.png").createOutputStream();check(file&&juce::PNGImageFormat().writeImageToStream(capture,*file),"offline activation capture");}
  e->setSize(400,453);DuckUiTestAccess::activationMode(*e,true);
  check(DuckUiTestAccess::activationControlsFit(*e),"offline controls fit the smallest plugin window");
  {auto capture=e->createComponentSnapshot(e->getLocalBounds(),true,1.f);auto file=output.getChildFile("activation-offline-compact.png").createOutputStream();check(file&&juce::PNGImageFormat().writeImageToStream(capture,*file),"compact offline activation capture");}
  e->setSize(800,905);DuckUiTestAccess::activationMode(*e,false);
  DuckLicenseTestAccess::activate(p);DuckUiTestAccess::tick(*e);check(!DuckUiTestAccess::activationShown(*e),"activation shared with open editor");
  p.listenSidechain.store(true);DuckUiTestAccess::collapse(*e,false);check(!p.listenSidechain.load(),"collapse stops audition");DuckUiTestAccess::collapse(*e,true);
 }
 if(glSmoke)std::cerr<<"GL_PROBE_EDITOR_READY\n";
 if(glSmoke){
#if DUCK_ENABLE_OPENGL
#if JUCE_WINDOWS
  const double start=juce::Time::getMillisecondCounterHiRes();
  for(int cycle=0;cycle<100;++cycle){
   if(cycle>0){e.reset(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));e->setSize(800,905);}
   e->addToDesktop(juce::ComponentPeer::windowIsTemporary);e->setVisible(true);
   check(e->getPeer()!=nullptr,"native Windows editor peer created");
   if(cycle==0){
    const auto names=e->getPeer()->getAvailableRenderingEngines();
    const int original=e->getPeer()->getCurrentRenderingEngine();
    for(int engine=0;engine<names.size();++engine){
     DuckUiTestAccess::renderer(*e,names[engine]);pump(20);
     check(e->getPeer()->getCurrentRenderingEngine()==engine,"Windows renderer selection takes effect");
     const auto image=e->createComponentSnapshot(e->getLocalBounds());
     check(image.isValid(),"snapshot survives Windows renderer switch");
    }
    if(juce::isPositiveAndBelow(original,names.size()))DuckUiTestAccess::renderer(*e,names[original]);
   }
   DuckUiTestAccess::gl(*e,true);
   check(DuckUiTestAccess::fellBack(*e),"Windows GL request is blocked before unsafe native context creation");
   if(cycle==0||cycle==99){const auto frame=e->createComponentSnapshot(e->getLocalBounds());check(frame.isValid()&&frame.getPixelAt(300,650).getAlpha()==255,"native Windows renderer paints opaque graph glass");}
   e->removeFromDesktop();e.reset();
  }
  std::cout<<"PASS 100 native Windows editor peers with OpenGL disabled, total_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';
  // Reproduce four simultaneously open editors under live audio, including
  // HiDPI snapshots. Timing is diagnostic, not a hardware-dependent pass limit.
  std::array<std::unique_ptr<DuckPocketAudioProcessor>,4> processors;
  std::array<std::unique_ptr<DuckPocketAudioProcessorEditor>,4> editors;
  for(int i=0;i<4;++i){
   processors[size_t(i)]=std::make_unique<DuckPocketAudioProcessor>();
   processors[size_t(i)]->setRateAndBufferSizeDetails(48000,800);processors[size_t(i)]->prepareToPlay(48000,800);
   editors[size_t(i)].reset(static_cast<DuckPocketAudioProcessorEditor*>(processors[size_t(i)]->createEditor()));
   editors[size_t(i)]->setSize(615,609);DuckUiTestAccess::settle(*editors[size_t(i)]);
   editors[size_t(i)]->addToDesktop(juce::ComponentPeer::windowIsTemporary);editors[size_t(i)]->setVisible(true);
  }
  juce::AudioBuffer<float> samples(4,800);juce::MidiBuffer midi;
  for(int dpi:{1,2}){
   double captureMs=0.;
   for(int frame=0;frame<120;++frame){
    for(int n=0;n<800;++n){const double t=double(frame*800+n)/48000.;const float out=float(.4*std::sin(t*6.2831853*83));const float key=float(.9*std::exp(-std::fmod(t,.25)/.045)*std::sin(t*6.2831853*110));samples.setSample(0,n,out);samples.setSample(1,n,out);samples.setSample(2,n,key);samples.setSample(3,n,key);}
    for(int i=0;i<4;++i){juce::AudioBuffer<float> input(samples);processors[size_t(i)]->processBlock(input,midi);DuckUiTestAccess::tick(*editors[size_t(i)]);}
    const double stamp=juce::Time::getMillisecondCounterHiRes();
    for(auto& editor:editors){const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,float(dpi));check(image.isValid(),"three-editor playback snapshot");}
    captureMs+=juce::Time::getMillisecondCounterHiRes()-stamp;pump(1);
   }
   for(auto& editor:editors){check(DuckUiTestAccess::scopeHasSignal(*editor),"all three native scopes show live signal");check(DuckUiTestAccess::plots(*editor)==std::array<std::uint64_t,2>{0,0},"Windows live graphs never allocate or composite software bloom layers");check(DuckUiTestAccess::fellBack(*editor),"three Windows editors retain native rendering");}
   std::cout<<"PASS Windows four-editor playback DPI="<<dpi<<" full_snapshot_batch_ms="<<captureMs/120.<<'\n';
  }
  const auto engines=editors[0]->getPeer()->getAvailableRenderingEngines();
  for(const auto& name:engines){
   // Switch ALL peers: changing only one leaves the other renderers active.
   for(auto& editor:editors){DuckUiTestAccess::renderer(*editor,name);editor->createComponentSnapshot(editor->getLocalBounds());}
   if(name.containsIgnoreCase("Direct2D")){
    for(auto& editor:editors){const auto builds=DuckUiTestAccess::nativeCaches(*editor);for(int i=0;i<3;++i)editor->createComponentSnapshot(editor->getLocalBounds());check(DuckUiTestAccess::nativeCaches(*editor)==builds,"unchanged native chrome is converted once, not each paint");}
   }
   std::atomic<bool> running{true};
   std::thread audio([&]{
    juce::AudioBuffer<float> block(4,800);juce::MidiBuffer events;std::uint64_t sample=0;
    auto deadline=std::chrono::steady_clock::now();
    while(running.load()){
     for(int n=0;n<800;++n,++sample){const double t=double(sample)/48000.;const float out=float(.4*std::sin(t*6.2831853*83)),key=float(.9*std::exp(-std::fmod(t,.25)/.045)*std::sin(t*6.2831853*110));block.setSample(0,n,out);block.setSample(1,n,out);block.setSample(2,n,key);block.setSample(3,n,key);}
     for(auto& processor:processors){juce::AudioBuffer<float> input(block);processor->processBlock(input,events);}
     deadline+=std::chrono::microseconds(16667);std::this_thread::sleep_until(deadline);
    }
   });
   heartbeatLast=juce::Time::getMillisecondCounterHiRes();heartbeatMax=0;heartbeatCount=0;
   const auto timer=SetTimer(nullptr,0,20,heartbeat);check(timer!=0,"native UI heartbeat timer created");
   pump(2000);KillTimer(nullptr,timer);running.store(false);audio.join();
   check(heartbeatCount>0,"four-editor playback services low-priority Windows messages");
   std::cout<<"PASS Windows four-editor real-time renderer="<<name<<" heartbeat_count="<<heartbeatCount<<" max_message_gap_ms="<<heartbeatMax<<'\n';
   // CI hardware varies; catch seconds-long starvation rather than enforce FPS.
   check(heartbeatMax<1000.,"four-editor playback does not starve UI messages for a second");
  }
  for(auto& editor:editors){editor->removeFromDesktop();editor.reset();}
#else
  const double start=juce::Time::getMillisecondCounterHiRes();
  for(int cycle=0;cycle<100;++cycle){
   if(cycle>0){e.reset(static_cast<DuckPocketAudioProcessorEditor*>(p.createEditor()));e->setSize(800,905);}
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
   check(DuckUiTestAccess::blurredFrames(*e)==0,"native GPU live graphs never run a blur pass");
   if(cycle==0){const auto previous=DuckUiTestAccess::gpuRevision(*e);DuckUiTestAccess::theme(*e,PocketTheme::Amber);for(int poll=0;poll<75&&!DuckUiTestAccess::themePresented(*e,previous);++poll)pump(20);check(DuckUiTestAccess::themePresented(*e,previous),"paused GPU presents the new theme without audio");}
   if(cycle==0){for(int fold=0;fold<10;++fold){const int width=e->getWidth();DuckUiTestAccess::collapse(*e,(fold%2)==0);
    for(int poll=0;poll<75&&DuckUiTestAccess::gpuHeight(*e)!=e->getHeight();++poll){DuckUiTestAccess::trigger(*e);pump(20);}
    check(e->getWidth()==width,"native fold preserves width");check(DuckUiTestAccess::gpuHeight(*e)==e->getHeight(),"native GPU uses live drawable height after paused fold");}}
   DuckUiTestAccess::gl(*e,false);e->removeFromDesktop();e.reset();
  }
  std::cout<<"PASS native OpenGL traces without bloom and 100 editor peer lifecycles, total_ms="<<juce::Time::getMillisecondCounterHiRes()-start<<'\n';
#endif
#else
  std::cout<<"SKIP OpenGL is disabled at compile time\n";
#endif
  return 0;
 }
 juce::Image edge(juce::Image::ARGB,1,1,true,juce::SoftwareImageType()),edgeBlur(juce::Image::ARGB,1,1,true,juce::SoftwareImageType());edge.setPixelAt(0,0,juce::Colours::white);
 PocketSoftwareGlow::blur(edge,edgeBlur,true,2);check(edgeBlur.getPixelAt(0,0)==juce::Colours::white,"single-pixel blur clamps edges safely");
 PocketSoftwareGlow::blur(edgeBlur,edge,false,2);check(edge.getPixelAt(0,0)==juce::Colours::white,"vertical blur also clamps edges safely");
 juce::Image line(juce::Image::ARGB,65,1,true,juce::SoftwareImageType()),smooth(line.createCopy());
 for(int x=0;x<65;++x)line.setPixelAt(x,0,juce::Colour(juce::uint8(x*3),juce::uint8(x*3),juce::uint8(x*3)));
 PocketSoftwareGlow::boxBlur(line,smooth,true,6);
 for(int x=7;x<58;++x)check(smooth.getPixelAt(x,0).getRed()-smooth.getPixelAt(x-1,0).getRed()==3,"bypass blur preserves a continuous gradient without a mosaic grid");
 PocketSoftwareGlow::boxBlur(edge,edgeBlur,true,12);check(edgeBlur.getPixelAt(0,0)==juce::Colours::white,"box blur safely clamps a one-pixel image");
 juce::Image emptyMask(juce::Image::ARGB,8,8,true,juce::SoftwareImageType());check(!PocketSoftwareGlow::hasEmission(emptyMask),"idle mask has no glow to composite");emptyMask.setPixelAt(4,4,juce::Colours::white);check(PocketSoftwareGlow::hasEmission(emptyMask),"signal mask enables the glow pass");
 PocketSoftwareGlow emptyGlow;emptyGlow.prepare(32,32);juce::Image glass(juce::Image::ARGB,32,32,true,juce::SoftwareImageType()),idleGlass(juce::Image::ARGB,32,32,true,juce::SoftwareImageType());
 for(int y=0;y<32;++y)for(int x=0;x<32;++x)glass.setPixelAt(x,y,juce::Colour(juce::uint8(x*7),juce::uint8(y*7),juce::uint8((x+y)*3)));{juce::Graphics g(idleGlass);g.drawImageAt(glass,0,0);emptyGlow.paint(g,glass,{0,0,32,32},1,1,1,false);}
 for(int y=0;y<32;++y)for(int x=0;x<32;++x)check(idleGlass.getPixelAt(x,y)==glass.getPixelAt(x,y),"empty glow preserves every cached glass pixel");
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
 HeaderValue outputField(dialLook,true);outputField.setRange(-12,6,.01);outputField.setValue(-.000003,juce::dontSendNotification);check(outputField.displayedValue()=="0.0 dB","header Output zero has no minus");outputField.setValue(-.1,juce::dontSendNotification);check(outputField.displayedValue()=="-0.1 dB","header Output uses one decimal");
 p.selectLookahead(1);DuckUiTestAccess::tick(*e);DuckUiTestAccess::attack(*e).setValue(5,juce::sendNotificationSync);
 p.selectLookahead(3);DuckUiTestAccess::tick(*e);
 check(DuckUiTestAccess::attack(*e).getValue()==5&&DuckUiTestAccess::attack(*e).getMaximum()==25,"Lookahead expands Attack range without changing ms");
 check(std::abs(DuckUiTestAccess::attack(*e).valueToProportionOfLength(5)-.20)<.0001,"5 of 25 ms occupies 20 percent of dial");
 p.selectLookahead(0);DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::attack(*e).getValue()==1&&DuckUiTestAccess::attack(*e).getMaximum()==1,"low-latency Attack range and value clamp together");
 p.selectLookahead(1);DuckUiTestAccess::tick(*e);DuckUiTestAccess::attack(*e).setValue(0,juce::sendNotificationSync);
 ModernDial attackDial(dialLook,"Attack","ms","ms",0,false,false,true);attackDial.setRange(0,5,.1);attackDial.setValue(.1,juce::dontSendNotification);check(attackDial.displayedValue()=="0.1","Attack value and ms unit use separate lines");
 auto* outParameter=p.parameters.getParameter("outputGain");outParameter->setValueNotifyingHost(outParameter->convertTo0to1(-.1f));pump(5);check(DuckUiTestAccess::header(*e,true).displayedValue()=="-0.1 dB","header Output follows host automation");DuckUiTestAccess::header(*e,true).setValue(0,juce::sendNotificationSync);
 auto* mixParameter=p.parameters.getParameter("mix");mixParameter->setValueNotifyingHost(mixParameter->convertTo0to1(50));pump(5);check(DuckUiTestAccess::header(*e,false).displayedValue()=="50%","header Mix follows host automation");DuckUiTestAccess::header(*e,false).setValue(25,juce::sendNotificationSync);check(p.parameters.getRawParameterValue("mix")->load()==25,"numeric Mix writes the attached parameter");DuckUiTestAccess::header(*e,false).setValue(100,juce::sendNotificationSync);
 p.parameters.getParameter("legacyAttack")->setValueNotifyingHost(1);DuckUiTestAccess::attack(*e).setValue(.1,juce::sendNotificationSync);check(std::abs(p.attackParameter().convertFrom0to1(p.attackParameter().getValue())-.1f)<.001f&&p.parameters.getRawParameterValue("legacyAttack")->load()==0,"editing Attack exits migrated compatibility mode");DuckUiTestAccess::attack(*e).setValue(0,juce::sendNotificationSync);
 for(int width:{400,615,800,1500}){e->setSize(width,juce::roundToInt(width*905./800.));check(DuckUiTestAccess::layout(*e),"controls remain contained and separate during resize");check(!DuckUiTestAccess::header(*e,false).isVisible(),"Mix is hidden from the editor");}
 DuckUiTestAccess::collapse(*e,false);check(!DuckUiTestAccess::rangesVisible(*e)&&e->getHeight()<int(e->getWidth()*905./800.),"collapse hides both filters and shortens the window");
 DuckUiTestAccess::collapse(*e,true);check(DuckUiTestAccess::rangesVisible(*e),"expand restores both filters");
 p.parameters.getParameter("bypass")->setValueNotifyingHost(1);DuckUiTestAccess::tick(*e);
 DuckUiTestAccess::collapse(*e,false);DuckUiTestAccess::settle(*e);DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::bypassOverlay(*e),"collapse during bypass rebuilds the resized overlay");
 p.parameters.getParameter("bypass")->setValueNotifyingHost(0);DuckUiTestAccess::tick(*e);DuckUiTestAccess::collapse(*e,true);
 for(int width:{400,615,1500}){
  DuckUiTestAccess::collapse(*e,true);e->setSize(width,juce::roundToInt(width*905./800.));DuckUiTestAccess::settle(*e);
  const auto before=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);const auto cache=DuckUiTestAccess::caches(*e);
  const int topHeight=juce::roundToInt(744.f*float(width)/800.f*2.f);
  for(int fold=0;fold<4;++fold){DuckUiTestAccess::collapse(*e,(fold%2)!=0);
   const auto after=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);
   check(e->getWidth()==width,"fold never changes width at the minimum or maximum size");
   check(DuckUiTestAccess::caches(*e)==cache,"fold reuses chrome immediately, without settling or regeneration");
   juce::Image::BitmapData a(before,juce::Image::BitmapData::readOnly),b(after,juce::Image::BitmapData::readOnly);
   const auto grip=(DuckUiTestAccess::resizeGrip(*e).toFloat()*2.f).toNearestInt();
   for(int y=0;y<topHeight;++y)for(int x=0;x<a.width;++x)if(!grip.contains(x,y)&&!samePixel(a.getPixelColour(x,y),b.getPixelColour(x,y))){std::cerr<<"fold diff width="<<width<<" fold="<<fold<<" pixel="<<x<<","<<y<<" before="<<a.getPixelColour(x,y).toString()<<" after="<<b.getPixelColour(x,y).toString()<<" height="<<e->getHeight()<<"\n";check(false,"paused top/graph pixels stay identical through fold");}
  }
 }
 e->setSize(800,905);check(DuckUiTestAccess::widthResizePending(*e),"real width resize still schedules a rebuild while transport is idle");DuckUiTestAccess::settle(*e);
 DuckUiTestAccess::freeze(*e);pump(10);check(DuckUiTestAccess::frozen(*e),"freeze button freezes both graphs");
 DuckUiTestAccess::freeze(*e);pump(10);check(!DuckUiTestAccess::frozen(*e),"freeze button resumes both graphs");
 auto& balance= DuckUiTestAccess::balance(*e);
 check(balance.getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag,"M/S uses a rotary control");
 for(double value:{-1.,-.25,0.,.25,1.}){
  auto* param=p.parameters.getParameter("msBalance");param->setValueNotifyingHost(param->convertTo0to1(float(value)));pump(5);
  check(std::abs(balance.getValue()-value)<.00001,"M/S knob follows host automation");
  check(balance.displayedValue()==juce::String(juce::roundToInt(std::abs(value)*100.))+"%","M/S magnitude display");
  check(balance.balanceLabel()==(value<0?"mid":(value>0?"side":"M/S")),"M/S direction display");
 }
 balance.setValue(0,juce::sendNotificationSync);check(std::abs(p.parameters.getRawParameterValue("msBalance")->load())<.00001f,"M/S gesture returns parameter to neutral");
 // Reproduce host stop/reset after a freeze/resume cut-off at >1 second.
 juce::AudioBuffer<float> resumed(4,64);juce::MidiBuffer resumeMidi;
 for(int i=0;i<1200;++i){resumed.clear();p.processBlock(resumed,resumeMidi);}DuckUiTestAccess::tick(*e);
 DuckUiTestAccess::freeze(*e);pump(10);DuckUiTestAccess::freeze(*e);pump(10);
 const auto historyBeforeReset=DuckUiTestAccess::historyCounts(*e);check(historyBeforeReset[0]>0&&historyBeforeReset[1]>0,"histories populated before host reset");p.reset();DuckUiTestAccess::tick(*e);check(DuckUiTestAccess::historyCounts(*e)==historyBeforeReset,"host reset preserves both graph histories");
 for(int block=0;block<150;++block){for(int sample=0;sample<64;++sample){const float v=.3f*std::sin(float(block*64+sample)*.01f);for(int channel=0;channel<4;++channel)resumed.setSample(channel,sample,v);}p.processBlock(resumed,resumeMidi);}DuckUiTestAccess::tick(*e);
 e->createComponentSnapshot(e->getLocalBounds());check(DuckUiTestAccess::scopeHasSignal(*e),"audio after stop/reset resumes visible scope immediately");
#if JUCE_WINDOWS
 check(DuckUiTestAccess::plots(*e)==std::array<std::uint64_t,2>{0,0},"Windows playback bypasses all CPU bloom buffers");
#endif
 const PocketTheme themes[]{PocketTheme::Neon,PocketTheme::SolidDark,PocketTheme::Amber};const char* names[]{"neon","dark","amber"};
 {
  auto fadeProcessor=std::make_unique<DuckPocketAudioProcessor>();
  std::unique_ptr<DuckPocketAudioProcessorEditor> fadeEditor(static_cast<DuckPocketAudioProcessorEditor*>(fadeProcessor->createEditor()));
  for(auto theme:themes){DuckUiTestAccess::theme(*fadeEditor,theme);for(bool gain:{false,true}){
   const auto image=DuckUiTestAccess::ageFade(*fadeEditor,gain);std::uint64_t left=0,right=0;
   for(int y=0;y<107;++y)for(int x=50;x<150;++x){left+=image.getPixelAt(x,y).getAlpha();right+=image.getPixelAt(x+500,y).getAlpha();}
   check(left>0&&right>left*2,"both graph cores fade towards older data in every theme");
  }}
 }
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
 e->createComponentSnapshot({50,423,200,100});auto after=DuckUiTestAccess::plots(*e);check(after[0]==before[0]&&after[1]==before[1],"gain-only clip bypasses glow buffers and leaves oscilloscope untouched");

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
  e->setSize(width,juce::roundToInt(width*905./800.));DuckUiTestAccess::settle(*e);
  auto image=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);
  auto stream=output.getChildFile("compact-"+juce::String(width)+"-2x.png").createOutputStream();check(stream&&juce::PNGImageFormat().writeImageToStream(image,*stream),"compact review capture");
  stream.reset();DuckUiTestAccess::collapse(*e,false);auto closed=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);
  auto closedStream=output.getChildFile("collapsed-"+juce::String(width)+"-2x.png").createOutputStream();check(closedStream&&juce::PNGImageFormat().writeImageToStream(closed,*closedStream),"collapsed review capture");
  DuckUiTestAccess::collapse(*e,true);
 }
 DuckUiTestAccess::collapse(*e,false);p.parameters.getParameter("bypass")->setValueNotifyingHost(1);DuckUiTestAccess::tick(*e);auto bypassImage=e->createComponentSnapshot(e->getLocalBounds(),true,2.f);auto bypassStream=output.getChildFile("bypass-615-2x.png").createOutputStream();check(bypassStream&&juce::PNGImageFormat().writeImageToStream(bypassImage,*bypassStream),"smooth bypass review capture");bypassStream.reset();p.parameters.getParameter("bypass")->setValueNotifyingHost(0);
 e.reset();std::cout<<"PASS real JUCE theme captures and chrome reuse\n";
}

