#pragma once
#include <JuceHeader.h>
#include "LicensePublicKey.h"

namespace pocket {
// Signed licenses, never a trusted activation boolean in project state.
inline bool verifyLicense(const juce::String& input,const char* publicModulus=licenseModulus) {
    const auto key=input.trim();
    if(key.length()!=553||!key.startsWith("DP1."))return false;
    const auto id=key.substring(4,40),signature=key.substring(41);
    if(key[40]!='.'||!id.containsOnly("0123456789abcdef-")||juce::Uuid(id).toDashedString()!=id
       ||signature.length()!=512||!signature.containsOnly("0123456789abcdef"))return false;
    const auto payload=juce::String("DuckPocket|1|")+id;
    const auto hash=juce::SHA256(payload.toRawUTF8(),size_t(payload.getNumBytesAsUTF8())).toHexString();
    // EMSA-PKCS1-v1_5: 00 01 FF..FF 00 SHA-256 DigestInfo digest.
    juce::String em="0001";
    for(int i=0;i<202;++i)em+="ff";
    em+="003031300d060960864801650304020105000420"+hash;
    juce::BigInteger value,modulus,exponent,expected;
    value.parseString(signature,16);modulus.parseString(publicModulus,16);
    if(value.isZero()||value>=modulus)return false;
    exponent=65537;expected.parseString(em,16);value.exponentModulo(exponent,modulus);
    return value==expected;
}
class LicenseState : private juce::Timer {
public:
    std::atomic<bool> active{false};
    static juce::File file() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("RainlineMusic/DuckPocket/license.key");
    }
    LicenseState(){refresh();startTimer(2000);}
    ~LicenseState() override {stopTimer();}
    bool activate(const juce::String& key,juce::String& error){
        if(!verifyLicense(key)){error="Invalid Duck Pocket license key.";return false;}
        const auto target=file();
        if(!target.getParentDirectory().createDirectory()){error="Cannot create license folder.";return false;}
        juce::TemporaryFile temporary(target);
        if(!temporary.getFile().replaceWithText(key.trim())||!temporary.overwriteTargetFileWithTemporary()){
            error="Cannot save license. Check folder permissions.";return false;
        }
        refresh();return active.load(std::memory_order_acquire);
    }
private:
    juce::String cached;
    bool checked=false;
    void refresh(){
        const auto target=file();
        // Bound untrusted input and verify only when its contents change.
        const auto key=target.getSize()<=1024?target.loadFileAsString().trim():juce::String();
        if(!checked||key!=cached){cached=key;checked=true;active.store(verifyLicense(key),std::memory_order_release);}
    }
    void timerCallback() override {refresh();}
};
}
