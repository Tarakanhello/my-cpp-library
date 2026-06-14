#ifndef SHOW_DEBUG_LOG_INFO_H_INCLUDED
#define SHOW_DEBUG_LOG_INFO_H_INCLUDED

#include <iostream>
#include <source_location>
#include <string_view>

namespace mylib
{
    namespace debug
    {
#define SHOW_DEBUG_LOG_INFO(expr) \
                            mylib::debug::showDebugLogInfoImpl(#expr, \
                            (expr), \
                            std::source_location::current())




#define SHOW_DEBUG_LOG_INFO_MSG(expr, msg) \
                                mylib::debug::showDebugLogInfoImpl(#expr, \
                                (expr), \
                                std::source_location::current(), \
                                msg)




        template<typename T>
        void showDebugLogInfoImpl(std::string_view exprText,
                                  const T& result,
                                  const std::source_location& loc,
                                  std::string_view message = "")
        {
            std::cerr << "\nfile: "   << loc.file_name()     << ";\n"
                      << "line: "     << loc.line()          << ";\n"
                      << "column: "   << loc.column()        << ";\n"
                      << "function: " << loc.function_name() << ";\n"
                      << "var: "      << exprText            << ";\n"
                      << "result: "   << result              << ";\n"
                      << "message: "  << (message.empty() ? "-" : message) << '.'
                      << std::endl;
        }
    } // end debug namespace
} // end namespace mylib

#endif // SHOW_DEBUG_LOG_INFO_H_INCLUDED
