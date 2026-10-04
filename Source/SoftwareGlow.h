#pragma once
#include <JuceHeader.h>
// GUI-only reusable images. Blur touches premultiplied colour bytes, so transparent
// pixels cannot introduce dark fringes. Image buffers are retained after size/DPI warm-up.
class PocketSoftwareGlow {
public:
    juce::Image core,emission;
    std::uint64_t prepares=0;
    void prepare(int width,int height){
        ++prepares;
        if(core.getWidth()!=width||core.getHeight()!=height){
            core=make(width,height);const int w=juce::jmax(1,width/4),h=juce::jmax(1,height/4);
            emission=make(w,h);horizontal=make(w,h);soft=make(w,h);trail=make(w,h);shifted=make(w,h);upscaled=make(width,height);composite=make(width,height);lastTime=-1;
        }
        core.clear(core.getBounds());emission.clear(emission.getBounds());
    }
    void reset(){trail.clear(trail.getBounds());lastTime=-1;}
    void paint(juce::Graphics& g,const juce::Image& background,juce::Rectangle<float> bounds,float intensity,double time,double window,bool phosphor){
        if(intensity<=.0001f||!background.isValid()){g.drawImage(core,bounds);return;}
        if(phosphor&&lastTime>=0&&time>lastTime){
            const double dt=juce::jlimit(0.,window,time-lastTime);shifted.clear(shifted.getBounds());
            juce::Graphics sg(shifted);sg.setOpacity(float(std::exp(-dt/.12)));sg.drawImageTransformed(trail,juce::AffineTransform::translation(-float(dt/window*trail.getWidth()),0));
            juce::Graphics eg(emission);eg.setOpacity(.22f);eg.drawImageAt(shifted,0,0);
        }
        if(phosphor&&time!=lastTime){trail.clear(trail.getBounds());juce::Graphics tg(trail);tg.drawImageAt(emission,0,0);lastTime=time;}
        const int step=intensity>.16f?2:1;blur(emission,horizontal,true,step);blur(horizontal,soft,false,step);
        upscaled.clear(upscaled.getBounds());{juce::Graphics ug(upscaled);ug.setImageResamplingQuality(juce::Graphics::highResamplingQuality);ug.drawImage(soft,upscaled.getBounds().toFloat());}
        // Additive RGB over the cached glass; crisp cores are composited last.
        juce::Image::BitmapData base(background,juce::Image::BitmapData::readOnly),bloom(upscaled,juce::Image::BitmapData::readOnly),dest(composite,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<dest.height;++y)for(int x=0;x<dest.width;++x){
            const auto b=base.getPixelColour(juce::jmin(base.width-1,x*base.width/dest.width),juce::jmin(base.height-1,y*base.height/dest.height));
            const auto* e=reinterpret_cast<const juce::PixelARGB*>(bloom.getPixelPointer(x,y));
            auto* d=reinterpret_cast<juce::PixelARGB*>(dest.getPixelPointer(x,y));
            d->setARGB(255,juce::uint8(juce::jmin(255,int(b.getRed())+juce::roundToInt(e->getRed()*intensity))),juce::uint8(juce::jmin(255,int(b.getGreen())+juce::roundToInt(e->getGreen()*intensity))),juce::uint8(juce::jmin(255,int(b.getBlue())+juce::roundToInt(e->getBlue()*intensity))));
        }
        g.drawImage(composite,bounds);g.drawImage(core,bounds);
    }
    static void blur(const juce::Image& input,juce::Image& output,bool horizontalPass,int step=1){
        constexpr int weights[]{1,6,15,20,15,6,1};
        juce::Image::BitmapData src(input,juce::Image::BitmapData::readOnly),dst(output,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<src.height;++y)for(int x=0;x<src.width;++x){auto* d=dst.getPixelPointer(x,y);for(int channel=0;channel<4;++channel){int value=0;for(int k=-3;k<=3;++k){const auto* pixel=src.getPixelPointer(juce::jlimit(0,src.width-1,x+(horizontalPass?k*step:0)),juce::jlimit(0,src.height-1,y+(horizontalPass?0:k*step)));value+=pixel[channel]*weights[k+3];}d[channel]=juce::uint8((value+32)/64);}}
    }
private:
    juce::Image horizontal,soft,trail,shifted,upscaled,composite;
    double lastTime=-1;
    static juce::Image make(int w,int h){return juce::Image(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());}
};
