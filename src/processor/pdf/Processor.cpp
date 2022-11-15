#include "Processor.h"

#include <processor/pdf/types/PageState.h>

#include <pdf/objects/PdfDocument.h>
#include <pdf/objects/PdfPage.h>
#include <pdf/objects/PdfFont.h>
#include <pdf/utils/tools.h>
#include <pdf/utils/ToUnicode.h>

#include <parser/pdf/ContentParser.h>

#include <array>

#define CASE_OP(op) \
case op : \
ProcessOperator_ ## op (state, command.command_queue); \
break;

#define OP_DEFN(op) \
static inline void ProcessOperator_ ## op(SPageState& state, const CSVVector& command_queue)

using CSVVector = std::vector<std::string_view>;

namespace processor::pdf {

static inline void ProcessCustomLine2(SPageState& state, const PdfPoint& point)
{
    if (state.PS.SubPath.size() < 2) { return; }

    const PdfPoint& last_point = state.PS.SubPath[state.PS.SubPath.size() - 2];

    if (point.x == last_point.x) {
        double lp_y = last_point.y;
        double y = point.y;
        //vertical line
        processor::types::VerticalLine vert;
        vert.x = point.x;
        if (state.GS.CTM.d < 0.0) {
            vert.y = std::min(lp_y, y);
        } else {
            vert.y = std::max(lp_y, y);
        }
        vert.height = std::abs(y - lp_y);
        vert.nPage = state.nPage;
        vert.Cma = state.GS.CTM.a;
        state.AllVerticalLines.emplace_back(std::move(vert));
    } else if (point.y == last_point.y) {
        double lp_x = last_point.x;
        double x = point.x;
        processor::types::HorizontalLine horz;
        if (state.GS.CTM.a < 0.0) {
            horz.x = std::max(lp_x, x);
        } else {
            horz.x = std::min(lp_x, x);
        }
        horz.y = point.y;
        horz.width = std::abs(x - lp_x);
        horz.nPage = state.nPage;
        horz.Cmd = state.GS.CTM.d;
        state.AllHorizontalLines.emplace_back(std::move(horz));
    }
}


static inline std::string HexToString_Ascii(std::string hex_string)
{
    std::string temp;
    for (size_t first = 0; first < hex_string.size(); first += 2) {
        std::string hex_char = hex_string.substr(first, 2);
        //0045
        char cc = 0;
        for (int i = 0; i < 2; i++) {
            char t = hex_char[i];
            if (t >= '0' && t <= '9') {
                cc |= (t - '0') << ((1 - i) * 4);
            } else if (t >= 'a' && t <= 'f') {
                cc |= 10 + ((t - 'a') << ((1 - i) * 4));
            } else if (t >= 'A' && t <= 'F') {
                cc |= 10 + ((t - 'A') << ((1 - i) * 4));
            }
        }
        temp += cc;
    }
    return temp;
}

static inline std::string HexToString_Unicode(CToUnicode* to_unicode, std::string hex_string)
{
    std::string retval;
    std::wstring temp;

    for (size_t first = 0; first < hex_string.size(); first += 4) {
        std::string hex_char = hex_string.substr(first, 4);
        //0045
        wchar_t cc = 0;
        for (int i = 0; i < 4; i++) {
            char t = hex_char[i];
            if (t >= '0' && t <= '9') {
                cc |= (t - '0') << ((3 - i) * 4);
            } else if (t >= 'a' && t <= 'f') {
                cc |= 10 + ((t - 'a') << ((3 - i) * 4));
            } else if (t >= 'A' && t <= 'F') {
                cc |= 10 + ((t - 'A') << ((3 - i) * 4));
            }
        }
        temp += to_unicode->GetMappedGlyph(cc);
    }
    return str_from_wstr(temp);
}
//Text Array
static inline std::vector<TextAtLocation> ProcessOperatorTJ(DIGraphicsState& GS, std::string_view text, size_t nPage)
{
    auto& TS = GS.TextState;
    auto ToUnicode = TS.Tfont->m_pToUnicode;
    auto first = text.begin();
    const auto last = text.end();
    std::string retval;
    const auto last_space = first;
    std::vector<TextAtLocation> texts;
    Matrix& tm = TS.Tm;
    CPdfFont* font = TS.Tfont;

    double dTextSpace = 0.0;
    //double dCurrentX = tm.x;
    //double dFirstX = dCurrentX;
    Matrix& mCurrent = tm;
    Matrix mFirst = mCurrent;


    for (; first != last; ++first) {
        if (*first == '[') {
            for (; first != last; ++first) {
                if (*first == ']') {
                    break;
                } else if (*first == '<') {
                    const auto hex_first = ++first;
                    for (; first != last; ++first) {
                        if (*first == '>') {
                            break;
                        }
                    }
                    std::string hex_string(hex_first, first);

                    //support these for real
                    double Tc = TS.Tc;
                    double Tw = TS.Tw;
                    double Th = TS.Th;
                    double Thr = (Th / 100.0);
                    double Tfs = TS.Tfs;
                    std::string temp;

                    if (ToUnicode) {
                        temp = HexToString_Unicode(ToUnicode, hex_string);
                    } else {
                        //its ascii mate, EASY!
                        temp = HexToString_Ascii(hex_string);
                    }
                    if (dTextSpace <= -478.0) {
                        auto copy_ts = GS;
                        copy_ts.TextState.Tm = mFirst;
                        texts.push_back({ copy_ts, retval, nPage });
                        retval.clear();
                        mFirst = mCurrent;
                    }

                    for (auto& c : temp) {
                        double gw = font->GetGlyphWidth(c);
                        double Tx = (((gw / 1000.0) - (dTextSpace / 1000.0)) * Tfs + Tc + Tw) * Thr;
                        //T(m) = T(tlm) * T(m)
                        Matrix Tupdate = Matrix::IdentityMatrix();
                        Tupdate.x = Tx;
                        Multiply(Tupdate, mCurrent);
                        mCurrent = Tupdate;
                        if (dTextSpace <= -478.0) {
                            mFirst = mCurrent;
                        }
                        retval += c;
                        dTextSpace = 0.0;
                    }
                    dTextSpace = 0.0;
                } else if (*first == '(') {
                    const auto tt = ++first;
                    std::vector<char> val;
                    for (; first != last; ++first) {
                        if (*first == ')') {
                            break;
                        } else if (*first == '\\') {
                            val.push_back(*(first + 1));
                            ++first;
                        } else {
                            val.push_back(*first);
                        }
                    }

                    //support these for real
                    double Tc = TS.Tc;
                    double Tw = TS.Tw;
                    double Th = TS.Th;
                    double Thr = Th / 100.0;
                    double Tfs = TS.Tfs;

                    std::string temp(val.begin(), val.end());

                    if (dTextSpace <= -478.0) {
                        auto copy_ts = GS;
                        copy_ts.TextState.Tm = mFirst;
                        texts.push_back({ copy_ts, retval, nPage });
                        retval.clear();
                        mFirst = mCurrent;
                    }

                    for (auto& c : temp) {
                        int ic = (unsigned char)c;
                        double gw = font->GetGlyphWidth(ic);
                        double Tx = (((gw / 1000.0) - (dTextSpace / 1000.0)) * Tfs + Tc + Tw) * Thr;
                        Matrix Tupdate = Matrix::IdentityMatrix();
                        Tupdate.x = Tx;
                        Multiply(Tupdate, mCurrent);
                        mCurrent = Tupdate;
                        if (dTextSpace <= -478.0) {
                            mFirst = mCurrent;
                        }
                        retval += c;
                        dTextSpace = 0.0;
                    }
                    dTextSpace = 0.0;

                } else if (*first == '-' || (*first >= '0' && *first <= '9')) {

                    const auto digit_first = first;
                    bool bNeg = false;
                    if (*first == '-') {
                        bNeg = true;
                        ++first;
                    }
                    for (; first != last; ++first) {
                        if (*first >= '0' && *first <= '9') {
                        } else if (*first == '.') {
                        } else {
                            break;
                        }
                    }

                    std::string numba(digit_first, first);
                    dTextSpace = atof(numba.c_str());
                    --first;
                }
            }
        }

        if (retval.empty() == false) {
            auto copy_ts = GS;
            copy_ts.TextState.Tm = mFirst;
            texts.push_back({ copy_ts, retval, nPage });
        }
    }

    return texts;
}

static inline std::string ProcessTextBlock(const DIGraphicsState& GS, std::string_view text)
{
    auto ToUnicode = GS.TextState.Tfont->m_pToUnicode;
    auto first = text.begin();
    auto last = text.end();

    //Tm is supposed to be updated as characters are shown. why dont i do that?

    std::string retval;

    const auto last_space = first;
    for (; first != last; ++first) {
        if (*first == '[') {
            for (; first != last; ++first) {
                if (*first == ']') {
                    break;
                } else if (*first == '<') {
                    const auto hex_first = ++first;
                    for (; first != last; ++first) {
                        if (*first == '>') {
                            break;
                        }
                    }
                    std::string hex_string(hex_first, first);
                    if (ToUnicode) {
                        retval += HexToString_Unicode(ToUnicode, hex_string);
                    } else {
                        //its ascii mate, EASY!
                        retval += HexToString_Ascii(hex_string);
                    }
                }
            }
        } else if (*first == '(') {
            //a "(, ), \" must have a \ before it
            const auto tt = ++first;
            std::vector<char> val;
            for (; first != last; ++first) {
                if (*first == ')') {
                    break;
                } else if (*first == '\\') {
                    val.push_back(*(first + 1));
                    ++first;
                } else {
                    val.push_back(*first);
                }
            }
            retval.append(val.begin(), val.end());
        } else if (*first == '<') {
            const auto hex_first = ++first;
            for (; first != last; ++first) {
                if (*first == '>') {
                    break;
                }
            }
            std::string hex_string(hex_first, first);
            if (ToUnicode) {
                retval += HexToString_Unicode(ToUnicode, hex_string);
            } else {
                //its ascii mate, EASY!
                retval += HexToString_Ascii(hex_string);
            }
        } else if (*first == '-' || (*first >= '0' && *first <= '9')) {

            const auto digit_first = first;
            bool bNeg = false;
            if (*first == '-') {
                bNeg = true;
                ++first;
            }
            for (; first != last; ++first) {
                if (*first >= '0' && *first <= '9') {
                } else {
                    break;
                }
            }

            std::string numba(digit_first, first);
            int num = atoi(numba.c_str());

            //move text a certain amount of pixels?  what the heck?

        }
    }
    return retval;
}

//bezier curves calculate by varying between t 0.0..1.0
//R(t) = (1 - t)^3 * P.0 + 3t(1 - t)^2 * P.1 + 3t^2(1 - t) * P.2 + t^3 * P.3
//When t = 0.0, the value of R(t) coincides with current point P.0
//When t = 1.0, the value of R(t) coincides with final point P.3
//curve does not in general, pass through the two control poits P.1 and P.2
//i wonder if this is true:
//R(t).x = (1 - t)^3 * P.0.x + 3t(1 - t)^2 * P.1.x + 3t^2(1 - t) * P.2.x + t^3 * P.3.x
//R(t).y = (1 - t)^3 * P.0.y + 3t(1 - t)^2 * P.1.y + 3t^2(1 - t) * P.2.y + t^3 * P.3.y


//we might be able to precompile different resolutions
static inline std::array<double, 4> CalculateBezierComponent(double t)
{
    std::array<double, 4> retval;
    const double invT = 1.0 - t;
    retval[0] = pow(invT, 3);
    retval[1] = 3 * t * pow(invT, 2);
    retval[2] = pow(3 * t, 2) * invT;
    retval[3] = pow(t, 3);
    return retval;
}


static inline double CalculateBezier(const std::array<double, 4>& t, double p0, double p1, double p2, double p3)
{
    return t[0] * p0 + t[1] * p1 + t[2] * p2 + t[3] * p3;
}


OP_DEFN(c)
{
    auto& PS = state.PS;
    //belzier curve
    //starting point command_queue[4,5]
    //command_queue[0-3] control points
    //append path
    if (PS.SubPath.empty() == false) {
        const PdfPoint& P0 = PS.SubPath.back();
        PdfPoint P1(atof(command_queue[0].data()), atof(command_queue[1].data()));
        PdfPoint P2(atof(command_queue[2].data()), atof(command_queue[3].data()));
        PdfPoint P3(atof(command_queue[4].data()), atof(command_queue[5].data()));
        for (double t = 0.0; t < 1.0; t += 0.1) {
            auto ts = CalculateBezierComponent(t);
            double nx = CalculateBezier(ts, P0.x, P1.x, P2.x, P3.x);
            double ny = CalculateBezier(ts, P0.y, P1.y, P2.y, P3.y);
            PS.SubPath.emplace_back(nx, ny);
        }
        PS.SubPath.emplace_back(P3);
    } else {
        const PdfPoint& P0 = PS.Path.back().back();
        PdfPoint P1(atof(command_queue[0].data()), atof(command_queue[1].data()));
        PdfPoint P2(atof(command_queue[2].data()), atof(command_queue[3].data()));
        PdfPoint P3(atof(command_queue[4].data()), atof(command_queue[5].data()));
        subpath_t sub;
        for (double t = 0.0; t < 1.0; t += 0.1) {
            auto ts = CalculateBezierComponent(t);
            double nx = CalculateBezier(ts, P0.x, P1.x, P2.x, P3.x);
            double ny = CalculateBezier(ts, P0.y, P1.y, P2.y, P3.y);
            sub.emplace_back(nx, ny);
        }
        sub.emplace_back(P3);
        PS.Path.emplace_back(std::move(sub));
    }
}

OP_DEFN(f)
{
    auto& PS = state.PS;
    if (PS.SubPath.empty() == false) {
        PS.SubPath.push_back(PS.SubPath.front());
        ProcessCustomLine2(state, PS.SubPath.back());
        PS.Path.emplace_back(std::move(PS.SubPath));
    }
    //Fill Path
    //Clear Path
    PS.Path.clear();
}

OP_DEFN(g)
{
    state.GS.ColorSpace.NonStroke = "/DeviceGray";
    state.GS.Color.NonStroke = trim_all(command_queue[0].data());
}

OP_DEFN(h)
{
    auto& PS = state.PS;
    if (PS.SubPath.empty() == false) {
        PS.SubPath.push_back(PS.SubPath.front());
        ProcessCustomLine2(state, PS.SubPath.back());
        PS.Path.emplace_back(std::move(PS.SubPath));
    }
}

OP_DEFN(j)
{
    //line join style in command_queue[0]
}

OP_DEFN(k)
{
    state.GS.ColorSpace.NonStroke = "/DeviceCMYK";
    {
        std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
        state.GS.Color.NonStroke = trim_all(joined);
    }
}

OP_DEFN(l)
{
    state.PS.SubPath.emplace_back(atof(command_queue[0].data()), atof(command_queue[1].data()));
    ProcessCustomLine2(state, state.PS.SubPath.back());
}

OP_DEFN(m)
{
    state.PS.SubPath.clear();
    state.PS.SubPath.emplace_back(atof(command_queue[0].data()), atof(command_queue[1].data()));
}

OP_DEFN(n)
{
    state.PS.SubPath.clear();
    state.PS.Path.clear();//dunno if clear is appropriate but this isnt trying to print to screen, yet
}

OP_DEFN(q)
{
    state.GraphicsStateQueue.push_back(state.GS);
}

OP_DEFN(v)
{
    //cubic bezier line
    //extend from current point to command_queue[2,3]
    //current point and command_queue[0,1] as control points
    //append path
    //new point is command_queue[2,3]
}

OP_DEFN(w)
{
    //starting point command_queue[2,3]
    //command_queue[0,1 2,3] control points
    //append path
    //new point is command_queue[2,3]
}

OP_DEFN(y)
{
    //extend from current point to command_queue[2,3]
    //command_queue[0,1 2,3] control points
    //append path
    //new point is command_queue[2,3]
}

OP_DEFN(f_s)
{
    auto& PS = state.PS;

    if (PS.SubPath.empty() == false) {
        PS.SubPath.push_back(PS.SubPath.front());
        ProcessCustomLine2(state, PS.SubPath.back());
        PS.Path.emplace_back(std::move(PS.SubPath));
    }
    //Fill Path Even-Odd Rule
    //Clear Path
    PS.Path.clear();

}

OP_DEFN(G)
{
    state.GS.ColorSpace.Stroke = "/DeviceGray";
    state.GS.Color.Stroke = trim_all(command_queue[0].data());
}

OP_DEFN(J)
{
    //line cap in command_queue[0]
}

OP_DEFN(K)
{
    state.GS.ColorSpace.Stroke = "/DeviceCMYK";
    {
        std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
        state.GS.Color.Stroke = trim_all(joined);
    }
}

OP_DEFN(M)
{

}

OP_DEFN(Q)
{
    state.GS = state.GraphicsStateQueue.back();
    state.GraphicsStateQueue.pop_back();
}

OP_DEFN(S)
{
    //Stroke path.
    //Then clear?
    state.PS.Path.clear();
}

OP_DEFN(W)
{
    //Set Clipping path equal to PS.Path
    state.GS.ClippingPath.Path = state.PS.Path;
    state.GS.ClippingPath.Rule = 0;
}

OP_DEFN(W_s)
{
    state.GS.ClippingPath.Path = state.PS.Path;
    state.GS.ClippingPath.Rule = 1;
}

OP_DEFN(cm)
{
    Matrix new_cm;
    new_cm.a = atof(command_queue[0].data());
    new_cm.b = atof(command_queue[1].data());
    new_cm.c = atof(command_queue[2].data());
    new_cm.d = atof(command_queue[3].data());
    new_cm.x = atof(command_queue[4].data());
    new_cm.y = atof(command_queue[5].data());

    Multiply(state.GS.CTM, new_cm);
}

OP_DEFN(cs)
{
    state.GS.ColorSpace.NonStroke = command_queue[0];
}

OP_DEFN(gs)
{
    std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
    //looks up the dictionary in the current page
    //state.page["Resources"]["ExtGState"][joined]
}

OP_DEFN(re)
{
    auto& PS = state.PS;
    PS.SubPath.clear();
    double re_x = atof(command_queue[0].data());
    double re_y = atof(command_queue[1].data());
    double re_cx = atof(command_queue[2].data());
    double re_cy = atof(command_queue[3].data());
    PS.SubPath.emplace_back(re_x, re_y);
    PS.SubPath.emplace_back(re_x + re_cx, re_y);
    ProcessCustomLine2(state, PS.SubPath.back());
    PS.SubPath.emplace_back(re_x + re_cx, re_y + re_cy);
    ProcessCustomLine2(state, PS.SubPath.back());
    PS.SubPath.emplace_back(re_x, re_y + re_cy);
    ProcessCustomLine2(state, PS.SubPath.back());
    PS.SubPath.emplace_back(re_x, re_y);
    ProcessCustomLine2(state, PS.SubPath.back());
    PS.Path.emplace_back(std::move(PS.SubPath));
}

OP_DEFN(rg)
{
    state.GS.ColorSpace.NonStroke = "/DeviceRGB";
    std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
    state.GS.Color.NonStroke = trim_all(joined);
}

OP_DEFN(Do)
{
    //Draw command_queue[0] Page["XObject"][name]
}

OP_DEFN(Tc)
{
    state.GS.TextState.Tc = atof(command_queue[0].data());
}

OP_DEFN(Tf)
{
    state.GS.TextState.Tf = command_queue[0];
    auto fnt = state.GS.TextState.Tf.substr(1);
    state.GS.TextState.Tfont = state.FontMaps[fnt];
    state.GS.TextState.Tfs = atof(command_queue[1].data());
    state.GS.TextState.Tfont->SetSize(state.GS.TextState.Tfs);
}

OP_DEFN(Tj)
{
    if (state.GS.TextState.Tmode == 3) {
    } else {
        auto copy_ts = state.GS;
        std::string tj_text = ProcessTextBlock(state.GS, command_queue[0]);
        state.AllTexts.push_back({ copy_ts, tj_text, state.nPage });
    }
}

OP_DEFN(Tm)
{
    auto& GS = state.GS;

    GS.TextState.Tm.a = atof(command_queue[0].data());
    GS.TextState.Tm.b = atof(command_queue[1].data());
    GS.TextState.Tm.c = atof(command_queue[2].data());
    GS.TextState.Tm.d = atof(command_queue[3].data());
    GS.TextState.Tm.x = atof(command_queue[4].data());
    GS.TextState.Tm.y = atof(command_queue[5].data());
    GS.TextState.Tlm = GS.TextState.Tm;
    GS.GetUserSpaceMatrix();
}

OP_DEFN(Tw)
{
    state.GS.TextState.Tw = atof(command_queue[0].data());
}

OP_DEFN(BT)
{
    state.GS.TextState.Tm = state.GS.TextState.Tlm = state.GS.TextState.Trm = Matrix::IdentityMatrix();
}

OP_DEFN(CS)
{
    state.GS.ColorSpace.Stroke = command_queue[0];
}

OP_DEFN(ET)
{

}

OP_DEFN(RG)
{
    state.GS.ColorSpace.Stroke = "/DeviceRGB";
    std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
    state.GS.Color.Stroke = trim_all(joined);
}

OP_DEFN(TJ)
{
    if (state.GS.TextState.Tmode == 3) {
    } else {
        for (auto& c : ProcessOperatorTJ(state.GS, command_queue[0], state.nPage)) {
            state.AllTexts.push_back({ c.GS, c.text, state.nPage });
        }
    }
}

OP_DEFN(scn)
{
    std::string joined = fmt::format("{}", fmt::join(command_queue, " "));
    state.GS.Color.NonStroke = trim_all(joined);

}

OP_DEFN(BDC)
{

}

OP_DEFN(BMC)
{

}

OP_DEFN(EMC)
{

}

SProcessedText ProcessText(CPdfDocument* pDocument)
{
    SProcessedText retval;

    auto pages = pDocument->GetPageContents();
    size_t nPage = 0;
    for (auto&& [pPage, sContents] : pages) {
        auto commands = parser::pdf::ParseContent(sContents);
        SPageState state(pPage, nPage++);

        for (auto& command : commands) {
            switch (command.command) 
            {
                using enum parser::pdf::CommandType;
                    CASE_OP(c)
                    CASE_OP(f)
                    CASE_OP(g)
                    CASE_OP(h)
                    CASE_OP(j)
                    CASE_OP(k)
                    CASE_OP(l)
                    CASE_OP(m)
                    CASE_OP(n)
                    CASE_OP(q)
                    CASE_OP(v)
                    CASE_OP(w)
                    CASE_OP(y)

                    CASE_OP(f_s)

                    CASE_OP(G)
                    CASE_OP(J)
                    CASE_OP(K)
                    CASE_OP(M)
                    CASE_OP(Q)
                    CASE_OP(S)
                    CASE_OP(W)

                    CASE_OP(W_s)

                    CASE_OP(cm)
                    CASE_OP(cs)
                    CASE_OP(gs)
                    CASE_OP(re)
                    CASE_OP(rg)

                    CASE_OP(Do)
                    CASE_OP(Tc)
                    CASE_OP(Tf)
                    CASE_OP(Tj)
                    CASE_OP(Tm)
                    CASE_OP(Tw)

                    CASE_OP(BT)
                    CASE_OP(ET)
                    CASE_OP(RG)
                    CASE_OP(TJ)

                    CASE_OP(scn)

                    CASE_OP(BDC)
                    CASE_OP(BMC)
                    CASE_OP(EMC)
            default:
                {
                    int look = 0;
                }
                break;
            }
        } //end for commands

        std::copy(state.AllTexts.begin(), state.AllTexts.end(), std::back_inserter(retval.AllTexts));
        std::copy(state.AllHorizontalLines.begin(), state.AllHorizontalLines.end(), std::back_inserter(retval.AllHorizontalLines));
        std::copy(state.AllVerticalLines.begin(), state.AllVerticalLines.end(), std::back_inserter(retval.AllVerticalLines));
    }

    return retval;
}

}