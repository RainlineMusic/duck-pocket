#include "../Source/PocketDSP.h"
#include <iostream>
#include <cstdlib>
#include <limits>
#include <new>
static size_t allocations=0;
void* operator new(size_t size){++allocations;if(auto* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
static void check(bool pass,const char* text){if(!pass){std::cerr<<"FAIL "<<text<<'\n';std::abort();}}
static void configure(pocket::Engine& e,float mix,float attack,float balance=0,bool bypass=false,float low=20,float high=20000,float output=0){e.configure(1,2000,20,20000,bypass,balance,low,high,output,100,false,mix,attack,false);}
int main(){
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.}){
  const float key=1.f-std::pow(10.f,-15.f/20.f);
  pocket::Engine full,half,zero;for(auto* e:{&full,&half,&zero})e->reset(sr);
  configure(full,100,0);configure(half,50,0);configure(zero,0,0);
  for(int n=0;n<int(sr*.1);++n){const auto a=full.process({.4f,-.2f},{key,key});const auto b=half.process({.4f,-.2f},{key,key});const auto c=zero.process({.4f,-.2f},{key,key});
   if(n>int(sr*.03)){check(std::abs(20*std::log10(a.gain)+15)<.001f,"reference ducks 15 dB");check(std::abs(20*std::log10(b.gain)+7.5f)<.001f,"50% Mix halves dB reduction and its meter");check(c.out==c.dry&&c.gain==1,"0% Mix is bit-exact latency-aligned dry");check(std::abs(b.out[0]/b.dry[0]-b.gain)<1e-6f,"actual wideband output matches scaled meter");}
  }
  configure(half,0,0);for(int n=0;n<int(sr*.1);++n)half.process({.4f,-.2f},{key,key});
  configure(half,100,0);for(int n=0;n<int(sr*.1);++n){const auto a=full.process({.4f,-.2f},{key,key});const auto b=half.process({.4f,-.2f},{key,key});if(n>int(sr*.09))check(a.out==b.out&&a.gain==b.gain,"automating back to 100% settles to exact full reduction");}
  for(float attack:{0.f,.1f,1.f,3.f,5.f,20.f}){
   pocket::Engine e;e.reset(sr);configure(e,100,attack);const int hit=int(sr*.02),a=std::min(e.latency(),int(std::round(std::min(5.f,attack)*sr*.001)));
   float previousGain=1;int first=-1;
   for(int n=0;n<hit+e.latency()+20;++n){const float k=n==hit?.8f:0;const auto sample=e.process({.5f,.5f},{k,k});
    if(sample.gain<.999999f&&first<0)first=n;
    if(n<hit+e.latency()-a)check(sample.gain==1,"Attack never anticipates beyond its selected duration");
    if(n>=hit+e.latency()-a&&n<=hit+e.latency()){check(sample.gain<=previousGain+1e-6f,"Attack ramps monotonically towards transient");previousGain=sample.gain;}
    if(n==hit+e.latency()){check(std::abs(sample.gain-.2f)<.001f,"full requested reduction is reached on the aligned transient");check(sample.dry[0]==.5f,"audio latency remains unchanged");}
   }
   check(first==hit+e.latency()-a,"Attack onset matches the selected lookahead horizon");check(e.latency()==pocket::Engine::latencyForRate(sr),"Attack never increases reported latency");
  }
  // Band selection and M/S use the scaled component gains, not a parallel dry bus.
  for(float balance:{-1.f,0.f,1.f})for(float mix:{0.f,50.f,100.f}){
   pocket::Engine e,dry;e.reset(sr);dry.reset(sr);configure(e,mix,5,balance,false,150,3000,-3);dry.configure(0,2000,20,20000,false,0,20,20000,-3);
   for(int n=0;n<int(sr*.1);++n){const auto sample=e.process({.3f,-.1f},{key,key});const auto reference=dry.process({.3f,-.1f},{0,0});check(std::isfinite(sample.out[0])&&std::isfinite(sample.gain),"band/M/S Mix is finite");if(mix==0&&n>int(sr*.09))check(sample.out==reference.out,"zero Mix retains Output gain while bypassing band subtraction");}
  }
  // Automation, invalid controls, and bypass under active ducking.
  pocket::Engine e;e.reset(sr);for(int n=0;n<int(sr*.1);++n){if(n%31==0)configure(e,float(n%101),float(n%51)*.1f,float(n%3-1),n>int(sr*.07));const auto v=e.process({.2f,-.1f},{.8f,.8f});check(std::isfinite(v.out[0])&&std::isfinite(v.out[1])&&v.gain>=0&&v.gain<=1,"automated Mix/Attack remains finite and bounded");}
  configure(e,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity());for(int n=0;n<1000;++n)check(std::isfinite(e.process({.2f,.2f},{.5f,.5f}).gain),"nonfinite new controls are safe");
 }
 pocket::Engine realtime;realtime.reset(192000);const auto before=allocations;
 for(int n=0;n<100000;++n){if(n%17==0)configure(realtime,float(n%101),float(n%51)*.1f);realtime.process({.2f,-.1f},{.8f,.8f});}
 check(allocations==before,"Mix and Attack processing/automation allocate no memory");
 std::cout<<"PASS Mix dB scaling, actual meter, dry identity, Attack timing/clamp, band/M/S, automation and fixed latency across six sample rates\n";
}
