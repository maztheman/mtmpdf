#pragma once

#include "Command.h"

namespace parser::pdf
{
    std::vector<SCommand> ParseContent(const std::string& sFullContents);
}