/**
 * @author      : rbrugo (brugo.riccardo@gmail.com)
 * @file        : main
 * @created     : Tuesday Aug 27, 2024 18:54:02 CEST
 * @description : 
 */

#include "cliar.hpp"
#include <fmt/std.h>

struct cli_args
{
    cliar::option<bool> verbose;  // simple flag
    cliar::option<std::optional<bool>, "optional flag with comment"> with_comment = false;

    cliar::option<int, "both option names are deduced"> this_deduces_both;
    cliar::option<float, "-l", "short name fixed, long name deduced"> deduced_long_name;
    cliar::option<std::string, "--short", "short name deduced, long name fixed"> deduce_short_name;
    cliar::option<std::optional<int>, "-b", "--both", "both option names fixed"> set_both = 100;
    cliar::option<std::optional<float>, "-", "--only-long", "disable short option"> only_long;
    cliar::option<std::optional<std::string>, "--", "-o", "disable long option"> only_short;
    // cliar::option<int, "--", "-", "can't disable both long and short"> i_wont_compile;
};

int main()
{
    // fmt::print("{}\n", cliar::help<cli_args>(argv[0], "Additional help back there"));
    try {
        auto args1 = std::vector<char const *>{
            "-v",
            "-t", "1",
            "-l", "12.34",
            "-d", "test",
            "--only-long", "-1.1",
            "-w", "false"
        };
        auto res1 = cliar::parse<cli_args>(args1);

        auto args2 = std::vector<char const *>{
            "--verbose",
            "--this-deduces-both", "1",
            "--deduced-long-name", "12.34",
            "--short", "test",
            "--only-long", "-1.1",
            "--set-both=100"
        };
        auto res2 = cliar::parse<cli_args>(args2);

        bool all_ok = true;
        reflect::for_each([&res1, &res2, &all_ok](auto I) {
            auto const & value1 = reflect::get<I>(res1);
            auto const & value2 = reflect::get<I>(res2);
            if (value1 != value2) {
                fmt::print("Error for member {}: {} != {}\n", reflect::member_name<I, cli_args>(), value1, value2);
                all_ok = false;
            }
        }, res1);
        if (all_ok) {
            fmt::print("All tests passed successfully!\n");
        }
    } catch (cliar::missing_required_option const & exc) {
        fmt::print("Error: {}\n", exc.what());
    }
}
