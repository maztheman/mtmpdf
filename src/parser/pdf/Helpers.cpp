#include "Helpers.h"
#include "CommandState.h"

namespace parser::pdf {

    bool is_special(const char ch)
    {
        if (ch == '/') {
            return true;
        } else if (ch == '<') {
            return true;
        } else if (ch == '>') {
            return true;
        } else if (ch == '[') {
            return true;
        } else if (ch == ']') {
            return true;
        } else if (ch == '(') {
            return true;
        } else if (ch == ')') {
            return true;
        } else if (isspace(ch)) {
            return true;
        }
        return false;
    }

    bool is_non_command(const char ch)
    {
        if (ch >= 'a' && ch <= 'z') {
            return false;
        } else if (ch >= 'A' && ch <= 'Z') {
            return false;
        } else if (ch == '*') {
            return false;
        }
        return true;
    }

    bool is_in_property(const SCommandState& state)
    {
        if (state.props.empty()) {
            return false;
        }
        return state.props.back().type == ParentType::PROPERTY;
    }

    bool is_in_group(const SCommandState& state)
    {
        if (state.props.empty()) {
            return false;
        }
        return state.props.back().type == ParentType::GROUP;
    }

    bool is_in_array(const SCommandState& state)
    {
        if (state.props.empty()) {
            return false;
        }
        return state.props.back().type == ParentType::ARRAY;
    }

    bool is_parent_array(const SCommandState& state)
    {
        if (state.props.size() < 2) {
            return false;
        }
        return (state.props.rbegin() + 1)->type == ParentType::ARRAY;
    }

    bool is_parent_property(const SCommandState& state)
    {
        if (state.props.size() < 2) {
            return false;
        }
        return (state.props.rbegin() + 1)->type == ParentType::PROPERTY;
    }

    bool is_tag_start(const SCommandState& state)
    {
        return !is_in_group(state) && !is_in_property(state) && state.c == '/';
    }

    bool is_tag_end(const SCommandState& state)
    {
        return is_special(state.c);
    }

    bool is_property_start(const SCommandState& state)
    {
        return state.c == '<' && state.isNext('<');
    }

    bool is_property_end(const SCommandState& state)
    {
        if (state.props.empty()) {
            return false;
        }
        return  state.c == '>' && state.isNext('>');
    }

    bool is_array_start(const SCommandState& state)
    {
        return !is_in_property(state) && state.c_last != '\\' && state.c == '[';
    }

    bool is_array_end(const SCommandState& state)
    {
        return !is_in_property(state) && state.c_last != '\\' && state.c == ']';
    }

    bool is_group_start(const SCommandState& state)
    {
        return state.c_last != '\\' && state.c == '(';
    }

    bool is_group_end(const SCommandState& state)
    {
        return state.c_last != '\\' && state.c == ')';
    }
}