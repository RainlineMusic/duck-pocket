#include "PluginProcessor.h"
#include "license_fixture.h"
#include <iostream>
#include <cstdlib>
struct DuckLicenseTestAccess {static void active(DuckPocketAudioProcessor& p,bool b){p.license->active.store(b);}};
static void check(bool b,const char* m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::abort();}}
int main(){
 juce::ScopedJuceInitialiser_GUI init;
 check(pocket::verifyLicense(testLicense,testModulus),"standard RSA SHA256 signature accepted");
 check(!pocket::verifyLicense(testLicense),"test key cannot activate production");
 auto altered=juce::String(testLicense).replaceSection(10,1,"f");
 check(!pocket::verifyLicense(altered,testModulus),"altered payload rejected");
 check(!pocket::verifyLicense(juce::String(testLicense).dropLastCharacters(1)+(juce::String(testLicense).endsWith("0")?"1":"0"),testModulus),"altered signature rejected");
 check(!pocket::verifyLicense("DP1.invalid")&&!pocket::verifyLicense(juce::String::repeatedString("a",8192)),"malformed keys rejected");
 auto p=std::make_unique<DuckPocketAudioProcessor>();DuckLicenseTestAccess::active(*p,false);
 juce::String error;check(!p->activateLicense(testLicense,error),"foreign signing key not persisted");
 p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
 juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
 for(int block=0;block<30;++block){for(int n=0;n<256;++n){audio.setSample(0,n,.2f);audio.setSample(1,n,.3f);audio.setSample(2,n,.9f);audio.setSample(3,n,.9f);}p->processBlock(audio,midi);}
 check(std::abs(audio.getSample(0,255)-.2f)<1e-5&&std::abs(audio.getSample(1,255)-.3f)<1e-5,"unlicensed latency-aligned dry output");
 DuckLicenseTestAccess::active(*p,true);p->listenSidechain.store(true);
 p->parameters.getParameter("scLow")->setValueNotifyingHost(0);
 p->parameters.getParameter("scHigh")->setValueNotifyingHost(1);
 double heard=0;
 for(int block=0;block<30;++block){for(int n=0;n<256;++n){const float key=.4f*std::sin(float(block*256+n)*.13f);audio.setSample(0,n,0);audio.setSample(1,n,0);audio.setSample(2,n,key);audio.setSample(3,n,key);}p->processBlock(audio,midi);heard+=audio.getMagnitude(0,256);}
 check(heard>1,"post-filter sidechain audible");
 p->listenSidechain.store(false);
 for(int block=0;block<30;++block){audio.clear();p->processBlock(audio,midi);}
 check(audio.getMagnitude(0,256)<1e-5,"monitor returns to normal output");
 juce::MemoryBlock state;p->getStateInformation(state);DuckLicenseTestAccess::active(*p,false);p->setStateInformation(state.getData(),int(state.getSize()));check(!p->isActivated(),"project state cannot activate");
 std::cout<<"PASS license signatures, tampering, unlicensed dry, monitor, state\n";
}
