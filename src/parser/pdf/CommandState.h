#pragma once

#include "Command.h"

namespace parser::pdf
{
    using svi = std::string_view::const_iterator;

    struct SParent
    {
        int type;
        svi beg;
    };

    struct SCommandState
    {
        struct SStack
        {
            svi first;
            char c;
            char c_last;
        };

        std::string_view svContents;
        std::vector<std::string_view> retval;
        std::vector<SCommand> commands;
        svi beg;
        svi lop;
        svi first;
        std::deque<SParent> props;
        char c;
        char c_last;
        std::deque<SStack> stack;

        void intialize(const std::string& sFullContents)
        {
            svContents = sFullContents;
            first = lop = beg = svContents.begin();
            c_last = 0;
            if (operator()()) {
                c = *first;
            }
        }

        bool check_add_sv(svi f)
        {
            if ((f - lop) > 0) {
                retval.push_back(svContents.substr(lop - beg, f - lop));
                return true;
            }
            return false;
        }


        /// <summary>
        /// peeks ahead to see if character matches
        /// </summary>
        /// <param name="ch"></param>
        /// <returns></returns>
        bool isNext(const char ch) const {
            if ((first + 1) != svContents.end()) {
                return *(first + 1) == ch;
            }
            return false;
        }

        bool isSpaceOrSpecial() const {
            if (first == svContents.end()) {
                return true;//end is special
            }
            const char nx = *(first);
            if (isspace(nx)) {
                return true;
            }
            if (nx == '<' || nx == '(' || nx == '[') {
                return true;
            }

            return false;
        }

        void prev()
        {
            --first;
            c = c_last;
        }

        /// <summary>
        /// move onto the next piece
        /// </summary>
        void next()
        {
            //TODO: make this better?

            if (operator()()) {
                c_last = c;
                ++first;
                if (operator()()) {
                    c = *first;
                }
            }
        }

        void push()
        {
            stack.push_back({ first, c, c_last });
        }

        void popUndo()
        {
            stack.pop_back();
        }

        void pop()
        {
            auto& aa = stack.back();
            first = aa.first;
            c = aa.c;
            c_last = aa.c_last;
            stack.pop_back();
        }

        /// <summary>
        /// are we at the end?
        /// </summary>
        /// <param name="state"></param>
        bool operator()() const
        {
            return first != svContents.end();
        }

        std::string_view currentValue(size_t additional = 0) const {
            return svContents.substr(lop - beg, first - lop + additional);
        }

        std::string_view fromProps() const {
            return svContents.substr(props.back().beg - beg, first - props.back().beg);
        }
    };

    namespace ParentType {
        constexpr auto PROPERTY = 1;
        constexpr auto GROUP = 2;
        constexpr auto ARRAY = 3;
    }
}