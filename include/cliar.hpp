/**
 * @author      : rbrugo (brugo.riccardo@gmail.com)
 * @file        : cliar
 * @created     : Monday Aug 26, 2024 18:14:12 CEST
 * @description : 
 * */

#ifndef CLIAR_HPP
#define CLIAR_HPP

#include "reflect"
#include <type_traits>
#include <string>
#include <vector>
#include <fmt/ranges.h>
#include <ranges>

namespace cliar
{

namespace refl {
template <typename>
constexpr inline auto is_optional = false;
template <template <typename> class C, class T>
constexpr inline auto is_optional<C<T>> = std::same_as<C<T>, std::optional<T>>;

template <typename Aggregate, auto I>
using member_type = std::remove_cvref_t<decltype(reflect::get<I>(std::declval<Aggregate>()))>;

template <auto X>
struct consteval_value
{
    constexpr static inline decltype(X) value = X;
};

template <bool Cond, auto First, auto Second>
constexpr inline auto conditional_v = [] {
    if constexpr (Cond) {
        return First;
    } else {
        return Second;
    }
}();

consteval auto as_fixed_string(char ch) {
    char data[2]{ch, '\0'};
    return reflect::fixed_string{data};
}

/**
 * @brief Concatenate two `fixed_string` into a single one
 *
 * @tparam a the `fixed_string` to be placed first
 * @tparam b the `fixed_string` to be placed last
 * @return the resulting `fixed_string`
 */
// template <reflect::fixed_string a, reflect::fixed_string b>
template <typename Ch, std::size_t S1, std::size_t S2>
consteval auto concat(reflect::fixed_string<Ch, S1> const & a, reflect::fixed_string<Ch, S2> const & b)
{
    // constexpr auto S1 = a.size();
    // constexpr auto S2 = b.size();
    using T = std::remove_cvref_t<decltype(a.data[0])>;

    T data[S1 + S2 + 1]{};
    auto it = std::ranges::copy_n(a.data, S1, data);
    std::ranges::copy_n(b.data, S2, it.out);

    return reflect::fixed_string{data};
}

template <typename Ch, std::size_t S1, std::size_t S2>
consteval auto concat(Ch const (&a)[S1], reflect::fixed_string<Ch, S2> const & b)
{
    return concat(reflect::fixed_string{a}, b);
}
template <typename Ch, std::size_t S1, std::size_t S2>
consteval auto concat(reflect::fixed_string<Ch, S1> const & a, Ch const (&b)[S2])
{
    return concat(a, reflect::fixed_string{b});
}

/**
 * @brief turns a `fixed_string` in kebab case, replacing `_` with `-`
 *
 * @tparam C `value_type` of the `fixed_string`
 * @param s the `fixed_string` to modify
 * @return the modified string
 */
template <typename C, std::size_t N>
consteval auto to_kebab_case(reflect::fixed_string<C, N> s)
{
    auto data = std::span{s.data, s.size()};
    std::replace(data.begin(), data.end(), '_', '-');
    return s;
}

/**
 * @brief Generates a name for a given type
 *
 * Why manually and not automatically via `reflect::type_name<T>()`?
 * Basically to
 * - get a uniform name for similar types (signed integers, unsegned integers, floating point)
 * - get `string` / `optional<T>` as name for `std::string` and `std::optional<T>`
 * - reject invalid types
 *
 * @tparam T the type whose name is required
 * @return the type name
 */
template <typename T>
[[nodiscard]]
consteval auto type_name() noexcept
{
    using type = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<type, bool>) {
        return reflect::fixed_string{"bool"};
    }
    else if constexpr (std::signed_integral<type>) {
        return reflect::fixed_string{"int"};
    }
    else if constexpr (std::unsigned_integral<type>) {
        return reflect::fixed_string{"unsigned int"};
    }
    else if constexpr (std::floating_point<type>) {
        return reflect::fixed_string{"float"};
    }
    else if constexpr (std::same_as<type, std::string>) {
        return reflect::fixed_string{"string"};
    }
    else if constexpr (refl::is_optional<type> and not refl::is_optional<typename type::value_type>) {
        return refl::concat(refl::concat("optional<", type_name<typename type::value_type>()), ">");
    } else {
        static_assert(false, "invalid type for serialization");
    }
}

template <typename T, std::size_t I>
    requires std::is_aggregate_v<std::remove_cvref_t<T>>
consteval auto has_default_value()
{
    auto test = T{};
    auto const & member = reflect::get<I>(test);
    using value_type = typename std::remove_cvref_t<decltype(member)>::value_type;

    if constexpr (is_optional<value_type>) {
        return member.has_value();
    } else {
        return false;
    }
}

template <typename Cli, std::size_t I>
    requires (has_default_value<Cli, I>())
consteval auto get_default_value()
{
    return std::move(*reflect::get<I>(Cli{}));
}

template <typename Cli>
consteval auto to_tuple_of_optionals_impl()
{
    return []<auto ...I>(std::index_sequence<I...>) {
        return std::tuple{std::optional<refl::member_type<Cli, I>>{}...};
    }(std::make_index_sequence<reflect::size<Cli>()>());
}

template <typename T>
using to_tuple_of_optionals = decltype(to_tuple_of_optionals_impl<T>());
}  // namespace refl

