#pragma once
#include <JuceHeader.h>
#if DUCK_ENABLE_OPENGL
#include <juce_opengl/juce_opengl.h>
#include <mutex>
// This mutex exchanges immutable GUI/GPU snapshots only. The audio thread never
// touches this renderer, images, locks, OpenGL, or JUCE Components.
class PocketGlowRenderer final : public juce::OpenGLRenderer {
public:
    struct Plot {juce::Rectangle<int> bounds;juce::Image background,emission;float intensity=0;};
    struct Frame {std::array<Plot,2> plots;int width=1,height=1;std::uint64_t chromeRevision=0;};
    juce::OpenGLContext context;
    std::atomic<bool> ready{false},failed{false},presented{false};
    std::atomic<std::uint64_t> frames{0};
    PocketGlowRenderer(){context.setRenderer(this);context.setContinuousRepainting(false);context.setComponentPaintingEnabled(true);context.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);}
    ~PocketGlowRenderer() override {stop();}
    void attach(juce::Component& target){failed.store(false);context.attachTo(target);}
    void stop(){context.setContinuousRepainting(false);context.detach();ready.store(false);presented.store(false);}
    void publish(std::shared_ptr<const Frame> next){std::lock_guard<std::mutex> guard(exchange);pending=std::move(next);}
    void newOpenGLContextCreated() override {
        using namespace juce::gl;
        failed.store(false);ready.store(false);
        const juce::String vertex="attribute vec2 position;varying vec2 uv;void main(){uv=(position+1.0)*0.5;gl_Position=vec4(position,0.0,1.0);}";
        const juce::String copy="varying vec2 uv;uniform sampler2D source;uniform float opacity;void main(){gl_FragColor=texture2D(source,uv)*opacity;}";
        const juce::String blur="varying vec2 uv;uniform sampler2D source;uniform vec2 stepSize;void main(){vec4 c=texture2D(source,uv)*0.227027;c+=(texture2D(source,uv+stepSize*1.384615)+texture2D(source,uv-stepSize*1.384615))*0.316216;c+=(texture2D(source,uv+stepSize*3.230769)+texture2D(source,uv-stepSize*3.230769))*0.070270;gl_FragColor=c;}";
        copyProgram=makeProgram(vertex,copy);blurProgram=makeProgram(vertex,blur);
        if(!copyProgram||!blurProgram){failed.store(true);return;}
        context.extensions.glGenBuffers(1,&vertexBuffer);context.extensions.glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);
        constexpr float vertices[]{-1,-1,1,-1,-1,1,-1,1,1,-1,1,1};context.extensions.glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
        context.extensions.glGenVertexArrays(1,&vertexArray);context.extensions.glBindVertexArray(vertexArray);
        ready.store(true);
    }
    void openGLContextClosing() override {
        ready.store(false);presented.store(false);lastFrame.reset();for(auto& p:resources){p.base.release();p.mask.release();p.horizontal.release();p.vertical.release();p.chromeRevision=std::numeric_limits<std::uint64_t>::max();}
        if(vertexBuffer)context.extensions.glDeleteBuffers(1,&vertexBuffer);if(vertexArray)context.extensions.glDeleteVertexArrays(1,&vertexArray);
        vertexBuffer=vertexArray=0;copyProgram.reset();blurProgram.reset();
    }
    void renderOpenGL() override {
        using namespace juce::gl;
        if(!ready.load())return;
        std::shared_ptr<const Frame> frame;{std::lock_guard<std::mutex> guard(exchange);frame=pending;}
        if(!frame)return;
        ++frames;const auto defaultTarget=juce::OpenGLFrameBuffer::getCurrentFrameBufferTarget();
        const float scale=float(context.getRenderingScale());const int vw=juce::jmax(1,juce::roundToInt(frame->width*scale)),vh=juce::jmax(1,juce::roundToInt(frame->height*scale));
        glViewport(0,0,vw,vh);juce::OpenGLHelpers::clear(juce::Colours::transparentBlack);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);
        context.extensions.glBindVertexArray(vertexArray);
        for(size_t i=0;i<resources.size();++i){auto& r=resources[i];const auto& plot=frame->plots[i];if(!plot.background.isValid())continue;
            if(r.chromeRevision!=frame->chromeRevision){r.base.loadImage(plot.background);r.chromeRevision=frame->chromeRevision;}
            if(frame!=lastFrame&&plot.emission.isValid())r.mask.loadImage(plot.emission);
            if(plot.intensity>.001f&&plot.emission.isValid()){
                const int w=plot.emission.getWidth(),h=plot.emission.getHeight();
                if(r.horizontal.getWidth()!=w||r.horizontal.getHeight()!=h){
                    if(!r.horizontal.initialise(context,w,h)||!r.vertical.initialise(context,w,h)){failed.store(true);ready.store(false);return;}}
                r.horizontal.makeCurrentAndClear();glViewport(0,0,w,h);draw(*blurProgram,r.mask.getTextureID(),(1.f+plot.intensity*6.f)/float(w),0,1);
                r.vertical.makeCurrentAndClear();glViewport(0,0,w,h);draw(*blurProgram,r.horizontal.getTextureID(),0,(1.f+plot.intensity*6.f)/float(h),1);
            }
            context.extensions.glBindFramebuffer(GL_FRAMEBUFFER,defaultTarget);
            const auto bounds=plot.bounds.toFloat()*scale;glViewport(juce::roundToInt(bounds.getX()),vh-juce::roundToInt(bounds.getBottom()),juce::roundToInt(bounds.getWidth()),juce::roundToInt(bounds.getHeight()));
            glDisable(GL_BLEND);draw(*copyProgram,r.base.getTextureID(),0,0,1);
            if(plot.intensity>.001f&&plot.emission.isValid()){glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE);draw(*copyProgram,r.vertical.getTextureID(),0,0,plot.intensity);glDisable(GL_BLEND);}
        }
        presented.store(true);lastFrame=frame;context.extensions.glBindFramebuffer(GL_FRAMEBUFFER,defaultTarget);glViewport(0,0,vw,vh);
        context.extensions.glBindBuffer(GL_ARRAY_BUFFER,0);context.extensions.glBindVertexArray(0);
    }
