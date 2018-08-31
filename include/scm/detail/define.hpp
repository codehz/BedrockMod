//
// schmutz - Scheme Unterstüzung
//
// Copyright (C) 2017 Juan Pedro Bolivar Puente
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or at: http://boost.org/LICENSE_1_0.txt
//

#pragma once

#ifndef SCM_AUTO_EXPORT
#define SCM_AUTO_EXPORT 0
#endif

#include <scm/detail/subr_wrapper.hpp>
#include <scm/list.hpp>
#include <log.h>

namespace scm {
namespace detail {

template <typename Tag, typename Fn>
static void define_impl(const char * name, Fn fn)
{
    Log::trace("guile", "register %s", name);
    using args_t = function_args_t<Fn>;
    constexpr auto args_size = pack_size<args_t>::value;
    constexpr auto has_rest  = std::is_same<pack_last_t<args_t>, scm::args>{};
    constexpr auto arg_count = args_size - has_rest;
    auto subr = (scm_t_subr) +subr_wrapper_aux<Tag>(fn, args_t{});
    scm_c_define_gsubr(name, arg_count, 0, has_rest, subr);
#if SCM_AUTO_EXPORT
    scm_c_export(name);
#endif
}

} // namespace detail
} // namespace scm