namespace detail {
template <reflect::fixed_string Str>
consteval bool is_long_name() { return std::string_view{Str}.starts_with("--"); }
template <reflect::fixed_string Str>
consteval bool is_short_name() { return not is_long_name<Str>() and std::string_view{Str}.starts_with("-"); }
template <reflect::fixed_string Str>
consteval bool is_description() { return not is_long_name<Str>() and not is_short_name<Str>(); }

template <reflect::fixed_string ...Str>
consteval bool long_name_disabled() { return ((is_long_name<Str>() and std::string_view{Str}.size() == 2) or ...); }
template <reflect::fixed_string ...Str>
consteval bool short_name_disabled() { return ((is_short_name<Str>() and std::string_view{Str}.size() == 1) or ...); }
}  // namespace detail



template <typename T>
concept valid_option_primitive = std::same_as<T, bool>
                         or std::signed_integral<T>
                         or std::unsigned_integral<T>
                         or std::floating_point<T>
                         or std::same_as<T, std::string>;

template <typename T>
concept valid_option_optional = refl::is_optional<T>
                            and not refl::is_optional<typename T::value_type>
                            and valid_option_primitive<typename T::value_type>;

template <typename T>
concept valid_option_type = valid_option_optional<T> or valid_option_primitive<T>;


/**
 * @brief A class representing a CLI argument for the program
 *
 * The first template argument is the option type. It must be either an integer, a floating point
 * number, a string, a bool or an optional of one of the previous type.
 * Every template argument after the first one must be a string literal, and will be used to
 * determinate the short and the long name for the option, and eventually a description.
 * The rules regarding those literals are:
 * - a string starting with "--" will be treated as a long name
 * - a string starting with "-" will be treated as a short name
 * - a string not starting with a dash will be treated as a description
 * - only the first string for each category (long name, short name and description) will
 *   be considered
 * - if an option long name or short name is missing, it will be deduced from the member name
 * - you can prevent the name deduction using "-" for short names and "--" for long names.
 *   This will leave the class without a long/short name. You can't disable both long and short name
 * Example:
 * ```cpp
 * struct cli_args
 * {
 *     option<bool> verbose;  // -v, --verbose
 *     option<std::optional<bool>, "optional flag with description"> with_comment = false; // -w, --with-comment
 * 
 *     option<int, "both option names are deduced"> deduce_both; // -d, --deduce-both
 *     option<float, "-l", "short fixed, long deduced"> long_name;  // -l, --long-name
 *     option<std::string, "--short", "short deduced, long fixed"> short_name;  // -s, --short
 *     option<std::optional<int>, "-b", "--both", "both option names fixed"> set_both; // -b, --both
 *     option<std::optional<float>, "-", "--only-long", "disable short option"> only_long; // --only-long
 *     option<std::optional<std::string>, "--", "-o", "disable long option"> only_short;  // -o
 *     // option<int, "--", "-", "can't disable both long and short"> i_wont_compile;
 * };
 * ```
 * If `T` is an `optional<U>`, the option is not required and may have a default, which you can
 * assign as default value of the struct parameter:
 * ```cpp
 * struct cli_args
 * {
 *     option<std::optional<bool>> verbose = false;  // optional argument
 *     option<bool> log_to_stderr;  // required
 *     option<int> timeout = 999;  // default argument is ignored, since the option is required
 * };
 * ```
 *
 * @tparam T the type of the option
 * @tparam Args a list of strings representing short and long option names and the description
 * */
