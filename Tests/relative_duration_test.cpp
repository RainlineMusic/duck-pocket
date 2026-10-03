#include "../Source/PocketDSP.h"
#include <cstdlib>
#include <iostream>
static void check(bool v,const char* m){if(!v){std::cerr<<m<<'\n';std::abort();}}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  pocket::Engine old,relative;old.reset(sr);relative.reset(sr);
  old.configure(1,2000);relative.configure(1,2000,20,20000,false,0,20,20000,0,100,true);
  for(int n=0;n<int(sr*2);++n){float k=n%int(sr*.5)<int(sr*.1)?.8f:0;
   auto a=old.process({.3f,-.2f},{k,k}),b=relative.process({.3f,-.2f},{k,k});check(a.out==b.out&&a.key==b.key&&a.gain==b.gain,"100% must equal previous AUTO bit for bit");}
  for(float pct:{25.f,50.f,75.f}){
   pocket::Engine e;e.reset(sr);e.configure(1,2000,20,20000,false,0,20,20000,0,pct,true);
   for(int n=0;n<int(sr*.5);++n)e.process({1,1},{n<int(sr*.1)?.8f:0,n<int(sr*.1)?.8f:0});
   const int limit=int(sr*.1*pct*.01);float before=1,after=0;
   for(int n=0;n<int(sr*.2);++n){auto v=e.process({1,1},{n<int(sr*.1)?.8f:0,n<int(sr*.1)?.8f:0});const int age=n-e.latency();
    if(age==limit/3)before=v.gain;
    if(age==limit+int(sr*.005))after=v.gain;}
   check(before<.3f,"relative event keeps initial depth");check(after>.999f,"relative event ends at requested fraction");
  }
 }
 std::cout<<"PASS percentage duration, learned length and exact 100% AUTO\n";
}
