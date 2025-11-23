#pragma once

#include <debug/log.hpp>
#include <iostream>
#include <ostream>
#include <sstream>

namespace prnt {
namespace internals {
    static constexpr std::string_view whitespace = " ............................... ";
    static constexpr size_t lineLength = 28;
}  // namespace internals

    [[maybe_unused]] static std::string inject_line_divider(std::string_view command, std::string_view helpText)
    {
        size_t lengthLeft = internals::lineLength - command.length();
        if (lengthLeft == 0 || lengthLeft > 128)  // might wrap
            throw new std::range_error("AddLineDevider is too short!");

        std::stringstream output;
        output << " # " << command << ' ';
        while (lengthLeft > 0) {
            output << '.';
            --lengthLeft;
        }
        output << " " << helpText;
        return output.str();
    }

    inline std::ostream& out(std::cout);
    inline std::ostream& err(std::cerr);
    
}  // namespace prnt