template <typename T, reflect::fixed_string ...Args>
    requires valid_option_type<T>
struct option
{
    using value_type = T;

    constexpr option() = default;
    template <typename U = T>
        requires std::constructible_from<T, U>
    constexpr option(U && u) : _value{std::forward<U>(u)} {}

    T _value;

    template <typename U> requires std::assignable_from<T, U>
    constexpr auto operator=(this auto & self, U && t) -> decltype(auto) {
        self._value = std::forward<U>(t);
        return self;
    }

    template <typename U>
    friend constexpr bool operator==(option<T, Args...> const & arg, U const & u)
    {
        return arg._value == u;
    }

    template <typename U, auto ...Args2>
    friend constexpr bool operator==(option<T, Args...> const & arg, option<U, Args2...> const & u)
    {
        return arg._value == u._value;
    }

    constexpr auto has_value() const requires refl::is_optional<T> { return _value.has_value(); }
    template <typename Self> requires refl::is_optional<T>
    constexpr auto value(this Self && self) -> decltype(auto) { return std::forward_like<Self>(self._value).value(); }
    template <typename Self> requires refl::is_optional<T>
    constexpr auto operator*(this Self && self) -> decltype(auto) { return *std::forward_like<Self>(self._value); }

    explicit(false) constexpr operator T const &() const & { return _value; }
    explicit(false) constexpr operator T() && { return std::move(_value); }

    static constexpr auto short_name_disabled = detail::short_name_disabled<Args...>();
    static constexpr auto long_name_disabled = detail::long_name_disabled<Args...>();
    static constexpr auto has_short_name = not short_name_disabled and (detail::is_short_name<Args>() or ...);
    static constexpr auto has_long_name = not long_name_disabled and (detail::is_long_name<Args>() or ...);
    static constexpr auto has_description = (detail::is_description<Args>() or ...);

    template <reflect::fixed_string Str, reflect::fixed_string ...Strs>
    static consteval auto short_name_impl() {
        if constexpr (detail::is_short_name<Str>()) {
            return Str;
        } else {
            return short_name_impl<Strs...>();
        }
    }

    static consteval auto short_name() requires has_short_name
    { return short_name_impl<Args...>(); }

    template <reflect::fixed_string Str, reflect::fixed_string ...Strs>
    static consteval auto long_name_impl() {
        if constexpr (detail::is_long_name<Str>()) {
            return Str;
        } else {
            return long_name_impl<Strs...>();
        }
    }

    static consteval auto long_name() requires has_long_name
    { return reflect::fixed_string{long_name_impl<Args...>()}; }

    template <reflect::fixed_string Str, reflect::fixed_string ...Strs>
    static consteval auto description_impl() {
        if constexpr (detail::is_description<Str>()) {
            return Str;
        } else {
            return description_impl<Strs...>();
        }
    }

    static consteval auto description() {
        if constexpr (not has_description) {
            return reflect::fixed_string{""};
        } else {
            return description_impl<Args...>();
        }
    }

    static_assert(not short_name_disabled or not long_name_disabled, "You can't disable both names in an option");
};

template <typename>
constexpr inline auto is_option = false;
template <template <typename, auto...> class C, class T, auto ...Args>
constexpr inline auto is_option<C<T, Args...>> = std::same_as<C<T, Args...>, option<T, Args...>>;

template <typename T, reflect::fixed_string ...Args>
struct format_arg
{
    option<T, Args...> const * content;

    format_arg(option<T, Args...> const & arg) : content(std::addressof(arg)) {};
};

