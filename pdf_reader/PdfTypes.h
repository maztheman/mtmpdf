#pragma once
#include <string>

class CPdfFont;

struct Matrix
{
    double a, b, c, d, x, y;

    void ToIdentity()
    {
        a = d = 1.0;
        b = c = 0.0;
        x = y = 0.0;
    }

    static Matrix IdentityMatrix() {
        Matrix a;
        a.ToIdentity();
        return a;
    }
};

struct PdfPoint
{
    PdfPoint(double in_x, double in_y) 
        : x(in_x), y(in_y)
    {

    }
    double x, y;
};

typedef std::vector<PdfPoint> subpath_t;
typedef std::vector<subpath_t> path_t;

struct SClippingPath
{
    path_t              Path;
    int                 Rule;
};

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

    CPdfFont*           Tfont = nullptr;

    Matrix              Tm = Matrix::IdentityMatrix();
    Matrix              Tlm = Matrix::IdentityMatrix();
    Matrix              Trm = Matrix::IdentityMatrix();
};


struct SMultiColor
{
    std::string         Stroke;
    std::string         NonStroke;


    static SMultiColor FromDefault(const std::string& Default)
    {
        SMultiColor retval;
        retval.Stroke = Default;
        retval.NonStroke = Default;
        return retval;
    }
};

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

    Matrix&             GetTextRenderingMatrix();
    Matrix              GetTextRenderingMatrix() const;

    Matrix              GetUserSpaceMatrix() const;
};

struct PathState
{
    subpath_t           SubPath;
    path_t              Path;
};

struct TextAtLocation
{
    DIGraphicsState GS;
    std::string text;
    size_t nPage;
};

struct VerticalLine
{
    double x;
    double y;
    double height;
    double Cma;
    size_t nPage;
};

struct HorizontalLine
{
    double x;
    double y;
    double width;
    double Cmd;
    size_t nPage;
};

struct PdfRect
{
    double x, y;
    double width, height;
};

struct PdfColumn
{
    PdfRect rect;
    PdfRect maybe_rect;
    bool has_maybe_rect;
    bool header_is_centered;
    std::string name;
};

PdfRect Measure(const DIGraphicsState& GS, const std::string& text);

