#include "../Source/PocketDSP.h"
#include <iostream>
#include <cstdlib>
#include <new>
static size_t allocations=0;
void* operator new(size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
static void check(bool b,const char* m){if(!b){std::cerr<<"FAIL "<<m<<'\n';std::abort();}}
int main(){
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.}){
  for(float horizon:{1.f,5.f,10.f,20.f,50.f})for(float attack:{0.f,1.f,5.f,20.f,50.f}){
   pocket::Engine e;e.reset(sr,1,horizon);
   e.configure(1,2000,20,20000,false,0,20,20000,0,100,false,100,attack,false);
   const int latency=pocket::Engine::latencyForRate(sr,horizon),hit=int(sr*.08);
   const int a=std::min(latency,int(std::round(std::min(attack,horizon)*sr*.001)));
   check(e.latency()==latency,"selected latency matches sample rate");
   int first=-1;
   for(int n=0;n<hit+latency+10;++n){const float key=n==hit?.8f:0.f;const auto s=e.process({.5f,.5f},{key,key});
    check(s.dry[0]==(n>=latency?.5f:0.f),"dry and processed paths share selected latency");
    if(s.gain<.999999f&&first<0)first=n;
    if(n==hit+latency)check(std::abs(s.gain-.2f)<.001f,"full reduction reaches delayed transient at every horizon");
   }
   check(first==hit+latency-a,"attack starts at chosen anticipation time");
  }
  pocket::Engine e;e.reset(sr);e.configure(0,2000);
  int n=0;for(;n<int(sr*.06);++n)e.process({float(n),float(n)},{});
  const auto before=allocations;
  for(float horizon:{50.f,1.f,20.f,5.f,10.f,50.f}){
   e.setLookaheadMs(horizon);e.configure(0,2000);
   for(int i=0;i<128;++i,++n){auto s=e.process({float(n),float(n)},{});check(s.out[0]==float(n-e.latency()),"live switches use continuous buffered history");}
  }
  check(allocations==before,"latency switching and processing allocate no memory");
 }
 std::cout<<"PASS 1/5/10/20/50 ms: latency, attack timing, dry alignment, continuous history and allocation-free switching across six sample rates\n";
}
