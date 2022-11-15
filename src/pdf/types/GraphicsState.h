#pragma once

#include "Matrix.h"
#include "ClippingPath.h"
#include "MultiColor.h"
#include "TextState.h"
#include "PdfRect.h"

struct DIGraphicsState
{
    Matrix              CTM = Matrix::IdentityMatrix();
    SClippingPath       ClippingPath;
    SMultiColor         ColorSpace = SMultiColor::FromDefault("/DeviceGray");
    SMultiColor         Color = SMultiColor::FromDefault("black");
    TxtState            TextState;
    double              LineWidth = 1.0;
    int                 LineCap = 0;
    int                 LineJoin = 0;
    double              MiterLimit = 10.0;
    std::string         DashPattern = "[] 0";
    std::string         RenderingIntent = "RelativeColorimetric";
    bool                StrokeAdjustment = false;
    std::string         BlendMode = "Normal";
    std::string         SoftMask = "None";
    double              AlphaConstant = 1.0;
    bool                AlphaSource = false;

    Matrix& GetTextRenderingMatrix();
    Matrix              GetTextRenderingMatrix() const;

    Matrix              GetUserSpaceMatrix() const;
};

PdfRect Measure(const DIGraphicsState& GS, const std::string& text);