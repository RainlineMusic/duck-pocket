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
    return key.length()==20&&key.startsWith("DUCK")&&key.substring(4).containsOnly("0123456789ABCDEFGHJKLMNPQRSTUVWXYZ")?key:juce::String();
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
        if(!s->connect(nullptr))error="Cannot connect to activation server. Try again or use Offline.";
        else if(s->getStatusCode()!=200) {
            switch(s->getStatusCode()) {
                case 403:error="Invalid or disabled key. Check your purchase email.";break;
                case 409:error="Device limit reached. Manage devices at rainlinemusic.su/account.";break;
                case 429:error="Too many attempts. Try again in a minute.";break;
                case 503:error="Activation server is temporarily unavailable. Try again later.";break;
                default:error="Online activation failed. Try again or use Offline.";break;
            }
        }
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

// Only a complete, authenticated transport response matching the current token
// can revoke it. Errors, timeouts, throttling and unrelated replies mean offline.
inline bool verificationRevoked(const juce::var& json, const juce::String& token,
                                 const juce::String& device, int status) {
    if (status != 200 && status != 403) return false;
    auto* o = json.getDynamicObject();
    if (o == nullptr || !o->getProperty("valid").isBool()
        || bool(o->getProperty("valid")) || o->getProperty("error").toString() != "revoked") return false;
    const auto digest = juce::SHA256(token.toRawUTF8(), size_t(token.getNumBytesAsUTF8())).toHexString();
    return o->getProperty("license_id").toString() == token.substring(4,40)
        && o->getProperty("device").toString() == device
        && o->getProperty("token_hash").toString() == digest;
}
class LicenseVerification final : private juce::Thread {
public:
    LicenseVerification(juce::String endpoint,juce::String value,juce::String code)
        : Thread("Duck verification"),token(std::move(value)),api(std::move(endpoint)),device(std::move(code)) { startThread(); }
    ~LicenseVerification() override {
        signalThreadShouldExit(); std::shared_ptr<juce::WebInputStream> s;
        { const juce::ScopedLock l(lock); s=stream; } if(s)s->cancel();stopThread(-1);
    }
    const juce::String token;
    std::atomic<bool> done{false}; bool revoked=false;
private:
    juce::String api,device; juce::CriticalSection lock;
    std::shared_ptr<juce::WebInputStream> stream;
    void run() override {
        auto o=std::make_unique<juce::DynamicObject>();o->setProperty("token",token);o->setProperty("device",device);
        auto s=std::make_shared<juce::WebInputStream>(juce::URL(api).withPOSTData(juce::JSON::toString(juce::var(o.release()),true)),true);
        s->withCustomRequestCommand("POST").withExtraHeaders("Content-Type: application/json\r\nAccept: application/json\r\n").withConnectionTimeout(3000).withNumRedirectsToFollow(0);
        {const juce::ScopedLock l(lock);stream=s;}
        if(!threadShouldExit()&&s->connect(nullptr)) {
            const auto status=s->getStatusCode();juce::MemoryOutputStream body;std::array<char,512> buffer{};
            while(!threadShouldExit()&&!s->isExhausted()&&body.getDataSize()<=8192){const auto n=s->read(buffer.data(),int(buffer.size()));if(n<=0)break;body.write(buffer.data(),size_t(n));}
            if(!threadShouldExit()&&!s->isError()&&s->isExhausted()&&body.getDataSize()<=8192)
                revoked=verificationRevoked(juce::JSON::parse(body.toString()),token,device,status);
        }
        done.store(true,std::memory_order_release);
    }
};
class LicenseState : private juce::Timer {
public:
    std::atomic<bool> active{false};
    const juce::String deviceCode=localDeviceCode();
    static juce::File file() {return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("RainlineMusic/DuckPocket/license.key");}
    LicenseState(){refresh();startTimer(100);}
    // Safe to call from a processor constructor; timer owns request state.
    void checkOnOpen() noexcept {verificationWanted.store(true,std::memory_order_release);}
    ~LicenseState() override {stopTimer();request.reset();verification.reset();}
    bool onlineBusy() const {return request!=nullptr;}
    juce::String onlineMessage() const {return message;}
    bool activate(const juce::String& key,juce::String& error){
        if(deviceCode.isEmpty()){error="System device ID is unavailable. Contact support.";return false;}
        if(!verifyLicense(key,deviceCode)){error="Invalid license or license belongs to another device.";return false;}
        const auto target=file();
        if(!target.getParentDirectory().createDirectory()){error="Cannot create license folder.";return false;}
        juce::TemporaryFile temporary(target);
        if(!temporary.getFile().replaceWithText(key.trim())||!temporary.overwriteTargetFileWithTemporary()){error="Cannot save license. Check folder permissions.";return false;}
        file().getSiblingFile("license.revoked").deleteFile();checked=false;refresh();checkOnOpen();return active.load(std::memory_order_acquire);
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
        message="Activating...";request=std::make_unique<LicenseRequest>(api,key,deviceCode);startTimer(100);return true;
    }
private:
    juce::String cached,message;
    bool checked=false;
    std::unique_ptr<LicenseRequest> request;
    std::unique_ptr<LicenseVerification> verification;
    std::atomic<bool> verificationWanted{false};
    double verificationStarted=0;
    void refresh(){const auto target=file();const auto key=target.getSize()<=1024?target.loadFileAsString().trim():juce::String();if(!checked||key!=cached){cached=key;checked=true;const auto digest=juce::SHA256(key.toRawUTF8(),size_t(key.getNumBytesAsUTF8())).toHexString();
        const bool revoked=file().getSiblingFile("license.revoked").loadFileAsString().trim()==digest;
        active.store(!revoked&&verifyLicense(key,deviceCode),std::memory_order_release);}}
    void timerCallback() override {
        if(verification){
            // A hard wall-clock deadline also covers slow or stalled body reads.
            if(!verification->done.load(std::memory_order_acquire)&&juce::Time::getMillisecondCounterHiRes()-verificationStarted>5000)verification.reset();
            else if(verification->done.load(std::memory_order_acquire)){
                if(verification->revoked&&cached==verification->token){
                    const auto digest=juce::SHA256(cached.toRawUTF8(),size_t(cached.getNumBytesAsUTF8())).toHexString();
                    file().getSiblingFile("license.revoked").replaceWithText(digest);
                    active.store(false,std::memory_order_release);file().deleteFile();message="Activation revoked. Open your account to activate this device again.";
                }verification.reset();
            }
        }
        if(verificationWanted.exchange(false,std::memory_order_acq_rel)&&!verification&&active.load(std::memory_order_acquire)){
            const juce::String activationApi=DUCK_LICENSE_API_URL;
            // Public endpoint lives beside activate; support legacy query routing.
            juce::String verifyApi;
            if(activationApi.endsWith("/activate"))verifyApi=activationApi.dropLastCharacters(8)+"verify";
            else if(activationApi.endsWith("route=activate"))verifyApi=activationApi.dropLastCharacters(8)+"verify";
            if(verifyApi.startsWithIgnoreCase("https://")&&!juce::URL(verifyApi).getDomain().isEmpty()){
                verificationStarted=juce::Time::getMillisecondCounterHiRes();verification=std::make_unique<LicenseVerification>(verifyApi,cached,deviceCode);
            }
        }
        if(request){if(!request->done.load(std::memory_order_acquire))return;message=request->error;if(message.isEmpty()&&!activate(request->license,message)){}request.reset();startTimer(100);}
        refresh();startTimer(request||verification?100:2000);
    }
};
}

