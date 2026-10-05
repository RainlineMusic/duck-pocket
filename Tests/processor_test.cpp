#include "PluginProcessor.h"
#include <iostream>
#include <cstdlib>
#include <thread>
static void check(bool b,const char* m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::abort();}}
static void set(DuckPocketAudioProcessor& p,const char* id,float value){auto* a=p.parameters.getParameter(id);a->setValueNotifyingHost(a->convertTo0to1(value));}
struct LatencyListener:juce::AudioProcessorListener {
 int changes=0;
 void audioProcessorParameterChanged(juce::AudioProcessor*,int,float) override {}
 void audioProcessorChanged(juce::AudioProcessor*,const ChangeDetails& details) override {if(details.latencyChanged)++changes;}
};
int main(){juce::ScopedJuceInitialiser_GUI init;
 auto processorStorage=std::make_unique<DuckPocketAudioProcessor>();auto& p=*processorStorage;auto fresh=std::make_unique<DuckPocketAudioProcessor>();juce::MidiBuffer midi;
 for(double invalid:{0.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){p.setRateAndBufferSizeDetails(invalid,64);p.prepareToPlay(invalid,64);check(p.getLatencySamples()==pocket::Engine::latencyForRate(invalid,10.f),"invalid host rate safe");}

 auto unsupported=p.getBusesLayout();unsupported.inputBuses.set(0,juce::AudioChannelSet::quadraphonic());unsupported.outputBuses.set(0,juce::AudioChannelSet::quadraphonic());check(!p.isBusesLayoutSupported(unsupported),"multichannel main bus rejected");
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.})for(int channels:{1,2})for(int keyChannels:{0,1,2}){
  auto layout=p.getBusesLayout();layout.inputBuses.set(0,channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());layout.outputBuses.set(0,layout.inputBuses[0]);layout.inputBuses.set(1,keyChannels==0?juce::AudioChannelSet::disabled():(keyChannels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo()));
  check(p.setBusesLayout(layout),"supported buses");p.setRateAndBufferSizeDetails(sr,128);p.prepareToPlay(sr,128);check(p.getLatencySamples()==int(std::ceil(sr*.010)),"reported latency");
  for(int size:{0,1,17,128,3,4096,65536}){juce::AudioBuffer<float> b(channels+keyChannels,size);b.clear();p.processBlock(b,midi);for(int c=0;c<channels;++c)for(int n=0;n<size;++n)check(std::isfinite(b.getSample(c,n)),"variable block finite");}
  set(p,"relativeDuration",0);set(p,"amount",0);p.reset();const int latency=p.getLatencySamples();int position=0;
  for(int size:{17,3,128,1,4096}){juce::AudioBuffer<float> b(channels+keyChannels,size);b.clear();for(int c=0;c<channels;++c)for(int n=0;n<size;++n)b.setSample(c,n,position+n==0?.5f:0.f);p.processBlock(b,midi);for(int c=0;c<channels;++c)for(int n=0;n<size;++n){const float expected=position+n==latency?.5f:0.f,actual=b.getSample(c,n);if(std::abs(actual-expected)>1e-6f){std::cerr<<"dry impulse SR="<<sr<<" channels="<<channels<<" key="<<keyChannels<<" sample="<<position+n<<" latency="<<latency<<" actual="<<actual<<" expected="<<expected<<" outputParam="<<p.parameters.getRawParameterValue("outputGain")->load()<<'\n';}check(std::abs(actual-expected)<=1e-6f,"latency-aligned dry across blocks");}position+=size;}
 }
 auto old=juce::ValueTree("PARAMETERS");for(auto id:{"amount","duration"}){auto n=juce::ValueTree("PARAM");n.setProperty("id",id,nullptr);n.setProperty("value",juce::String(id)=="duration"?123.f:70.f,nullptr);old.appendChild(n,nullptr);}old.setProperty("uiExpanded",true,nullptr);old.setProperty("uiWidth",1120,nullptr);
 juce::MemoryBlock state;juce::AudioProcessor::copyXmlToBinary(*old.createXml(),state);p.setStateInformation(state.getData(),int(state.getSize()));check(p.parameters.getRawParameterValue("relativeDuration")->load()==0,"old sessions use legacy duration");check(p.parameters.getRawParameterValue("duration")->load()==123,"old duration preserved");check(p.editorWidth.load()==1120,"old width preserved");
 check(p.getLookaheadMs()==5,"pre-lookahead projects retain 5 ms");check(p.parameters.getRawParameterValue("mix")->load()==100,"old sessions default to full Mix");check(p.parameters.getRawParameterValue("legacyAttack")->load()==1,"old sessions retain their original soft attack");
 p.getStateInformation(state);auto restoredStorage=std::make_unique<DuckPocketAudioProcessor>();auto& restored=*restoredStorage;restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.parameters.getRawParameterValue("duration")->load()==123,"state roundtrip");check(restored.parameters.getRawParameterValue("relativeDuration")->load()==0,"legacy mode roundtrip");check(fresh->parameters.getRawParameterValue("durationPercent")->load()==100,"new default 100 percent");
 set(p,"mix",50);set(p,"attack",3.2f);set(p,"legacyAttack",0);p.getStateInformation(state);restored.setStateInformation(state.getData(),int(state.getSize()));check(restored.parameters.getRawParameterValue("mix")->load()==50&&std::abs(restored.parameters.getRawParameterValue("attack")->load()-3.2f)<.001f,"Mix and Attack state roundtrip");check(fresh->parameters.getRawParameterValue("attackMs")->load()==5&&fresh->getLookaheadMs()==10,"new defaults: Attack 5 ms, Lookahead 10 ms");
 // Per-instance latency choice, immediate host notification, old automation mapping.
 auto variable=std::make_unique<DuckPocketAudioProcessor>();auto other=std::make_unique<DuckPocketAudioProcessor>();
 variable->setRateAndBufferSizeDetails(48000,64);variable->prepareToPlay(48000,64);
 LatencyListener listener;variable->addListener(&listener);
 set(*variable,"attackMs",5.f);variable->selectLookahead(3);
 check(variable->getLatencySamples()==1200&&listener.changes>0,"Lookahead immediately notifies the host of 25 ms latency");
 check(variable->parameters.getRawParameterValue("attackMs")->load()==5.f,"increasing Lookahead preserves Attack milliseconds");
 check(other->getLookaheadMs()==10,"Lookahead is not a global preference");
 variable->getStateInformation(state);other->setStateInformation(state.getData(),int(state.getSize()));
 check(other->getLookaheadMs()==25&&other->parameters.getRawParameterValue("attackMs")->load()==5.f,"Lookahead and Attack roundtrip per instance");
 variable->selectLookahead(0);check(variable->getLatencySamples()==48&&variable->parameters.getRawParameterValue("attackMs")->load()==1.f,"1 ms low-latency option clamps Attack");
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.})for(int index=0;index<4;++index){
  variable->selectLookahead(index);variable->setRateAndBufferSizeDetails(sr,64);variable->prepareToPlay(sr,64);
  check(variable->getLatencySamples()==pocket::Engine::latencyForRate(sr,float(variable->getLookaheadMs())),"all choices report correct latency after sample-rate changes");
 }
 // Both removed v4 choices migrate to the new longest horizon.
 for(float oldIndex:{3.f,4.f}){auto oldChoice=variable->parameters.copyState();oldChoice.setProperty("schemaVersion",4,nullptr);oldChoice.getChildWithProperty("id","lookahead").setProperty("value",oldIndex,nullptr);oldChoice.getChildWithProperty("id","attackMs").setProperty("value",40.f,nullptr);juce::AudioProcessor::copyXmlToBinary(*oldChoice.createXml(),state);other->setStateInformation(state.getData(),int(state.getSize()));check(other->getLookaheadMs()==25&&other->parameters.getRawParameterValue("attackMs")->load()==25,"removed choices migrate and clamp Attack");}
 variable->removeListener(&listener);
 check(!p.usesExtendedAttack(),"migrated sessions retain the original Attack automation mapping");
 p.selectLookahead(3);check(p.usesExtendedAttack()&&std::abs(p.parameters.getRawParameterValue("attackMs")->load()-3.2f)<.001f,"opting into extended lookahead preserves migrated Attack milliseconds");
 const char bad[]="broken XML";p.setStateInformation(bad,sizeof bad);p.setStateInformation(nullptr,0);check(p.parameters.getRawParameterValue("duration")->load()==123,"corrupt state is ignored");
 p.setRateAndBufferSizeDetails(48000,64);p.prepareToPlay(48000,64);p.editorOpen.store(true);juce::AudioBuffer<float> b(p.getTotalNumInputChannels(),4096);b.clear();p.processBlock(b,midi);PocketTrace v;check(p.popTrace(v),"trace produced");const auto epoch=v.generation;while(p.popTrace(v)){}const double beforeReset=v.time;p.reset();p.processBlock(b,midi);bool found=false;while(p.popTrace(v)){found=true;check(v.generation==epoch&&v.time>beforeReset,"host reset preserves display timeline");}check(found,"trace continues after reset");const double beforePrepare=v.time;p.prepareToPlay(48000,64);p.processBlock(b,midi);found=false;while(p.popTrace(v)){found=true;check(v.generation==epoch&&v.time>beforePrepare,"host prepare preserves display timeline");}check(found,"trace continues after prepare");
 auto scaledStorage=std::make_unique<DuckPocketAudioProcessor>();auto& scaled=*scaledStorage;scaled.setRateAndBufferSizeDetails(48000,64);scaled.prepareToPlay(48000,64);scaled.editorOpen.store(true);set(scaled,"mix",50);
 juce::AudioBuffer<float> scaleBuffer(4,4096);for(int i=0;i<4096;++i){scaleBuffer.setSample(0,i,.4f);scaleBuffer.setSample(1,i,.4f);scaleBuffer.setSample(2,i,1.f-std::pow(10.f,-15.f/20.f));scaleBuffer.setSample(3,i,scaleBuffer.getSample(2,i));}scaled.processBlock(scaleBuffer,midi);
 PocketTrace scaledTrace;float shownGain=1;while(scaled.popTrace(scaledTrace))shownGain=scaledTrace.gain;check(std::abs(20*std::log10(shownGain)+7.5f)<.001f,"graph packets carry actual Mix-scaled reduction");
 // Embedded duck: all requested rates, mono/stereo, variable blocks, retrigger,
 // bypass output and clean termination. No effect until explicitly requested.
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.,32000.})for(int channels:{1,2}){
  auto quack=std::make_unique<DuckPocketAudioProcessor>();auto buses=quack->getBusesLayout();buses.inputBuses.set(0,channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo());buses.outputBuses.set(0,buses.inputBuses[0]);buses.inputBuses.set(1,juce::AudioChannelSet::disabled());check(quack->setBusesLayout(buses),"duck bus layout");
  quack->setRateAndBufferSizeDetails(sr,127);quack->prepareToPlay(sr,127);juce::AudioBuffer<float> audio(channels,127);audio.clear();quack->processBlock(audio,midi);check(audio.getMagnitude(0,127)==0,"duck silent before double click");
  const int originalLatency=quack->getLatencySamples();quack->playDuck();quack->playDuck();float peak=0;double energy=0;
  for(int block=0;block<int(sr/127)+2;++block){audio.clear();quack->processBlockBypassed(audio,midi);for(int i=0;i<127;++i){float v=audio.getSample(0,i);check(std::isfinite(v),"duck samples finite");energy+=double(v)*v;peak=juce::jmax(peak,std::abs(v));if(channels==2)check(v==audio.getSample(1,i),"duck centered stereo");}}
  check(energy>1&&peak>0&&peak<1,"duck audible without amplification or stacked triggers");check(audio.getMagnitude(0,127)==0,"duck finishes without a tail");check(quack->getLatencySamples()==originalLatency,"duck does not change host latency");
 }
 std::atomic<bool> done{false};std::thread producer([&]{juce::AudioBuffer<float> samples(p.getTotalNumInputChannels(),64);samples.clear();for(int i=0;i<3000;++i){if(i==1500)p.reset();p.processBlock(samples,midi);}done.store(true,std::memory_order_release);});
 std::uint32_t previousGeneration=0;double previousTime=-1;int packets=0;
 auto consume=[&]{while(p.popTrace(v)){check(std::isfinite(v.time)&&std::isfinite(v.gain)&&std::isfinite(v.outHi)&&std::isfinite(v.keyLo),"untorn concurrent trace");if(v.generation==previousGeneration)check(v.time>=previousTime,"ordered concurrent trace");else previousTime=-1;previousGeneration=v.generation;previousTime=v.time;++packets;}};
 while(!done.load(std::memory_order_acquire)){consume();std::this_thread::yield();}producer.join();consume();check(packets>100,"concurrent trace exercised");
 for(int i=0;i<100;++i){std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());editor->setSize(800+i%2*700,633+i%2*555);}
 std::cout<<"PASS processor buses, variable blocks, latency, state migration, trace resets, concurrent trace exchange, 100 editor lifecycles\n";
}
