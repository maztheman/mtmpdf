#pragma once

#include "Command.h"

namespace parser::pdf {

    struct SCommandTypeArgs
    {
        CommandType command;
        int argCount;
        bool couldBeAnother{ false };
    };


    const SCommandTypeArgs* is_command(std::string_view sv);

    std::string_view to_string(CommandType command);
}