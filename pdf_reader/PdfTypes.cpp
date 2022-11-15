#include "stdafx.h"
#include "PdfTypes.h"

#include "tools.h"
#include "PdfFont.h"

Matrix& DIGraphicsState::GetTextRenderingMatrix()
{
    TextState.Trm = Matrix::IdentityMatrix();
    TextState.Trm.a = TextState.Tfs * (TextState.Th / 100.0);
    TextState.Trm.d = TextState.Tfs;
    TextState.Trm.y = TextState.Trise;

    Multiply(TextState.Trm, TextState.Tm);
    Multiply(TextState.Trm, CTM);

    return TextState.Trm;
}


Matrix DIGraphicsState::GetTextRenderingMatrix() const
{
    Matrix Trm = Matrix::IdentityMatrix();
    Trm.a = TextState.Tfs * (TextState.Th / 100.0);
    Trm.d = TextState.Tfs;
    Trm.y = TextState.Trise;

    Multiply(Trm, TextState.Tm);
    Multiply(Trm, CTM);

    return Trm;
}

Matrix DIGraphicsState::GetUserSpaceMatrix() const
{
    Matrix Us = TextState.Tm;
    Multiply(Us, CTM);
    return Us;
}

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