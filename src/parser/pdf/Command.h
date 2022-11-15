#pragma once

namespace parser::pdf {
    enum class CommandType
    {
        b,
        d,
        c,
        f,
        g,
        h,
        i,
        j,
        k,
        l,
        m,
        n,
        q,
        s,
        v,
        w,
        y,

        b_s,
        f_s,

        B,
        F,
        G,
        J,
        K,
        M,
        Q,
        S,
        W,

        B_s,
        W_s,
        T_s,

        cm,
        cs,
        d0,
        d1,
        gs,
        re,
        rg,
        ri,
        sc,
        sh,

        Do,
        Tc,
        Td,
        Tf,
        Tm,
        Tj,
        Tr,
        Ts,
        Tw,
        Tz,

        BI,
        BT,
        CS,
        EI,
        ET,
        ID,
        RG,
        SC,
        TD,
        TJ,

        scn,

        BDC,
        BMC,
        EMC,
        SCN,

        quote,
        dbl_quote,
        property,
        array
    };

    struct SCommand
    {
        SCommand(CommandType cmd, std::vector<std::string_view>&& que)
            : command(cmd)
            , command_queue(std::move(que))
        {

        }

        CommandType command;
        std::vector<std::string_view> command_queue;
    };
}