template <typename T, reflect::fixed_string ...Args>
[[nodiscard]]
constexpr auto as_cli_arg(option<T, Args...> const & arg)
{
    return format_arg{arg};
}

// Long names
template <typename Aggregate, std::integral_constant I>
static consteval auto deduced_long_name()
{
    constexpr auto name = reflect::member_name<I, Aggregate>();
    using char_t = std::remove_cvref_t<decltype(name[0])>;
    constexpr auto fixed = reflect::fixed_string<char_t, std::size(name)>{std::data(name)};
    return refl::concat("--", refl::to_kebab_case(fixed));
}

template <typename Aggregate, std::integral_constant I>
static consteval auto long_name_unconditional()
{
    using member_type = refl::member_type<Aggregate, I>;
    if constexpr (member_type::has_long_name) {
        return member_type::long_name();
    } else {
        return deduced_long_name<Aggregate, I>();
    }
}

template <typename Aggregate, std::integral_constant I>
static consteval auto long_name() -> std::optional<std::string_view>
{
    using member_type = refl::member_type<Aggregate, I>;
    constexpr static auto result = [] {
        if constexpr (member_type::long_name_disabled) {
            return std::nullopt;
        } else {
            return long_name_unconditional<Aggregate, I>();
        }
    }();
    return result;
}


// Short names
template <typename Aggregate, std::integral_constant I>
static consteval auto deduced_short_name()
{
    return refl::concat("-", refl::to_kebab_case(refl::as_fixed_string(reflect::member_name<I, Aggregate>()[0])));
}

template <typename Aggregate, std::integral_constant I>
static consteval auto short_name_unconditional()
{
    using member_type = refl::member_type<Aggregate, I>;
    if constexpr (member_type::has_short_name) {
        return member_type::short_name();
    } else {
        return deduced_short_name<Aggregate, I>();
    }
}

template <typename Aggregate, std::integral_constant I>
static consteval auto short_name() -> std::optional<std::string_view>
{
    using member_type = refl::member_type<Aggregate, I>;
    constexpr static auto result = [] {
        if constexpr (member_type::short_name_disabled) {
            return std::nullopt;
        } else {
            return short_name_unconditional<Aggregate, I>();
        }
    }();
    return result;
}

namespace detail {
template <typename Cli>
consteval auto members_are_cli_args_impl()
{
    return []<std::size_t ...I>(std::index_sequence<I...>) {
        return (is_option<cliar::refl::member_type<Cli, I>> and ...);
    }(std::make_index_sequence<reflect::size<Cli>()>());
}
}  // namespace detail
template <typename Cli>
concept members_are_cli_args = detail::members_are_cli_args_impl<Cli>();

// Exceptions
class missing_required_option : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};

class wrong_option_type : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};

class unknown_option : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};

class repeated_option : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};


// Functions
/**
 * @brief Check wether the Cli template argument contains duplicate option names
 *
 * @tparam Cli The aggregate type to check
 * @return `true` if any duplicate is found, `false` otherwise
 */
template <typename Cli>
consteval auto has_repeated_option_names()
{
    auto names = std::vector<std::string>{};

    constexpr auto size = reflect::size<Cli>();
    auto fn = []<auto N>(this auto const & self, refl::consteval_value<N>) {
        using member_type = refl::member_type<Cli, N>;
        constexpr auto I = std::integral_constant<std::size_t, N>{};
        auto ns = std::vector<std::string>{};
        if constexpr (not member_type::long_name_disabled) {
            ns.push_back(static_cast<std::string>(static_cast<std::string_view>(
                cliar::long_name_unconditional<Cli, I>()
            )));
        }
        if constexpr (not member_type::short_name_disabled) {
            ns.push_back(static_cast<std::string>(static_cast<std::string_view>(
                cliar::short_name_unconditional<Cli, I>()
            )));
        }
        if constexpr (N + 1 < size) {
            auto next = self(refl::consteval_value<N + 1>{});
            ns.reserve(ns.size() + next.size());
            for (auto opt : next) {
                ns.emplace_back(std::move(opt));
            }
        }
        return ns;
    };
    // };
    auto result = fn(refl::consteval_value<0>{});
    std::sort(result.begin(), result.end());

    return std::ranges::adjacent_find(result) != result.end();
}


