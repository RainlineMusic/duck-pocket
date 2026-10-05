#include "../Source/PocketDSP.h"
#include "Baseline/PocketDSP.h"
#include <random>
#include <iostream>
#include <cstdlib>
int main(){std::mt19937 random(101);std::uniform_real_distribution<float> noise(-.8f,.8f);
 for(double sr:{44100.,48000.,88200.,96000.,176400.,192000.})for(float duration:{5.f,50.f,200.f,2000.f}){
  pocket::Engine next;baseline::Engine previous;next.reset(sr);previous.reset(sr);
  for(int n=0;n<int(sr*.6);++n){if(n%137==0){const float depth=n%2?1.5f:.65f,balance=n%3?.3f:-.6f,low=n%5?20.f:300.f,high=n%7?20000.f:3000.f,output=n%11?-2.f:6.f;const bool bypass=n%13==0;
   next.configure(depth,duration,20,20000,bypass,balance,low,high,output);previous.configure(depth,duration,20,20000,bypass,balance,low,high,output);}
   const float x=noise(random),r=noise(random),hit=float(std::sin(n*.075)*std::exp(-double(n%int(sr*.18))/(sr*.025)));
   const auto a=next.process({x,r},{hit,hit*.9f});const auto b=previous.process({x,r},{hit,hit*.9f});
   if(a.out!=b.out||a.key!=b.key||a.gain!=b.gain){std::cerr<<"FAIL null at "<<sr<<" sample "<<n<<'\n';return 1;}
  }
 }
 std::cout<<"PASS bit-exact legacy DSP against Release-1.0 4f20dbd\n";
}
