#include "ContentParser.h"
#include "CommandState.h"
#include "Helpers.h"
#include "CommandArgs.h"

namespace parser::pdf {

    using namespace std::string_view_literals;

    static inline void ProcessTag(SCommandState& state)
    {
        state.lop = state.first;
        state.next();
        for (; state(); state.next()) {
            if (is_tag_end(state)) {
                if (is_in_property(state)) {
                } else {
                    state.retval.push_back(state.currentValue());
                }
                break;
            }
        }
        state.lop = state.first;
        state.prev();
    }


    static inline void ProcessProperty(SCommandState& state)
    {
        state.next();//move ahead one
        state.next();

        if (state()) {

            state.lop = state.first;
            state.props.push_back({ ParentType::PROPERTY, state.first - 2 });
        }

        state.prev();
    }

    static inline int ProcessPropertyEnd(SCommandState& state)
    {
        if (state.props.empty()) {
            fmt::print(stderr, "bad properties pos: {}", state.first - state.beg);
            return -1;
        }
        auto& st = state.props.back();
        if (st.type != ParentType::PROPERTY) {
            fmt::print(stderr, "property mismatch pos: {}", state.first - state.beg);
            return -1;
        }
        state.next();
        state.next();
        state.retval.push_back(state.fromProps());
        state.props.pop_back();
        state.lop = state.first;
        state.prev();
        return 0;
    }

    static inline void ProcessGroup(SCommandState& state)
    {
        state.props.push_back({ ParentType::GROUP, state.first });
    }

    static inline int ProcessGroupEnd(SCommandState& state)
    {
        if (state.props.empty()) {
            fmt::print(stderr, "bad properties pos: {}", state.first - state.beg);
            return -1;
        }
        auto& st = state.props.back();
        if (st.type != ParentType::GROUP) {
            fmt::print(stderr, "group mismatch pos: {}", state.first - state.beg);
            return -1;
        }
        if (!is_parent_array(state)) {
            state.next();
            state.retval.push_back(state.fromProps());
            state.props.pop_back();
            state.lop = state.first;
            state.prev();
        } else {
            //you are an in array, continue to process forwards
            state.props.pop_back();
        }
        return 0;
    }

    static inline void ProcessArray(SCommandState& state)
    {
        state.props.push_back({ ParentType::ARRAY, state.first });
    }

    static inline int ProcessArrayEnd(SCommandState& state)
    {
        if (state.props.empty()) {
            fmt::print(stderr, "bad properties pos: {}", state.first - state.beg);
            return -1;
        }
        auto& st = state.props.back();
        if (st.type != ParentType::ARRAY) {
            fmt::print(stderr, "array mismatch pos: {}", state.first - state.beg);
            return -1;
        }
        state.next();
        state.retval.push_back(state.fromProps());
        state.props.pop_back();
        state.lop = state.first;
        state.prev();
        return 0;
    }

    static inline bool ProcessCommand(SCommandState& state)
    {
        //check if its an operator
        if (auto cmd = is_command(state.currentValue()); cmd) {
            //we need to consume the command right now.
            if (cmd->couldBeAnother) {
                state.push();
                //scan ahead until non-alpha
                for (; state() && !is_non_command(state.c); state.next()) {
                }

                if (auto nx = is_command(state.currentValue()); nx != nullptr) {
                    cmd = nx;
                    state.lop = state.first;
                    state.prev();
                    state.popUndo();
                } else {
                    state.pop();
                }
            } else {
                state.lop = state.first;
            }

            if (cmd->argCount != -1 && state.retval.size() != cmd->argCount) {
                fmt::print(stderr, "argument mismatch for command {}", to_string(cmd->command));
            }

            state.commands.emplace_back(cmd->command, std::move(state.retval));
            return true;
        } else if (state.c_last == 'd' && (state.c == '0' || state.c == '1')) {
            //super special case
            if (auto cmd = is_command(state.c == '0' ? "d0"sv : "d1"sv); cmd) {
                if (cmd->argCount != -1 && state.retval.size() != cmd->argCount) {
                    fmt::print(stderr, "argument mismatch for command {}", to_string(cmd->command));
                }

                state.commands.emplace_back(cmd->command, std::move(state.retval));
                return true;
            }
        }
        return false;
    }

    static inline void ProcessCurrentValue(SCommandState& state)
    {
        if (auto sv = state.currentValue(); sv.empty() == false) {
            if (!ProcessCommand(state)) {
                state.retval.push_back(state.currentValue());
            }
        }
    }

    static inline void ProcessSpace(SCommandState& state)
    {
        if (isspace(state.c_last)) {
            state.lop = state.first + 1;
        } else {
            if (is_in_property(state) || is_in_group(state)) {
            } else {
                ProcessCurrentValue(state);
            }
            state.lop = state.first + 1;
        }
    }

    std::vector<SCommand> ParseContent(const std::string& sFullContents)
    {
        SCommandState  state;
        state.intialize(sFullContents);

        for (; state(); state.next()) {
            if (is_property_start(state)) {
                ProcessProperty(state);
            } else if (is_property_end(state)) {
                if (auto rc = ProcessPropertyEnd(state); rc == -1) {
                    return {};
                }
            } else if (is_tag_start(state)) {
                ProcessTag(state);
            } else if (is_group_start(state)) {
                if (state.props.empty()) {
                    ProcessCommand(state);
                }
                ProcessGroup(state);
            } else if (is_group_end(state)) {
                if (auto rc = ProcessGroupEnd(state); rc == -1) {
                    return {};
                }
            } else if (is_array_start(state)) {
                if (state.props.empty()) {
                    ProcessCommand(state);
                }
                ProcessArray(state);
            } else if (is_array_end(state)) {
                if (auto rc = ProcessArrayEnd(state); rc == -1) {
                    return {};
                }
            } else if (isspace(state.c)) {
                ProcessSpace(state);
            } else {
                if (!is_in_group(state)) {
                    ProcessCommand(state);
                }
            }
        }

        ProcessCurrentValue(state);

        return state.commands;
    }
}