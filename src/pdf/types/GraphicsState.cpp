#include "GraphicsState.h"
#include "pdf/objects/PdfFont.h"

PdfRect Measure(const DIGraphicsState& GS, const std::string& text)
{
    auto& TS = GS.TextState;

    PdfRect rect{ 0.0,0.0,0.0,0.0 };

    double Tfs = TS.Tfs;
    double Tc = TS.Tc;
    double Tw = TS.Tw;
    double Th = TS.Th;
    double Thr = Th / 100.0;

    double cx = 0.0;
    for (auto& c : text) {
        auto gw = TS.Tfont->GetGlyphWidth(c);
        double Tx = (((gw / 1000.0)) * Tfs + Tc + Tw) * Thr;
        cx = Tx * TS.Tm.a + cx;
    }

    rect.x = TS.Tm.x;
    rect.y = TS.Tm.y;
    rect.width = cx;
    rect.height = 0;

    return rect;
}