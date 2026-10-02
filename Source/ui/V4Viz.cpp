#include "V4Viz.h"
#include "../PluginProcessor.h"

V4Viz::V4Viz(Kind k, DeVerbProcessor* p, const juce::String& mod, const juce::String& txt)
    : kind(k), proc(p), mod_(mod), text_(txt) {}

void V4Viz::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto ink = juce::Colour(0xff16171A);
    auto paper = juce::Colour(0xffECE9E2);
    auto labP = juce::Colour(0xff55544F);

    switch (kind)
    {
        case Title:
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(20.f, juce::Font::bold)));
            g.drawText("deVerb", r, juce::Justification::centredLeft, false);
            break;
        case EngineFwd:
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(24.f, juce::Font::bold)));
            g.drawText("FWD", 0, 0, 90, (int)r.getHeight(), juce::Justification::centredLeft, false);
            g.setFont(10.f);
            g.setColour(labP);
            g.drawText("PRESENT", 95, 0, 120, (int)r.getHeight(), juce::Justification::centredLeft, false);
            break;
        case EngineRev:
            g.setColour(paper);
            g.setFont(juce::Font(juce::FontOptions(24.f, juce::Font::bold)));
            g.drawText("REV", 0, 0, 70, (int)r.getHeight(), juce::Justification::centredLeft, false);
            break;
        case Scope:
        {
            g.setColour(juce::Colour(0xffC7C2B6));
            g.drawRect(r, 1.f);
            if (proc != nullptr)
            {
                constexpr int N = 128;
                float tmp[N];
                proc->getScopeSnapshot(tmp, N);
                g.setColour(ink);
                juce::Path p;
                for (int i = 0; i < N; ++i)
                {
                    float x = r.getX() + (float)i / (N-1) * r.getWidth();
                    float y = r.getCentreY() - tmp[i] * r.getHeight() * 0.45f;
                    if (i == 0) p.startNewSubPath(x, y);
                    else p.lineTo(x, y);
                }
                g.strokePath(p, juce::PathStrokeType(1.5f));
            }
            break;
        }
        case CaptureWin:
        {
            float beats = 2.f;
            float pos01 = 0.f;
            if (proc != nullptr) { beats = proc->getRevCapBeatsUi(); pos01 = proc->getRevReadPosUi(); }
            g.setColour(juce::Colour(0xff2B2D32));
            g.fillRect(r);
            float winW = beats / 4.f * r.getWidth();
            g.setColour(juce::Colour(0xff3A3C42));
            g.fillRect(juce::Rectangle<float>(r.getRight()-winW, r.getY(), winW, r.getHeight()));
            g.setColour(paper);
            float hx = r.getRight() - pos01 * winW;
            g.drawLine(hx, r.getY(), hx, r.getBottom(), 1.5f);
            break;
        }
        case Ruler:
        {
            g.setColour(labP);
            g.setFont(10.f);
            for (int b = 0; b < 4; ++b)
                g.drawText(juce::String(b+1), (int)(b * r.getWidth()/4), 0, (int)(r.getWidth()/4), 12,
                           juce::Justification::centred, false);
            break;
        }
        case GateBig:
        {
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(44.f, juce::Font::plain)));
            g.drawText(text_.isNotEmpty() ? text_ : "07/16", r, juce::Justification::centredRight, false);
            break;
        }
        case DelayMs: case VerbT60: case MorphRead: case BpmSrc: case GranLed:
            g.setColour(kind == GranLed || kind == BpmSrc ? labP : ink);
            g.setFont(11.f);
            g.drawFittedText(text_, getLocalBounds(), juce::Justification::centred, 1);
            break;
        case Taps:
        {
            float fb = 0.35f;
            if (proc != nullptr) fb = proc->apvts.getRawParameterValue("fwd_delay_fb")->load();
            g.setColour(labP);
            g.drawLine(r.getX(), r.getBottom()-4, r.getRight(), r.getBottom()-4, 1.f);
            for (int i = 0; i < 8; ++i)
            {
                float x = r.getX() + 16 + i * 66;
                if (x > r.getRight() - 8) break;
                float rad = 12.f * std::pow(0.78f, (float)i) * (0.5f + fb);
                g.setColour(i == 0 ? juce::Colour(0xff2E9D6B) : juce::Colour(0xff2E9D6B).withAlpha(1.f - i*0.1f));
                if (i == 0) g.drawEllipse(x-rad, r.getBottom()-4-rad*2, rad*2, rad*2, 1.5f);
                else g.fillEllipse(x-rad, r.getBottom()-4-rad*2, rad*2, rad*2);
            }
            break;
        }
        case Decay:
        {
            g.setColour(ink);
            juce::Path p;
            int n = (int)r.getWidth()/3;
            for (int i = 0; i < n; ++i)
            {
                float a = std::exp(-(float)i/n*4.2f) * (r.getHeight()/2-3);
                float x = r.getX()+i*3+1;
                p.startNewSubPath(x, r.getCentreY()-a);
                p.lineTo(x, r.getCentreY()+a);
            }
            g.strokePath(p, juce::PathStrokeType(1.f));
            break;
        }
        case IR:
            g.setColour(labP);
            g.drawRoundedRectangle(r, 4.f, 1.f);
            g.setFont(10.f);
            g.drawFittedText("CONVOLUTION\nPHASE 8", getLocalBounds(), juce::Justification::centred, 2);
            break;
        case Shards:
        {
            g.setColour(juce::Colour(0xffF0B323));
            for (int i = 0; i < 8; ++i)
            {
                float x = r.getX() + 8 + i * (r.getWidth()-16)/7;
                float s = 6 + (i % 3) * 4;
                juce::Path t;
                t.addTriangle(x-s/2, r.getCentreY()+s/2, x+s/2, r.getCentreY()+s/2, x, r.getCentreY()-s/2);
                g.fillPath(t);
            }
            break;
        }
        case ModTitle:
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(11.f, juce::Font::bold)));
            g.drawText(text_, r, juce::Justification::centredLeft, false);
            break;
    }
}
