#pragma once

#include "Matrix.h"

class CPdfFont;

struct TxtState
{
    double              Tc = 0.0;
    double              Tw = 0.0;
    double              Th = 100.0;
    double              Tl = 0.0;
    std::string         Tf;
    double              Tfs;
    int                 Tmode = 0;
    double              Trise = 0.0;

    CPdfFont* Tfont = nullptr;

    Matrix              Tm = Matrix::IdentityMatrix();
    Matrix              Tlm = Matrix::IdentityMatrix();
    Matrix              Trm = Matrix::IdentityMatrix();
};
