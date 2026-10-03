#include "V4Viz.h"
#include "WaterLnF.h"
#include "../PluginProcessor.h"

V4Viz::V4Viz(Kind k, DeVerbProcessor* p, const juce::String& mod, const juce::String& txt)
    : kind(k), proc(p), mod_(mod), text_(txt) {}

void V4Viz::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    // Polaridade p/n (FWD papel / REV tinta): tinta sobre tinta é invisível.
    auto ink = dark_ ? juce::Colour(0xffECE9E2) : juce::Colour(0xff16171A);
    auto paper = dark_ ? juce::Colour(0xff16171A) : juce::Colour(0xffECE9E2);
    auto labP = dark_ ? juce::Colour(0xff9A9892) : juce::Colour(0xff55544F);
    auto mono = juce::Font(juce::FontOptions("DejaVu Sans Mono", 11.f, juce::Font::plain));

    switch (kind)
    {
        case Title:
        {
            // marca: círculo cortado (papel em cima, tinta em baixo)
            float my = r.getCentreY();
            g.setColour(ink);
            g.drawEllipse(2.f, my - 9.f, 18.f, 18.f, 1.5f);
            g.fillEllipse(2.f, my - 9.f, 18.f, 9.f);
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(20.f, juce::Font::bold)));
            g.drawText("deVerb", juce::Rectangle<int>(28, 0, (int)r.getWidth() - 28, (int)r.getHeight()),
                       juce::Justification::centredLeft, false);
            break;
        }
        case EngineFwd:
        {
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(24.f, juce::Font::bold)));
            g.drawText("FWD", 0, 0, 62, (int)r.getHeight(), juce::Justification::centredLeft, false);
            juce::Path arr;
            float ay = r.getCentreY();
            arr.addTriangle(66.f, ay - 6.f, 66.f, ay + 6.f, 75.f, ay);
            g.fillPath(arr);
            g.setFont(10.f);
            g.setColour(labP);
            g.drawText("PRESENT", 82, 0, 120, (int)r.getHeight(), juce::Justification::centredLeft, false);
            break;
        }
        case EngineRev:
        {
            // 112px: triângulo + REV + PAST cabem à justa
            g.setColour(paper);
            juce::Path arr;
            float ay = r.getCentreY();
            arr.addTriangle(11.f, ay - 6.f, 11.f, ay + 6.f, 2.f, ay);
            g.fillPath(arr);
            g.setFont(juce::Font(juce::FontOptions(24.f, juce::Font::bold)));
            g.drawText("REV", 14, 0, 52, (int)r.getHeight(), juce::Justification::centredLeft, false);
            g.setColour(labP);
            g.setFont(juce::Font(juce::FontOptions(10.f, juce::Font::bold)));
            g.drawText("PAST", 68, 0, 44, (int)r.getHeight(), juce::Justification::centredLeft, false);
            break;
        }
        case Scope:
        {
            g.setColour(juce::Colour(0xffC7C2B6));
            g.drawRect(r, 1.f);
            g.setColour(labP.withAlpha(0.5f));
            g.drawLine(r.getX(), r.getCentreY(), r.getRight(), r.getCentreY(), 1.f);
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
                g.drawLine(r.getRight() - 14.f, r.getY() + 3.f,
                           r.getRight() - 14.f, r.getBottom() - 3.f, 1.5f);
                g.setColour(labP);
                g.setFont(10.f);
                g.drawText("NOW", juce::Rectangle<int>((int)r.getRight() - 40, 0, 38, 12),
                           juce::Justification::centredRight, false);
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
            // grelha de beats + etiquetas
            g.setColour(juce::Colour(0xff3A3C42));
            const char* bl[4] = { "NOW", "-1", "-2", "-3" };
            for (int k = 0; k < 4; ++k)
            {
                float x = r.getRight() - k * r.getWidth() / 4.f;
                g.drawLine(x, r.getY(), x, r.getBottom(), 1.f);
                g.setColour(labP);
                g.setFont(10.f);
                g.drawText(bl[k], juce::Rectangle<int>((int)x - 34, (int)r.getY(), 32, 12),
                           juce::Justification::centredRight, false);
                g.setColour(juce::Colour(0xff3A3C42));
            }
            g.setColour(paper);
            float hx = r.getRight() - pos01 * winW;
            g.drawLine(hx, r.getY(), hx, r.getBottom(), 1.5f);
            // cabeça de leitura ◂ (mockup .readhead)
            juce::Path hd;
            hd.addTriangle(hx - 7.f, r.getBottom() - 11.f, hx + 7.f, r.getBottom() - 11.f,
                           hx, r.getBottom() - 4.f);
            g.fillPath(hd);
            break;
        }
        case Ruler:
        {
            // Ticks por passo + número de beat + passo ativo iluminado.
            int n = auxA_ == 8 ? 8 : 16;
            int active = auxB_;
            g.setColour(labP);
            g.setFont(10.f);
            for (int b = 0; b < 4; ++b)
                g.drawText(juce::String(b+1), (int)(b * r.getWidth()/4), 0, (int)(r.getWidth()/4), 12,
                           juce::Justification::centred, false);
            float w = r.getWidth() / 16.f;
            for (int i = 0; i < 16; ++i)
            {
                // (o playhead vive nos steps; a régua é só escala, como no mockup)
                juce::ignoreUnused(active);
                float x = r.getX() + i * w + w * 0.5f;
                bool dim = i >= n;
                g.setColour(dim ? labP.withAlpha(0.3f) : labP);
                g.drawLine(x, r.getBottom() - 8.f, x, r.getBottom() - 4.f, 1.f);
            }
            break;
        }
        case GateBig:
        {
            // Número grande + "/total" pequeno (mockup .viz.gate-big).
            juce::String big = text_.upToFirstOccurrenceOf("/", false, true);
            juce::String tot = "/" + text_.fromFirstOccurrenceOf("/", false, true);
            if (big.isEmpty()) { big = "07"; tot = "/16"; }
            g.setColour(labP);
            g.drawLine(r.getX(), r.getY() + 4.f, r.getX(), r.getBottom() - 4.f, 1.f);
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 44.f, juce::Font::plain)));
            g.drawText(big, juce::Rectangle<int>((int)r.getX() + 8, (int)r.getY(),
                                                 (int)r.getWidth() - 70, (int)r.getHeight()),
                       juce::Justification::centredRight, false);
            g.setColour(labP);
            g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 16.f, juce::Font::plain)));
            g.drawText(tot, juce::Rectangle<int>((int)r.getRight() - 58, (int)r.getBottom() - 30, 54, 22),
                       juce::Justification::centredLeft, false);
            break;
        }
        case MorphRead:
        {
            g.setColour(labP);
            g.setFont(10.f);
            g.drawFittedText("MORPH", juce::Rectangle<int>(0, 0, (int)r.getWidth(), 12),
                             juce::Justification::centredLeft, 1);
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 14.f, juce::Font::plain)));
            g.drawFittedText(text_, juce::Rectangle<int>(0, 12, (int)r.getWidth(), 22),
                             juce::Justification::centredLeft, 1);
            break;
        }
        case DelayMs: case VerbT60: case BpmSrc:
            if (kind == BpmSrc)
            {
                g.setColour(labP);
                g.fillEllipse(r.getX() + 8.f, r.getCentreY() - 4.f, 8.f, 8.f);
                g.setColour(labP);
                g.setFont(mono);
                g.drawFittedText(text_, getLocalBounds(), juce::Justification::centred, 1);
            }
            else
            {
                // readout.right do mockup: encostado à direita
                g.setColour(ink);
                g.setFont(mono);
                g.drawFittedText(text_, getLocalBounds(), juce::Justification::centredRight, 1);
            }
            break;
        case GranLed:
        {
            // lâmpada + texto encostado à direita; quente = acento do módulo
            float ly = r.getCentreY();
            g.setColour(hot_ ? juce::Colour(0xffF0B323) : labP);
            if (hot_) g.fillEllipse(r.getRight() - 66.f, ly - 4.f, 8.f, 8.f);
            else g.drawEllipse(r.getRight() - 66.f, ly - 4.f, 8.f, 8.f, 1.5f);
            g.setColour(hot_ ? ink : labP);
            g.setFont(juce::Font(juce::FontOptions("DejaVu Sans Mono", 10.f, juce::Font::bold)));
            g.drawFittedText(text_, juce::Rectangle<int>((int)r.getX(), 0,
                             (int)r.getWidth() - 14, (int)r.getHeight()),
                             juce::Justification::centredRight, 1);
            break;
        }
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
            float tx = r.getX() + r.getWidth() * 0.52f;
            g.drawLine(tx, r.getY() + 2.f, tx, r.getBottom() - 2.f, 1.5f);
            g.setColour(labP);
            g.setFont(10.f);
            g.drawText("T60", juce::Rectangle<int>((int)tx + 5, 0, 40, 12),
                       juce::Justification::centredLeft, false);
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
            // 15 estilhas pseudo-aleatórias determinísticas (mockup: seed 23)
            juce::uint32 seed = 23;
            auto rnd = [&] { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.0; };
            for (int i = 0; i < 15; ++i)
            {
                float x = r.getX() + 8 + (float)(i * (r.getWidth() - 24) / 14) + (float)(rnd() * 8 - 4);
                float s = (float)(6 + rnd() * 10);
                bool up = rnd() > 0.5;
                float cy = r.getCentreY() + (float)(rnd() * 12 - 6);
                juce::Path t;
                if (up) t.addTriangle(x-s/2, cy+s/2, x+s/2, cy+s/2, x, cy-s/2);
                else t.addTriangle(x-s/2, cy-s/2, x+s/2, cy-s/2, x, cy+s/2);
                if (rnd() > 0.65) { g.setColour(ink); g.strokePath(t, juce::PathStrokeType(1.f)); }
                else { g.setColour(juce::Colour(0xffF0B323)); g.fillPath(t); }
            }
            break;
        }
        case ModTitle:
        {
            // mini-glifo do módulo (objeto em acento) + título, como no mockup
            auto acc = WaterLnF::accentFor(mod_);
            g.setColour(acc);
            if (mod_ == "gate") g.fillRect(4.f, r.getCentreY() - 8.f, 16.f, 16.f);
            else if (mod_ == "delay")
            {
                juce::Path p;
                p.addArc(4.f, r.getCentreY() - 8.f, 16.f, 16.f, -1.1f, 1.1f, true);
                g.strokePath(p, juce::PathStrokeType(2.f));
            }
            else if (mod_ == "verb") g.drawEllipse(4.f, r.getCentreY() - 8.f, 16.f, 16.f, 2.f);
            else
            {
                juce::Path p;
                p.addTriangle(4.f, r.getCentreY() + 8.f, 20.f, r.getCentreY() + 8.f,
                              12.f, r.getCentreY() - 8.f);
                g.fillPath(p);
            }
            g.setColour(ink);
            g.setFont(juce::Font(juce::FontOptions(11.f, juce::Font::bold)));
            g.drawText(text_, juce::Rectangle<int>(30, 0, (int)r.getWidth() - 30, (int)r.getHeight()),
                       juce::Justification::centredLeft, false);
            break;
        }
    }
}
