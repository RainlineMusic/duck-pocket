#include "../Source/PocketDSP.h"
#include <limits>
#include <iostream>
#include <cstdlib>
static void check(bool ok){if(!ok)std::abort();}
int main(){
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.,double(INFINITY),double(NAN),0.,-1.}){
  pocket::Engine e;e.reset(sr,NAN);check(e.latency()>0&&e.latency()<=1920);
  for(float a:{0.f,1.f,1.001f,1.5f,INFINITY,NAN}){
   e.configure(a,2000,100,8000,false,.2f,300,3000,6);
   for(int i=0;i<10000;++i){const float x=i<100?std::numeric_limits<float>::max():.5f;
    auto v=e.process({x,-x},{x,x});check(std::isfinite(v.out[0])&&std::isfinite(v.out[1])&&std::isfinite(v.gain));}
  }
 }
 std::cout<<"PASS invalid reset and extreme finite input\n";
}
