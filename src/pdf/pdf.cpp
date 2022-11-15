#include "pdf.h"

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