template <typename Cli>
consteval void check_repeated_names()
{
    static_assert(
        not has_repeated_option_names<Cli>(), "\n"
        " ############################################################################################\n"
        " #                 The Cli class must not contain repeated option names!                    #\n"
        " #       Remember that unspecified short and option names are deduced as follow:            #\n"
      R"( #        - `option<T, "-s", "--mount"> mount_fs;`     -> '-s', '--mount'                    #)""\n"
        " #        - `option<V> mount;`                         -> '-m', '--mount'                    #\n"
        " #        - `option<U> select_device;`                 -> '-s', '--select-device'            #\n"
        " #    Be sure to differentiate fixed and deduced names, and disable unwanted option names   #\n"
      R"( #                   passing "-" / "--" as non-type template parameters.                    #)""\n"
        " ############################################################################################"
    );
}

// Functions
/**
 * @brief Generates a description for the required struct
 *
 * @tparam Cli The class representing the CLI arguments
 * @param program_name A name for your program
 * @param additional_comment An optional comment to put after the auto-generated help
 * @return a string containing the help for the program
 */
template <typename Cli>
    requires std::is_aggregate_v<Cli> and members_are_cli_args<Cli>
auto help(std::string_view program_name, std::string_view additional_comment)
    -> std::string
{
    check_repeated_names<Cli>();
    auto flags = std::vector<std::string>{};
    auto options = std::vector<std::string>{};
    reflect::for_each<Cli>([&flags, &options](auto I) {
        using member_type = refl::member_type<Cli, I>;
        using value_t = member_type::value_type;
        constexpr auto long_name = cliar::long_name<Cli, I>();
        constexpr auto short_name = cliar::short_name<Cli, I>();
        constexpr auto description = member_type::description();
        constexpr auto has_short = short_name.has_value();
        constexpr auto has_long = long_name.has_value();
        constexpr auto first = std::string_view{has_short ? *short_name : long_name.value_or("")};
        constexpr auto separator = (has_short and has_long) ? ", " : "";
        constexpr auto second = std::string_view{(has_short and has_long) ? *long_name : ""};

        auto default_value = [I] {
            if constexpr (refl::has_default_value<Cli, I>()) {
                try {
                    return fmt::format(" (default: {})", refl::get_default_value<Cli, I>());
                } catch (...) {
                    return std::string{};
                }
            } else {
                return "";
            }
        }();

        if constexpr (std::same_as<value_t, bool> or std::same_as<value_t, std::optional<bool>>) {
            auto names = fmt::format("{}{}{}", first, separator, second);
            flags.push_back(fmt::format("    {:<50}{}{}", names, std::string_view{description}, default_value));
        } else {
            auto names = fmt::format("{}{}{}:", first, separator, second);
            auto with_type = fmt::format("{:<25} {}", names, refl::type_name<value_t>());
            options.push_back(fmt::format("    {:<50}{}{}", with_type, std::string_view{description}, default_value));
        }
    });
    constexpr auto nothing = std::string_view{};
    auto flags_tag = flags.empty() ? std::string_view{"[flags] "} : nothing;
    auto options_tag = options.empty() ? std::string_view{"[options] "} : nothing;

    auto flags_marker = flags.empty() ? nothing : std::string_view{"\n\nFLAGS:\n"};
    auto options_marker = options.empty() ? nothing : std::string_view{"\n\nOPTIONS:\n"};

    auto comment_newline = additional_comment.empty() ? std::string_view{} : std::string_view{"\n\n"};

    return fmt::format("Usage: {} {} {}{}{}{}{}{}{}\n",
                       program_name,
                       flags_tag, options_tag,
                       flags_marker,
                       fmt::join(flags, "\n"),
                       options_marker,
                       fmt::join(options, "\n"),
                       comment_newline,
                       additional_comment
                       );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
auto parse_arg(std::string_view rng) -> std::optional<T>
{
    if constexpr (std::is_same_v<T, bool>) {
        constexpr auto true_ = std::string_view{"true"};
        constexpr auto false_ = std::string_view{"false"};

        if (std::ranges::equal(rng, true_)) { return true; }
        if (std::ranges::equal(rng, false_)) { return false; }
        return std::nullopt;
    } else if constexpr (std::is_arithmetic_v<T>) {
        auto result = T{};
        auto [ptr, ec] = std::from_chars(rng.data(), rng.data() + rng.size(), result);
        if (ec == std::errc() and ptr == rng.data() + rng.size()) {
            return result;
        }
        return std::nullopt;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return rng | std::ranges::to<std::string>();
    } else if constexpr (refl::is_optional<T>) {
        return parse_arg<typename T::value_type>(rng);
    } else {
        static_assert(false, "You can only convert numbers, booleans and strings");
    }
}


template <typename T, typename It>
auto parse_option(It it, auto short_name, auto long_name) -> std::pair<It, T>
{
    auto parse_value = [short_name, long_name, &it](auto match) -> std::string_view {
        auto same = [](std::string_view a, std::string_view b) {
            return a == b;
        };

        if (same(match, short_name) or same(match, long_name)) {
            ++it;
            auto res = std::string_view{*it};
            constexpr auto is_arithmetic = [] {
                if constexpr (std::is_arithmetic_v<T>) {
                    return true;
                } else if constexpr (not refl::is_optional<T>) {
                    return false;
                } else {
                    return std::is_arithmetic_v<typename T::value_type>;
                }
            }();
            if constexpr (is_arithmetic) {
                if (res.starts_with("--")) {
                    throw wrong_option_type(fmt::format(
                        "Expected argument of type {}, got option '{}'", refl::type_name<T>(), res
                    ));
                }
            } else {
                if (res.starts_with("-")) {
                    throw wrong_option_type(fmt::format(
                        "Expected argument of type {}, got option '{}'", refl::type_name<T>(), res
                    ));
                }
            }
            return res;
        } else {
            auto split = std::views::split(std::string_view{*it}, std::string_view{"="})
                | std::ranges::to<std::vector<std::string>>();
            auto str = std::string_view{*it};
            auto idx = str.find('=');
            if (idx == std::string_view::npos) {
                throw unknown_option(fmt::format(
                    "Unknown option '{}' (maybe you meant {} {}?)", str, short_name, long_name
                ));
            }
            return str.substr(idx + 1);
        }
    };

    auto _value = parse_value(*it);
    auto result = parse_arg<T>(_value);
    if (not result.has_value()) {
        throw wrong_option_type(
            fmt::format("Option {} expects type {}, got {}", *it, refl::type_name<T>(), _value)
        );
    }
    return std::pair<It, T>{std::move(it), std::move(*result)};
}

/**
 * @brief Parses the argument list to get the desired struct
 *
 * @tparam Cli the aggregate representing a struct
 * @param args the command line arguments
 * @return an object of type `Cli` filled with the data parsed from `args`
 */
template <typename Cli>
    requires std::is_aggregate_v<Cli> and members_are_cli_args<Cli>
auto parse(std::span<char const * const> const cli_args)
{
    using buffer_t = refl::to_tuple_of_optionals<Cli>;
    auto buffer = buffer_t{};
 
    reflect::for_each<Cli>([cli_args, &buffer](auto I) {
        using member_type = refl::member_type<Cli, I>;
        constexpr auto long_name = cliar::long_name<Cli, I>().value_or("");
        constexpr auto short_name = cliar::short_name<Cli, I>().value_or("");

        auto it = std::ranges::find_if(cli_args, [=](char const * arg) {
            auto str = std::string_view{arg};
            if (not member_type::short_name_disabled) {
                if (str.starts_with(short_name)) {
                    if (str.size() == short_name.size()) {
                        return true;
                    }
                    if (str.at(short_name.size()) == '=') {
                        return true;
                    }
                }
            }
            auto is_long =  member_type::long_name_disabled ? false : str.starts_with(long_name);
            return is_long;
        });
        if (it != cli_args.end()) {
            using value_type = member_type::value_type;
            constexpr auto is_opt_flag = std::is_same_v<value_type, bool>
                                  or std::is_same_v<value_type, std::optional<bool>>;
            if constexpr (is_opt_flag) {
                auto const next_is_option = it + 1 != cli_args.end() 
                                        and std::string_view{*(it + 1)}.starts_with("-");
                if (next_is_option) {
                    std::get<std::optional<member_type>>(buffer) = true;
                    return;
                }
            }
            auto [new_it, res] = parse_option<value_type>(it, short_name, long_name);
            std::get<std::optional<member_type>>(buffer) = std::move(res);
            it = std::move(new_it);
        }
    });

    auto missing = std::vector<std::string_view>{};
    [&buffer, &missing]<auto ...I>(std::index_sequence<I...>) {
        auto fn = [&, cli=Cli{}]<typename Idx>(Idx) {
            constexpr auto N = std::integral_constant<std::size_t, Idx::value>{};
            if (auto const & v = std::get<N>(buffer); not v.has_value()) {
                using Tp = std::tuple_element_t<N, buffer_t>;
                using InnerTp = typename Tp::value_type::value_type;
                if constexpr (refl::is_optional<InnerTp>) {
                    if constexpr (refl::has_default_value<Cli, N>()) {
                        auto & default_value = reflect::get<N>(cli);
                        std::get<N>(buffer) = std::move(default_value);
                    } else {
                        std::get<N>(buffer) = InnerTp{};
                    }
                } else {
                    if constexpr (not refl::member_type<Cli, N>::long_name_disabled) {
                        auto ln = cliar::long_name_unconditional<Cli, N>();
                        missing.push_back(ln);
                    } else if constexpr (not refl::member_type<Cli, N>::short_name_disabled) {
                        auto sn = cliar::short_name_unconditional<Cli, N>();
                        missing.push_back(sn);
                    } else {
                        missing.push_back(reflect::member_name<N, Cli>());
                    }
                }
            }
        };
        (fn(refl::consteval_value<I>{}), ...);

    }(std::make_index_sequence<std::tuple_size_v<buffer_t>>());

    if (not missing.empty()) {
        throw missing_required_option(
            fmt::format("Required arguments are missing: {}\n", fmt::join(missing, ", "))
        );
    }

    return std::apply([]<typename ...Args>(Args &&... args) static {
        return Cli{std::forward<Args>(args).value()...};
    }, std::move(buffer));
}

} // namespace cliar

