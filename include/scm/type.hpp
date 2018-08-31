//
// schmutz - Scheme Unterstüzung
//
// Copyright (C) 2017 Juan Pedro Bolivar Puente
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or at: http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/detail/finalizer_wrapper.hpp>
#include <scm/detail/define.hpp>

#include <cassert>
#include <string>

namespace scm {
namespace detail {

template <typename T>
struct foreign_type_storage
{
    static SCM data;
};

template <typename T>
SCM foreign_type_storage<T>::data = SCM_UNSPECIFIED;

template <typename T, typename... Args>
val make_foreign(Args&& ...args)
{
    using storage_t = foreign_type_storage<T>;
    assert(storage_t::data != SCM_UNSPECIFIED &&
           "cannot create undefined type");
    return scm_make_foreign_object_1(
        storage_t::data,
        new (scm_gc_malloc(sizeof(T), "scmpp")) T(
            std::forward<Args>(args)...));
}

template <typename T, typename... Args>
T& get_foreign(val v)
{
    using storage_t = foreign_type_storage<T>;
    assert(storage_t::data != SCM_UNSPECIFIED &&
           "can not convert to undefined type");
    scm_assert_foreign_object_type(storage_t::data, v);
    return *(T*)scm_foreign_object_ref(v, 0);
}

template <typename T>
struct convert_foreign_type
{
    template <typename U>
    static SCM to_scm(U&& v) { return make_foreign<T>(std::forward<U>(v)); }
    static T&  to_cpp(SCM v) { return get_foreign<T>(v); }
};

// Assume that every other type is foreign
template <typename T>
struct convert<T,
               std::enable_if_t<!std::is_fundamental<T>::value &&
                                // only value types are supported at
                                // the moment but the story might
                                // change later...
                                !std::is_pointer<T>::value>>
    : convert_foreign_type<T>
{
};

template <typename Tag, typename T, int Seq=0>
struct type_definer : move_sequence
{
    using this_t = type_definer;
    using next_t = type_definer<Tag, T, Seq + 1>;

    const char* type_name_ = nullptr;
    scm_t_struct_finalize finalizer_ = nullptr;

    type_definer(type_definer&&) = default;

    type_definer(const char* type_name)
        : type_name_(type_name)
    {}

    ~type_definer()
    {
        if (!moved_from_) {
            using storage_t = detail::foreign_type_storage<T>;
            assert(storage_t::data == SCM_UNSPECIFIED);
            auto tl = strlen(type_name_);
            auto define_name = new char[tl + 3];
            define_name[0] = '<';
            strcpy(define_name + 1, type_name_);
            define_name[tl + 1] = '>';
            define_name[tl + 2] = 0;
            // auto define_name  = "<" + type_name_ + ">";
            auto foreign_type = scm_make_foreign_object_type(
                scm_from_utf8_symbol(define_name),
                scm_list_1(scm_from_utf8_symbol("data")),
                finalizer_);
            scm_c_define(define_name, foreign_type);
            storage_t::data = foreign_type;
#if SCM_AUTO_EXPORT
            scm_c_export(define_name);
#endif
            delete[] define_name;
        }
    }

    template <int Seq2, typename Enable=std::enable_if_t<Seq2 + 1 == Seq>>
    type_definer(type_definer<Tag, T, Seq2> r)
        : move_sequence{std::move(r)}
        , type_name_{std::move(r.type_name_)}
        , finalizer_{std::move(r.finalizer_)}
    {}

    /**
     * Define a Scheme procedure `([type-name])` that returns a Scheme
     * value holding a default constructed `T` instance.
     */
    next_t constructor() &&
    {
        define_impl<this_t>(type_name_, &make_foreign<T>);
        return { std::move(*this) };
    }

    /**
     * Define a Scheme procedure `([type-name] ...)` that returns a
     * Scheme value holding the result of invoking `fn(args...)`.
     */
    template <typename Fn>
    next_t constructor(Fn fn) &&
    {
        define_impl<this_t>(type_name_, fn);
        return { std::move(*this) };
    }

    /**
     * Define a Scheme procedure `(make-[type-name])` that returns a
     * Scheme value holding a default constructed `T` instance.
     */
    next_t maker() &&
    {
        auto tl = strlen(type_name_);
        auto buffer = new char[tl + 6];
        strcpy(buffer, "make-");
        strcpy(buffer + 5, type_name_);
        define_impl<this_t>(buffer, &make_foreign<T>);
        delete[] buffer;
        return { std::move(*this) };
    }

    /**
     * Define a Scheme procedure `(make-[type-name] ...)` that returns
     * the result of invoking `fn(...)`.
     */
    template <typename Fn>
    next_t maker(Fn fn) &&
    {
        auto tl = strlen(type_name_);
        auto buffer = new char[tl + 6];
        strcpy(buffer, "make-");
        strcpy(buffer + 5, type_name_);
        define_impl<this_t>(buffer, fn);
        delete[] buffer;
        return { std::move(*this) };
    }

    /**
     * Set the finalizer for Scheme wrapped values of `T` to invoke
     * the destructor of `T` on the wrapped value.
     */
    next_t finalizer() &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(
            [] (T& x) { x.~T(); });
        return { std::move(*this) };
    }

    /**
     * Set the finalizer of `T` to invoke `fn(*this)`.
     */
    template <typename Fn>
    next_t finalizer(Fn fn) &&
    {
        finalizer_ = (scm_t_struct_finalize) +finalizer_wrapper<Tag>(fn);
        return { std::move(*this) };
    }

    /**
     * Define a Scheme procedure `([type-name]-[name] ...)` that returns
     * the result of invoking `fn(...)`.
     */
    template <typename Fn>
    next_t define(const char *name, Fn fn) &&
    {
        auto gl = strlen(type_name_);
        auto buffer = new char[gl + strlen(name) + 2];
        strcpy(buffer, type_name_);
        buffer[gl] = '-';
        strcpy(buffer + gl + 1, name);
        define_impl<this_t>(buffer, fn);
        delete[] buffer;
        return { std::move(*this) };
    }
};

} // namespace detail

/**
 * Returns a new `scm::detail::type_definer` that registers a type
 * `T` with the given `type_name` and can be used to add related
 * definitions to the current module.
 */
template <typename T, typename Tag=T>
detail::type_definer<Tag, T> type(const char* type_name)
{
    return { type_name };
}

} // namespace scm