private:
    struct Resources {std::uint64_t chromeRevision=std::numeric_limits<std::uint64_t>::max();juce::OpenGLTexture base,mask;juce::OpenGLFrameBuffer horizontal,vertical;};
    std::array<Resources,2> resources;
    std::unique_ptr<juce::OpenGLShaderProgram> copyProgram,blurProgram;
    GLuint vertexBuffer=0,vertexArray=0;
    std::mutex exchange;std::shared_ptr<const Frame> pending,lastFrame;
    std::unique_ptr<juce::OpenGLShaderProgram> makeProgram(const juce::String& vertex,const juce::String& fragment){
        auto p=std::make_unique<juce::OpenGLShaderProgram>(context);
        if(!p->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(vertex))||!p->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(fragment))||!p->link())return {};
        return p;
    }
    void draw(juce::OpenGLShaderProgram& program,GLuint texture,float dx,float dy,float opacity){
        using namespace juce::gl;program.use();program.setUniform("source",0);program.setUniform("stepSize",dx,dy);program.setUniform("opacity",opacity);
        context.extensions.glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        const auto attribute=juce::OpenGLShaderProgram::Attribute(program,"position").attributeID;if(attribute<0)return;
        context.extensions.glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer);context.extensions.glEnableVertexAttribArray(GLuint(attribute));context.extensions.glVertexAttribPointer(GLuint(attribute),2,GL_FLOAT,GL_FALSE,0,nullptr);glDrawArrays(GL_TRIANGLES,0,6);context.extensions.glDisableVertexAttribArray(GLuint(attribute));
    }
};
#endif
