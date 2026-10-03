#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
enum class PocketTheme { Neon, Amber, SolidDark, SolidWhite };
struct PocketTokens {
    juce::Colour chassis,raised,glass,ink,muted,border,major,minor,key,out,neutral,brand;
    static PocketTokens forTheme(PocketTheme theme){
        using C=juce::Colour;
        switch(theme){
        case PocketTheme::Neon:return {C(0xff10161d),C(0xff1c2530),C(0xff0b1118),C(0xffe5eaf0),C(0xffa8b2bf),C(0xff36404d),C(0xff394957),C(0xff24313e),C(0xff63ccd6),C(0xffbca1f3),C(0xffb7c0cd),C(0xffe5bb68)};
        case PocketTheme::SolidWhite:return {C(0xffebecef),C(0xfff5f5f7),C(0xffe2e5e9),C(0xff202833),C(0xff58616d),C(0xffbcc2cb),C(0xffaeb8c4),C(0xffcdd3da),C(0xff176773),C(0xff6c4ca3),C(0xff58616d),C(0xff89611c)};
        case PocketTheme::Amber:return {C(0xff211c17),C(0xff302820),C(0xff171410),C(0xffeee4d8),C(0xffbdb0a1),C(0xff514538),C(0xff514739),C(0xff342d25),C(0xffcfa96d),C(0xffe78555),C(0xffbeb2a4),C(0xffc6ad78)};
        case PocketTheme::SolidDark:default:return {C(0xff191d23),C(0xff252b33),C(0xff10151b),C(0xffe4e7eb),C(0xffa7aeb6),C(0xff3b444f),C(0xff36424f),C(0xff26313c),C(0xff6cbbc5),C(0xffaa9bcd),C(0xffb8c0ca),C(0xffd7b46a)};
        }
    }
};
inline juce::Font pocketFont(float size,bool mono=false,bool medium=false){
    static auto sans=juce::Typeface::createSystemTypefaceFor(BinaryData::IBMPlexSansRegular_ttf,BinaryData::IBMPlexSansRegular_ttfSize);
    static auto strong=juce::Typeface::createSystemTypefaceFor(BinaryData::IBMPlexSansMedium_ttf,BinaryData::IBMPlexSansMedium_ttfSize);
    static auto numbers=juce::Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMonoRegular_ttf,BinaryData::IBMPlexMonoRegular_ttfSize);
    return juce::Font(juce::FontOptions(mono?numbers:(medium?strong:sans)).withHeight(juce::jmax(11.f,size)));
}
