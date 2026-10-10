#include "PluginProcessor.h"
#include "license_fixture.h"
#include <iostream>
#include <cstdlib>
struct DuckLicenseTestAccess {static void active(DuckPocketAudioProcessor& p,bool b){p.license->active.store(b);}};
static void check(bool b,const char* m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::abort();}}
int main(){
 juce::ScopedJuceInitialiser_GUI init;
 check(pocket::verifyLicense(testLicense,testDevice,testModulus),"standard RSA SHA256 signature accepted");
 check(!pocket::verifyLicense(testLicense,testDevice),"test key cannot activate production");
 auto altered=juce::String(testLicense).replaceSection(10,1,"f");
 check(!pocket::verifyLicense(altered,testDevice,testModulus),"altered payload rejected");
 check(!pocket::verifyLicense(juce::String(testLicense).dropLastCharacters(1)+(juce::String(testLicense).endsWith("0")?"1":"0"),testDevice,testModulus),"altered signature rejected");
 check(!pocket::verifyLicense("DP1.invalid",testDevice)&&!pocket::verifyLicense(juce::String::repeatedString("a",8192),testDevice),"malformed keys rejected");
 check(pocket::deviceCodeFromSystemId("fixture-machine","test")==testDevice,"cross-language numeric device hash");
 check(pocket::canonicalDeviceCode(pocket::displayDeviceCode(testDevice))==testDevice,"grouped numeric code roundtrip");
 check(pocket::canonicalDeviceCode(juce::String(testDevice).dropLastCharacters(1)+(juce::String(testDevice).endsWith("0")?"1":"0")).isEmpty(),"device typo checksum");
 check(!pocket::verifyLicense(testLicense,pocket::deviceCodeFromSystemId("other-machine","test"),testModulus),"copied license rejected on another device");
 check(!pocket::verifyLicense(testLicense,juce::String(),testModulus),"missing system ID cannot activate");
 check(pocket::canonicalOnlineKey("duck-7k3m-9x2p-6r8n-4w5t")=="DUCK7K3M9X2P6R8N4W5T"&&pocket::canonicalOnlineKey("DUCK-invalid").isEmpty(),"short key syntax");
 auto reply=std::make_unique<juce::DynamicObject>();reply->setProperty("valid",false);reply->setProperty("error","revoked");reply->setProperty("license_id",juce::String(testLicense).substring(4,40));reply->setProperty("device",testDevice);reply->setProperty("token_hash",juce::SHA256(testLicense,size_t(juce::String(testLicense).getNumBytesAsUTF8())).toHexString());juce::var response(reply.release());
 check(pocket::verificationRevoked(response,testLicense,testDevice,200),"explicit revocation bound to token");
 check(!pocket::verificationRevoked(response,testLicense,testDevice,503),"server error preserves offline activation");
 check(!pocket::verificationRevoked(response,testLicense,"another-device",200),"unrelated response cannot revoke");
 check(!pocket::verificationRevoked(juce::var(),testLicense,testDevice,200),"malformed response keeps activation");
 response.getDynamicObject()->setProperty("valid",true);check(pocket::verificationResult(response,testLicense,testDevice,200)==pocket::VerificationResult::valid,"matching valid reply permits restored activation");
 check(pocket::verificationResult(response,testLicense,testDevice,403)==pocket::VerificationResult::offline,"HTTP denial cannot validate activation");
 check(pocket::verificationResult(response,testLicense,testDevice,429)==pocket::VerificationResult::offline,"rate limit preserves offline state");
 response.getDynamicObject()->setProperty("token_hash","wrong");check(pocket::verificationResult(response,testLicense,testDevice,200)==pocket::VerificationResult::offline,"wrong token hash cannot restore");
 response.getDynamicObject()->setProperty("token_hash",juce::SHA256(testLicense,size_t(juce::String(testLicense).getNumBytesAsUTF8())).toHexString());
 response.getDynamicObject()->setProperty("valid",false);
 const auto revokedHash=juce::SHA256(testLicense,size_t(juce::String(testLicense).getNumBytesAsUTF8())).toHexString();
 check(pocket::licenseMarkedRevoked(testLicense,revokedHash),"revoked token stays marked on reimport");
 check(!pocket::licenseMarkedRevoked(testLicense,"")&&!pocket::licenseMarkedRevoked("another-token",revokedHash),"marker applies only to exact token");
 response.getDynamicObject()->setProperty("valid","false");check(!pocket::verificationRevoked(response,testLicense,testDevice,200),"nonboolean validity ignored");
 auto p=std::make_unique<DuckPocketAudioProcessor>();DuckLicenseTestAccess::active(*p,false);
 juce::String error;check(!p->activateLicense(testLicense,error),"foreign signing key not persisted");
 auto temp=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("duck-invalid-license",".ducklicense");
 temp.replaceWithText(testLicense);check(!p->importLicenseFile(temp,error),"foreign file cannot activate");
 temp.replaceWithText(juce::String::repeatedString("a",2048));check(!p->importLicenseFile(temp,error),"oversized license rejected");temp.deleteFile();
 check(!p->startOnlineActivation("invalid",error)&&!p->onlineActivationBusy(),"invalid online key does not start network");
#if ! JUCE_WINDOWS && ! JUCE_MAC
 check(!p->startOnlineActivation("DUCK-7K3M-9X2P-6R8N-4W5T",error),"unconfigured API fails locally");
#endif
 p->setRateAndBufferSizeDetails(48000,256);p->prepareToPlay(48000,256);
 juce::AudioBuffer<float> audio(4,256);juce::MidiBuffer midi;
 for(int block=0;block<30;++block){for(int n=0;n<256;++n){audio.setSample(0,n,.2f);audio.setSample(1,n,.3f);audio.setSample(2,n,.9f);audio.setSample(3,n,.9f);}p->processBlock(audio,midi);}
 check(std::abs(audio.getSample(0,255)-.2f)<1e-5&&std::abs(audio.getSample(1,255)-.3f)<1e-5,"unlicensed latency-aligned dry output");
 DuckLicenseTestAccess::active(*p,true);p->listenSidechain.store(true);
 p->prepareToPlay(48000,256);check(p->listenSidechain.load(),"transport preparation preserves audition toggle");
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

