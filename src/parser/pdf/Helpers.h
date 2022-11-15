#pragma once

namespace parser::pdf {

    struct SCommandState;

    bool is_special(const char ch);
    bool is_non_command(const char ch);
    bool is_in_property(const SCommandState& state);
    bool is_in_group(const SCommandState& state);
    bool is_in_array(const SCommandState& state);
    bool is_parent_array(const SCommandState& state);
    bool is_parent_property(const SCommandState& state);
    bool is_tag_start(const SCommandState& state);
    bool is_tag_end(const SCommandState& state);
    bool is_property_start(const SCommandState& state);
    bool is_property_end(const SCommandState& state);
    bool is_array_start(const SCommandState& state);
    bool is_array_end(const SCommandState& state);
    bool is_group_start(const SCommandState& state);
    bool is_group_end(const SCommandState& state);
}