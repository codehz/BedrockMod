//
// schmutz - Scheme Unterstüzung
//
// Copyright (C) 2017 Juan Pedro Bolivar Puente
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or at: http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/define.hpp>
#include <string>

namespace scm {
namespace detail {

template <typename Tag, int Seq=0>
struct definer
{
    using this_t = definer;
    using next_t = definer<Tag, Seq + 1>;

    const char *group_name_ = nullptr;

    definer() = default;
    definer(definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    definer(definer<Tag, Seq2>)
    {}

    /**
     * Defines a Scheme procedure `(name ...)` that returns the result
     * of invoking `fn(...)`.
     */
    template <typename Fn>
    next_t define(const char* name, Fn fn) &&
    {
        define_impl<this_t>(name, fn);
        return { std::move(*this) };
    }
};

template <typename Tag, int Seq=0>
struct group_definer
{
    using this_t = group_definer;
    using next_t = group_definer<Tag, Seq + 1>;

    const char *group_name_ = nullptr;

    group_definer(const char *name)
        : group_name_{name} {}

    group_definer(group_definer&&) = default;

    template <int Seq2,
              typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    group_definer(group_definer<Tag, Seq2> other)
        : group_name_ {std::move(other.group_name_)}
    {}

    /**
     * Defines a Scheme procedure `([group-name]-name ...)` that returns
     * the result of invoking `fn(...)`.
     */
    template <typename Fn>
    next_t define(const char* name, Fn fn) &&
    {
        auto gl = strlen(group_name_);
        auto buffer = new char[gl + strlen(name) + 2];
        strcpy(buffer, group_name_);
        buffer[gl] = '-';
        strcpy(buffer + gl + 1, name);
        define_impl<this_t>(buffer, fn);
        delete[] buffer;
        return { std::move(*this) };
    }
};

} // namespace detail

/**
 * Returns a `scm::detail::definer` that can be used to add definitions
 * to the current module.
 */
template <typename Tag=void>
detail::definer<Tag> group()
{
    return {};
}

/**
 * Returns a named `scm::detail::group_definer` that can be used to add
 * definitions to the current module.
 */
template <typename Tag=void>
detail::group_definer<Tag> group(const char* group_name)
{
    return { group_name };
}

} // namespace scm
