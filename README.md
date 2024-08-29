# Cliar
A Command Line Interface Argument parser based on Reflection.

## Usage
```cpp
#include <cliar.hpp>
#include <fmt/core.h>

// Define a struct of `option<T, names...>`
struct cli_args
{
    cliar::option<bool> verbose = false;  // simple boolean flag
    cliar::option<<std::optional<bool>> help = false;  // optional flag with comment

    cliar::option<int, "both option names are deduced"> this_deduces_both;
    cliar::option<float, "-l", "short name fixed, long name deduced"> deduced_long_name;
    cliar::option<std::string, "--short", "short name deduced, long name fixed"> deduce_short_name;
    cliar::option<std::optional<int>, "-b", "--both", "both option names fixed"> set_both = 100;
    cliar::option<std::optional<float>, "-", "--only-long", "disable short option"> only_long;
    cliar::option<std::optional<std::string>, "--", "-o", "disable long option"> only_short;
    // cliar::option<int, "--", "-", "can't disable both long and short"> i_wont_compile;
};

int main(int argc, char * argv[])
{
    auto args = std::span{argv, argv + argc};
    // Parse the command line option
    auto result = cliar::parse<cli_args>(std::span{argv, argv + argc});

    // Eventually print the help message
    if (*result.help) {
        fmt::print("{}\n", cliar::help<cli_args>());
        return 0;
    }

    // Access the members and do your stuff
    fmt::print("{}\n", *result.only_short);
}
```

## Dependencies
Cliar only depends on `fmt` (eventually provided via `conan`) and `reflect` (bundled in the library as header).
Note that the provided header for `reflect` is slightly modified to make it work with `-Wshadow` under GCC.
