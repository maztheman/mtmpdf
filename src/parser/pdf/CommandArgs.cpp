#include "CommandArgs.h"

namespace parser::pdf {

    using namespace std::string_view_literals;


    static const std::unordered_map<std::string_view, SCommandTypeArgs> PS_COMMANDS = {
        {"b"sv, {CommandType::b, 0, true}},
        {"c"sv, {CommandType::c, 6, true}},
        {"d"sv, {CommandType::d, 2}},
        {"f"sv, {CommandType::f, 0, true}},
        {"g"sv, {CommandType::g, 1, true}},
        {"h"sv, {CommandType::h, 0}},
        {"i"sv, {CommandType::i, 1}},
        {"j"sv, {CommandType::j, 1}},
        {"k"sv, {CommandType::k, 4}},
        {"l"sv, {CommandType::l, 2}},
        {"m"sv, {CommandType::m, 2}},
        {"n"sv, {CommandType::n, 0}},
        {"q"sv, {CommandType::q, 0}},
        {"s"sv, {CommandType::s, 0, true}},
        {"v"sv, {CommandType::v, 4}},
        {"w"sv, {CommandType::w, 1}},
        {"y"sv, {CommandType::y, 4}},
        {"'"sv, {CommandType::quote, 1}},
        {"\""sv, {CommandType::dbl_quote, 3}},

        {"b*"sv, {CommandType::b_s, 0}},
        {"f*"sv, {CommandType::f_s, 0}},

        {"B"sv, {CommandType::B, 0, true}},
        {"F"sv, {CommandType::F, 0}},
        {"G"sv, {CommandType::G, 1}},
        {"J"sv, {CommandType::J, 1}},
        {"K"sv, {CommandType::K, 4}},
        {"M"sv,{CommandType::M, 1}},
        {"Q"sv, {CommandType::Q, 0}},
        {"S"sv, {CommandType::S, 0, true}},
        {"W"sv, {CommandType::W, 0, true}},

        {"B*"sv, {CommandType::B_s, 0}},
        {"W*"sv, {CommandType::W_s, 0}},
        {"T*"sv, {CommandType::T_s, 0}},

        {"cm"sv, {CommandType::cm, 6}},
        {"cs"sv, {CommandType::cs, 1}},
        {"d0"sv, {CommandType::d0, 2}},
        {"d1"sv, {CommandType::d1, 6}},
        {"gs"sv, {CommandType::gs, 1}},
        {"re"sv, {CommandType::re, 4}},
        {"rg"sv, {CommandType::rg, 3}},
        {"ri"sv, {CommandType::ri, 1}},
        {"sc"sv, {CommandType::sc, -1, true}},
        {"sh"sv, {CommandType::sh, 1}},

        {"Do"sv, {CommandType::Do, 1}},
        {"Tc"sv, {CommandType::Tc, 1}},
        {"Td"sv, {CommandType::Td, 2}},
        {"Tf"sv, {CommandType::Tf, 2}},
        {"Tm"sv, {CommandType::Tm, 6}},
        {"Tj"sv, {CommandType::Tj, 1}},
        {"Tr"sv, {CommandType::Tr, 1}},
        {"Ts"sv, {CommandType::Ts, 1}},
        {"Tw"sv, {CommandType::Tw, 1}},
        {"Tz"sv, {CommandType::Tz, 1}},



        {"BT"sv, {CommandType::BT, 0}},
        {"BI"sv, {CommandType::BI, 0}},
        {"CS"sv, {CommandType::CS, 1}},
        {"EI"sv, {CommandType::EI, 0}},
        {"ET"sv, {CommandType::ET, 0}},
        {"ID"sv, {CommandType::ID, 0}},
        {"RG"sv, {CommandType::RG, 3}},
        {"SC"sv, {CommandType::SC, -1, true}},
        {"TD"sv, {CommandType::TD, 1}},
        {"TJ"sv, {CommandType::TJ, 1}},


        {"scn"sv, {CommandType::scn, -1}},
        {"BDC"sv, {CommandType::BDC, 2}},
        {"BMC"sv, {CommandType::BMC, 1}},
        {"EMC"sv, {CommandType::EMC, 0}},
        {"SCN"sv, {CommandType::SCN, -1}},

    };

    static std::unordered_map<CommandType, std::string_view> COMMANDS_TO_PS = {
        {CommandType::b, "b"sv},
        {CommandType::c, "c"sv},
        {CommandType::d, "d"sv},
        {CommandType::f, "f"sv},
        {CommandType::g, "g"sv},
        {CommandType::h, "h"sv},
        {CommandType::i, "i"sv},
        {CommandType::j, "j"sv},
        {CommandType::k, "k"sv},
        {CommandType::l, "l"sv},
        {CommandType::m, "m"sv},
        {CommandType::n, "n"sv},
        {CommandType::q, "q"sv},
        {CommandType::s, "s"sv},
        {CommandType::v, "v"sv},
        {CommandType::w, "w"sv},
        {CommandType::y, "y"sv},
        {CommandType::quote, "'"sv},
        {CommandType::dbl_quote, "\""sv},

        {CommandType::b_s, "b*"sv},
        {CommandType::f_s, "f*"sv},

        {CommandType::B, "B"sv},
        {CommandType::F, "F"sv},
        {CommandType::G, "G"sv},
        {CommandType::J, "J"sv},
        {CommandType::K, "K"sv},
        {CommandType::M, "M"sv},
        {CommandType::Q, "Q"sv},
        {CommandType::S, "S"sv},
        {CommandType::W, "W"sv},

        {CommandType::B_s, "B*"sv},
        {CommandType::W_s, "W*"sv},
        {CommandType::T_s, "T*"sv},

        {CommandType::cm, "cm"sv},
        {CommandType::cs, "cs"sv},
        {CommandType::d0, "d0"sv},
        {CommandType::d1, "d1"sv},
        {CommandType::gs, "gs"sv},
        {CommandType::re, "re"sv},
        {CommandType::rg, "rg"sv},
        {CommandType::ri, "ri"sv},
        {CommandType::sc, "sc"sv},
        {CommandType::sh, "sh"sv},

        {CommandType::Do, "Do"sv},
        {CommandType::Tc, "Tc"sv},
        {CommandType::Td, "Td"sv},
        {CommandType::Tf, "Tf"sv},
        {CommandType::Tm, "Tm"sv},
        {CommandType::Tj, "Tj"sv},
        {CommandType::Tr, "Tr"sv},
        {CommandType::Ts, "Ts"sv},
        {CommandType::Tw, "Tw"sv},
        {CommandType::Tz, "Tz"sv},

        {CommandType::BT, "BT"sv},
        {CommandType::BI, "BI"sv},
        {CommandType::CS, "CS"sv},
        {CommandType::EI, "EI"sv},
        {CommandType::ET, "ET"sv},
        {CommandType::ID, "ID"sv},
        {CommandType::RG, "RG"sv},
        {CommandType::SC, "SC"sv},
        {CommandType::TD, "TD"sv},
        {CommandType::TJ, "TJ"sv},

        {CommandType::scn, "scn"sv},

        {CommandType::BDC, "BDC"sv},
        {CommandType::BMC, "BMC"sv},
        {CommandType::EMC, "EMC"sv},
        {CommandType::SCN, "SCN"sv},

    };


    const SCommandTypeArgs* is_command(std::string_view sv)
    {
        const auto ret = PS_COMMANDS.find(sv);
        return ret != PS_COMMANDS.end() ? &ret->second : nullptr;
    }

    std::string_view to_string(CommandType command)
    {
        return COMMANDS_TO_PS[command];
    }
}