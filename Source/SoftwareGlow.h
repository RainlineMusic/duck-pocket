#pragma once
#include <JuceHeader.h>
// GUI-only history, shared by the software and GPU emission paths. Repaints
// at the same audio time reuse the completed frame instead of dropping its tail.
class PocketPhosphorTrail {
public:
    void reset(){trail.clear(trail.getBounds());lastTime=-1;}
    void apply(juce::Image& emission,double time,double window){
        if(!emission.isValid()||!std::isfinite(time)||!std::isfinite(window)||window<=0){reset();return;}
        if(trail.getWidth()!=emission.getWidth()||trail.getHeight()!=emission.getHeight()){
            trail=make(emission.getWidth(),emission.getHeight());shifted=make(emission.getWidth(),emission.getHeight());lastTime=-1;
        }
        if(time<lastTime)reset();
        if(lastTime>=0&&std::abs(time-lastTime)<1.0e-9){
            emission.clear(emission.getBounds());juce::Graphics g(emission);g.drawImageAt(trail,0,0);return;
        }
        if(lastTime>=0){
            const double dt=juce::jlimit(0.,window,time-lastTime);shifted.clear(shifted.getBounds());
            {juce::Graphics g(shifted);g.setOpacity(float(std::exp(-dt/.12)));g.drawImageTransformed(trail,juce::AffineTransform::translation(-float(dt/window*trail.getWidth()),0));}
            juce::Graphics g(emission);g.setOpacity(.22f);g.drawImageAt(shifted,0,0);
        }
        trail.clear(trail.getBounds());{juce::Graphics g(trail);g.drawImageAt(emission,0,0);}lastTime=time;
    }
private:
    juce::Image trail,shifted;
    double lastTime=-1;
    static juce::Image make(int w,int h){return juce::Image(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());}
};
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
            emission=make(w,h);horizontal=make(w,h);soft=make(w,h);upscaled=make(width,height);composite=make(width,height);phosphorTrail.reset();
        }
        core.clear(core.getBounds());emission.clear(emission.getBounds());
    }
    void reset(){phosphorTrail.reset();}
    void paint(juce::Graphics& g,const juce::Image& background,juce::Rectangle<float> bounds,float intensity,double time,double window,bool phosphor){
        if(intensity<=.0001f||!background.isValid()){g.drawImage(core,bounds);return;}
        if(phosphor)phosphorTrail.apply(emission,time,window);
        if(!hasEmission(emission)){g.drawImage(core,bounds);return;}
        const int step=1;blur(emission,horizontal,true,step);blur(horizontal,soft,false,step);
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
    // Empty masks must leave the existing glass untouched. Besides avoiding
    // useless blur work, this prevents a second resampling of the cached grid.
    static bool hasEmission(const juce::Image& image){
        if(!image.isValid())return false;
        juce::Image::BitmapData pixels(image,juce::Image::BitmapData::readOnly);
        for(int y=0;y<pixels.height;++y)for(int x=0;x<pixels.width;++x)
            if(reinterpret_cast<const juce::PixelARGB*>(pixels.getPixelPointer(x,y))->getAlpha()>0)return true;
        return false;
    }
    // Contiguous sliding-window kernel: no skipped texels or sparse-grid artefacts.
    // Three box passes approximate a Gaussian, with work independent of radius.
    static void boxBlur(const juce::Image& input,juce::Image& output,bool horizontalPass,int radius){
        juce::Image::BitmapData src(input,juce::Image::BitmapData::readOnly),dst(output,juce::Image::BitmapData::writeOnly);
        const int length=horizontalPass?src.width:src.height,lines=horizontalPass?src.height:src.width;
        radius=juce::jlimit(1,64,radius);const int count=2*radius+1;
        for(int line=0;line<lines;++line){
            auto pixel=[&](int at){at=juce::jlimit(0,length-1,at);return src.getPixelPointer(horizontalPass?at:line,horizontalPass?line:at);};
            int sum[4]{};for(int k=-radius;k<=radius;++k){const auto* v=pixel(k);for(int c=0;c<4;++c)sum[c]+=v[c];}
            for(int at=0;at<length;++at){auto* d=dst.getPixelPointer(horizontalPass?at:line,horizontalPass?line:at);
                for(int c=0;c<4;++c)d[c]=juce::uint8((sum[c]+count/2)/count);
                const auto* a=pixel(at-radius);const auto* b=pixel(at+radius+1);for(int c=0;c<4;++c)sum[c]+=int(b[c])-int(a[c]);
            }
        }
    }
    static void blur(const juce::Image& input,juce::Image& output,bool horizontalPass,int step=1){
        constexpr int weights[]{1,6,15,20,15,6,1};
        juce::Image::BitmapData src(input,juce::Image::BitmapData::readOnly),dst(output,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<src.height;++y)for(int x=0;x<src.width;++x){auto* d=dst.getPixelPointer(x,y);for(int channel=0;channel<4;++channel){int value=0;for(int k=-3;k<=3;++k){const auto* pixel=src.getPixelPointer(juce::jlimit(0,src.width-1,x+(horizontalPass?k*step:0)),juce::jlimit(0,src.height-1,y+(horizontalPass?0:k*step)));value+=pixel[channel]*weights[k+3];}d[channel]=juce::uint8((value+32)/64);}}
    }
private:
    juce::Image horizontal,soft,upscaled,composite;
    PocketPhosphorTrail phosphorTrail;
    static juce::Image make(int w,int h){return juce::Image(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());}
};
