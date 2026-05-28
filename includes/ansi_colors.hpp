#pragma once

namespace ansiColors {

    // reset
    constexpr const char* reset = "\033[0m";

    // styles
    constexpr const char* bold       = "\033[1m";
    constexpr const char* dim        = "\033[2m";
    constexpr const char* italic     = "\033[3m";
    constexpr const char* underline  = "\033[4m";
    constexpr const char* blink      = "\033[5m";
    constexpr const char* invert     = "\033[7m";
    constexpr const char* strike     = "\033[9m";

    // normal foreground
    constexpr const char* black   = "\033[30m";
    constexpr const char* red     = "\033[31m";
    constexpr const char* green   = "\033[32m";
    constexpr const char* yellow  = "\033[33m";
    constexpr const char* blue    = "\033[34m";
    constexpr const char* magenta = "\033[35m";
    constexpr const char* cyan    = "\033[36m";
    constexpr const char* white   = "\033[37m";

    // bright foreground
    constexpr const char* bright_black   = "\033[90m";
    constexpr const char* bright_red     = "\033[91m";
    constexpr const char* bright_green   = "\033[92m";
    constexpr const char* bright_yellow  = "\033[93m";
    constexpr const char* bright_blue    = "\033[94m";
    constexpr const char* bright_magenta = "\033[95m";
    constexpr const char* bright_cyan    = "\033[96m";
    constexpr const char* bright_white   = "\033[97m";

    // background
    constexpr const char* bg_black   = "\033[40m";
    constexpr const char* bg_red     = "\033[41m";
    constexpr const char* bg_green   = "\033[42m";
    constexpr const char* bg_yellow  = "\033[43m";
    constexpr const char* bg_blue    = "\033[44m";
    constexpr const char* bg_magenta = "\033[45m";
    constexpr const char* bg_cyan    = "\033[46m";
    constexpr const char* bg_white   = "\033[47m";

    // bright background
    constexpr const char* bg_bright_black   = "\033[100m";
    constexpr const char* bg_bright_red     = "\033[101m";
    constexpr const char* bg_bright_green   = "\033[102m";
    constexpr const char* bg_bright_yellow  = "\033[103m";
    constexpr const char* bg_bright_blue    = "\033[104m";
    constexpr const char* bg_bright_magenta = "\033[105m";
    constexpr const char* bg_bright_cyan    = "\033[106m";
    constexpr const char* bg_bright_white   = "\033[107m";

}