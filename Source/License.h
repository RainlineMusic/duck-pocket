#pragma once
#include <JuceHeader.h>
#include "LicensePublicKey.h"
#include "LicenseConfig.h"

namespace pocket {
// 128 bits of a product-scoped hash, shown as decimal digits + typo checksum.
inline juce::String deviceCodeFromSystemId(const juce::String& systemId,const juce::String& platform) {
    const auto id=systemId.trim().toLowerCase();
    if(id.isEmpty()||id.removeCharacters("-").containsOnly("0")||id.removeCharacters("-").containsOnly("f"))return {};
    const auto payload="DuckPocket|device|2|"+platform+"|"+id;
    const auto digest=juce::SHA256(payload.toRawUTF8(),size_t(payload.getNumBytesAsUTF8())).toHexString();
    juce::BigInteger number;number.parseString(digest.substring(0,32),16);
    const auto body="2"+number.toString(10,39);
    const auto checksum=juce::SHA256(body.toRawUTF8(),size_t(body.getNumBytesAsUTF8())).toHexString().substring(0,4).getHexValue32()%97;
    return body+juce::String(checksum).paddedLeft('0',2);
}
inline juce::String canonicalDeviceCode(const juce::String& input) {
    if(input.length()>64)return {};
    const auto code=input.removeCharacters(" -\r\n\t");
    if(code.length()!=42||code[0]!='2'||!code.containsOnly("0123456789"))return {};
    const auto body=code.substring(0,40);
    juce::BigInteger value,maximum;value.parseString(body.substring(1),10);maximum.parseString("ffffffffffffffffffffffffffffffff",16);
    if(value>maximum)return {};
    const auto checksum=juce::SHA256(body.toRawUTF8(),size_t(body.getNumBytesAsUTF8())).toHexString().substring(0,4).getHexValue32()%97;
    return code.substring(40)==juce::String(checksum).paddedLeft('0',2)?code:juce::String();
}
inline juce::String localDeviceCode() {
#if JUCE_MAC
    return deviceCodeFromSystemId(juce::SystemStats::getUniqueDeviceID(),"mac");
#elif JUCE_WINDOWS
    return deviceCodeFromSystemId(juce::SystemStats::getUniqueDeviceID(),"win");
#else
    return deviceCodeFromSystemId(juce::SystemStats::getUniqueDeviceID(),"other");
#endif
}
inline juce::String displayDeviceCode(const juce::String& code) {
    juce::String result;for(int i=0;i<code.length();i+=6){if(i)result+="-";result+=code.substring(i,i+6);}return result;
}
inline juce::String canonicalOnlineKey(const juce::String& input) {
    if(input.length()>64)return {};
    const auto key=input.removeCharacters(" -\r\n\t").toUpperCase();
    return key.length()==20&&key.startsWith("DUCK")&&key.substring(4).containsOnly("0123456789ABCDEFGHJKMNPQRSTVWXYZ")?key:juce::String();
}
inline bool verifyLicense(const juce::String& input,const juce::String& deviceCode,const char* publicModulus=licenseModulus) {
    if(input.length()>1024)return false;
    const auto key=input.trim();
    const auto device=canonicalDeviceCode(deviceCode);
    if(key.length()!=596||!key.startsWith("DP2.")||device.isEmpty())return false;
    const auto id=key.substring(4,40),boundDevice=key.substring(41,83),signature=key.substring(84);
    if(key[40]!='.'||key[83]!='.'||boundDevice!=device||!id.containsOnly("0123456789abcdef-")||juce::Uuid(id).toDashedString()!=id
       ||signature.length()!=512||!signature.containsOnly("0123456789abcdef"))return false;
    const auto payload="DuckPocket|2|"+id+"|"+boundDevice;
    const auto hash=juce::SHA256(payload.toRawUTF8(),size_t(payload.getNumBytesAsUTF8())).toHexString();
    juce::String em="0001";for(int i=0;i<202;++i)em+="ff";
    em+="003031300d060960864801650304020105000420"+hash;
    juce::BigInteger value,modulus,exponent,expected;
    value.parseString(signature,16);modulus.parseString(publicModulus,16);
    if(value.isZero()||value>=modulus)return false;
    exponent=65537;expected.parseString(em,16);value.exponentModulo(exponent,modulus);
    return value==expected;
}
// Own the cancellable request independently of the editor. Never detach a thread
// into an unloaded plugin, and never hold our lock while connecting/reading.
class LicenseRequest final : private juce::Thread {
public:
    LicenseRequest(juce::String endpoint,juce::String key,juce::String code):Thread("Duck activation"),api(std::move(endpoint)),shortKey(std::move(key)),device(std::move(code)){startThread();}
    ~LicenseRequest() override {signalThreadShouldExit();std::shared_ptr<juce::WebInputStream> s;{const juce::ScopedLock l(lock);s=stream;}if(s)s->cancel();stopThread(-1);}
    std::atomic<bool> done{false};
    juce::String license,error; // read only after acquire(done), no UI callbacks
private:
    juce::String api,shortKey,device;
    juce::CriticalSection lock;
    std::shared_ptr<juce::WebInputStream> stream;
    void run() override {
        auto object=std::make_unique<juce::DynamicObject>();object->setProperty("product","duck-pocket");object->setProperty("protocol",2);object->setProperty("key",shortKey);object->setProperty("device",device);
        auto s=std::make_shared<juce::WebInputStream>(juce::URL(api).withPOSTData(juce::JSON::toString(juce::var(object.release()),true)),true);
        s->withCustomRequestCommand("POST").withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n").withConnectionTimeout(8000).withNumRedirectsToFollow(0);
        {const juce::ScopedLock l(lock);stream=s;}
        if(threadShouldExit()){s->cancel();done.store(true,std::memory_order_release);return;}
        if(!s->connect(nullptr)||s->getStatusCode()!=200)error="Online activation failed. Check your key or use Offline.";
        else {
            juce::MemoryOutputStream body;std::array<char,512> buffer{};
            while(!threadShouldExit()&&!s->isExhausted()&&body.getDataSize()<=8192){const auto n=s->read(buffer.data(),int(buffer.size()));if(n<=0)break;body.write(buffer.data(),size_t(n));}
            if(threadShouldExit())error="Activation cancelled.";
            else if(s->isError()||body.getDataSize()>8192)error="Invalid activation response.";
            else {const auto json=juce::JSON::parse(body.toString());if(auto* result=json.getDynamicObject())license=result->getProperty("license").toString();if(license.isEmpty())error="Invalid activation response.";}
        }
        done.store(true,std::memory_order_release);
    }
};
class LicenseState : private juce::Timer {
public:
    std::atomic<bool> active{false};
    const juce::String deviceCode=localDeviceCode();
    static juce::File file() {return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("RainlineMusic/DuckPocket/license.key");}
    LicenseState(){refresh();startTimer(2000);}
    ~LicenseState() override {stopTimer();request.reset();}
    bool onlineBusy() const {return request!=nullptr;}
    juce::String onlineMessage() const {return message;}
    bool activate(const juce::String& key,juce::String& error){
        if(deviceCode.isEmpty()){error="System device ID is unavailable. Contact support.";return false;}
        if(!verifyLicense(key,deviceCode)){error="Invalid license or license belongs to another device.";return false;}
        const auto target=file();
        if(!target.getParentDirectory().createDirectory()){error="Cannot create license folder.";return false;}
        juce::TemporaryFile temporary(target);
        if(!temporary.getFile().replaceWithText(key.trim())||!temporary.overwriteTargetFileWithTemporary()){error="Cannot save license. Check folder permissions.";return false;}
        refresh();return active.load(std::memory_order_acquire);
    }
    bool importFile(const juce::File& source,juce::String& error) {
        if(!source.existsAsFile()||source.getSize()<=0||source.getSize()>1024){error="Select a valid Duck Pocket license file (up to 1 KB).";return false;}
        return activate(source.loadFileAsString(),error);
    }
    bool startOnline(const juce::String& input,juce::String& error) {
        if(request){error="Activation is already in progress.";return false;}
        const auto key=canonicalOnlineKey(input);
        if(key.isEmpty()){error="Enter a key like DUCK-7K3M-9X2P-6R8N-4W5T.";return false;}
        if(deviceCode.isEmpty()){error="System device ID is unavailable. Contact support.";return false;}
        const juce::String api=DUCK_LICENSE_API_URL;
        if(!api.startsWithIgnoreCase("https://")||juce::URL(api).getDomain().isEmpty()){error="Online activation is not configured yet. Use Offline.";return false;}
        message="Activating…";request=std::make_unique<LicenseRequest>(api,key,deviceCode);startTimer(100);return true;
    }
private:
    juce::String cached,message;
    bool checked=false;
    std::unique_ptr<LicenseRequest> request;
    void refresh(){const auto target=file();const auto key=target.getSize()<=1024?target.loadFileAsString().trim():juce::String();if(!checked||key!=cached){cached=key;checked=true;active.store(verifyLicense(key,deviceCode),std::memory_order_release);}}
    void timerCallback() override {
        if(request){if(!request->done.load(std::memory_order_acquire))return;message=request->error;if(message.isEmpty()&&!activate(request->license,message)){}request.reset();startTimer(2000);}
        refresh();
    }
};
}