// Formatters
template <typename T, std::size_t N>
struct fmt::formatter<reflect::fixed_string<T, N>> : fmt::formatter<std::string_view>
{
    constexpr auto format(reflect::fixed_string<T, N> const & str, auto & ctx) const
    {
        return fmt::formatter<std::string_view>::format(std::string_view{str}, ctx);
    }
};

template <typename T, auto ...Args>
struct fmt::formatter<cliar::option<T, Args...>> : public fmt::formatter<T>
{
    constexpr auto format(cliar::option<T, Args...> const & arg, format_context & ctx) const
    {
        return fmt::formatter<T>::format(arg._value, ctx);
    }
};

template <typename T, auto ...Args>
struct fmt::formatter<cliar::format_arg<T, Args...>> : public fmt::formatter<T>
{
    using option = cliar::option<T, Args...>;
    constexpr auto format(cliar::format_arg<T, Args...> const & arg, auto & ctx) const
    {
        static constexpr auto empty = reflect::fixed_string{""};
        constexpr auto has_long = arg.has_long_name;
        constexpr auto has_short = arg.has_short_name;
        constexpr auto short_name = []() {
            if constexpr (has_short) { return option::short_name(); }
            else { return empty; }
        }();
        constexpr auto long_name = []() {
            if constexpr (has_long) { return option::long_name(); }
            else { return empty; }
        }();
        constexpr auto first = cliar::refl::conditional_v<has_short, short_name, long_name>;
        constexpr auto separator = (has_short and has_long) ? ", " : "";
        constexpr auto second = cliar::refl::conditional_v<has_short and has_long, long_name, empty>;
        return fmt::format_to(ctx.out(), "{}{}{}: {}", first, separator, second, arg._value);
    }
};

#endif /* CLIAR_HPP */
