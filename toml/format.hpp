#ifndef TOML11_FORMAT
#define TOML11_FORMAT
#include "value.hpp"
#include <type_traits>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace toml
{

// synopsis
// toml::format("key", value, toml::make_inline(80))
// toml::format("key", value, toml::forceinline)
// std::cout << toml::make_inline(80) << value;
// std::cout << toml::forceinline << value;

template<typename traits = std::char_traits<toml::charactor>,
         typename alloc = std::allocator<toml::charactor>>
std::basic_string<toml::charactor, traits, alloc>
format(const value& v);

template<typename traits = std::char_traits<toml::charactor>,
         typename alloc = std::allocator<toml::charactor>>
std::basic_string<toml::charactor, traits, alloc>
format(const value& v, std::size_t mk);

template<typename traits = std::char_traits<toml::charactor>,
         typename alloc = std::allocator<toml::charactor>>
std::basic_string<toml::charactor, traits, alloc>
format(const toml::key& k, const value& v);

template<typename traits = std::char_traits<toml::charactor>,
         typename alloc = std::allocator<toml::charactor>>
std::basic_string<toml::charactor, traits, alloc>
format(const toml::key& k, const value& v, std::size_t mk);

template<value_t Type>
struct format_impl;

template<> struct format_impl<value_t::Boolean>
{
    typedef detail::toml_default_type<value_t::Boolean>::type type;

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        return val ? "true" : "false";
    }
};

template<> struct format_impl<value_t::Integer>
{
    typedef detail::toml_default_type<value_t::Integer>::type type;

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        return std::to_string(val);
    }
};

template<> struct format_impl<value_t::Float>
{
    typedef detail::toml_default_type<value_t::Float>::type type;

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        std::basic_ostringstream<toml::charactor> oss;
        oss << std::showpoint << val;
        if(oss.str().back() == '.') oss << '0';
        return oss.str();
    }
};

template<> struct format_impl<value_t::String>
{
    typedef detail::toml_default_type<value_t::String>::type type;

    std::size_t max_length;
    std::size_t indent_length;

    format_impl() : max_length(80), indent_length(0){}
    format_impl(std::size_t mx) : max_length(mx), indent_length(0){}
    format_impl(std::size_t mx, std::size_t idt)
        : max_length(mx), indent_length(std::min(mx, idt)){}

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        auto tmp = make_inline(val);
        if(max_length == std::numeric_limits<std::size_t>::max() ||
           tmp.size() <= max_length) return tmp;
        return convert_multiline(std::move(tmp));
    }

  private:

    std::basic_string<toml::charactor>
    make_inline(std::basic_string<toml::charactor>&& val)
    {
        std::basic_string<toml::charactor> str;
        str += '"';
        for(const auto& c : val)
        {
            if('\0' < c && c < '\31')
            {
                switch(c)
                {
                    case '\b': str += "\\b"; break;
                    case '\t': str += "\\t"; break;
                    case '\n': str += "\\n"; break;
                    case '\f': str += "\\f"; break;
                    case '\r': str += "\\r"; break;
                    default:
                    {
                        str += 'u';
                        std::basic_ostringstream<toml::charactor> oss;
                        oss << std::setw(4) << std::fill(0) << std::hex
                            << static_cast<std::int8_t>(c);
                        str += oss.str();
                        break;
                    }
                }
            }
            else if(c == '"')
            {
                str += "\\\"";
            }
            else if(c == '\\')
            {
                str += "\\\\";
            }
            else
            {
                str += c;
            }
        }
        str += '"';
        return str;
    }

    std::basic_string<toml::charactor>
    convert_multiline(std::basic_string<toml::charactor>&& val)
    {
        std::basic_string<toml::charactor> str; str.reserve(val.size() + 6);
        std::basic_string<toml::charactor> indent(' ', indent_length);
        str += "\"\"\"\n" + indent;
        std::size_t current = indent_length;
        for(auto iter = val.begin()+1; iter != val.end()-1; ++iter)
        {
            if(*iter != '\\')
            {
                if(current + 1 == max_length){str += "\\\n"; str += indent;}
                str += *iter; continue;
            }
            assert(std::next(iter) < val.end()-1);
            if(*std::next(iter) == 'u')
            {
                if(current + 5 == max_length){str += "\\\n"; str += indent;}
                assert(iter + 5 < val.end()-1);
                str += *iter; ++iter; // u
                str += *iter; ++iter; // 0
                str += *iter; ++iter; // 0
                str += *iter; ++iter; // 0
                str += *iter; continue;// 0
            }
            if(current + 2 == max_length){str += "\\\n"; str += indent;}
            str += *iter; ++iter; str += *iter;
        }
        str += "\"\"\"";
        return str;
    }

};

template<> struct format_impl<value_t::Datetime>
{
    typedef detail::toml_default_type<value_t::Datetime>::type type;

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        std::basic_ostringstream<toml::charactor> oss;
        oss << val;
        return oss.str();
    }
};

// TODO max length!
template<> struct format_impl<value_t::Array>
{
    typedef detail::toml_default_type<value_t::Array>::type type;

    std::size_t max_length;
    std::size_t indent_length;

    std::basic_string<toml::charactor>
    operator()(const type& val)
    {
        std::basic_string<toml::charactor> retval;
        retval += '[';
        for(const auto&& item : val)
        {
            auto tmp = format(val, max_length, indent_length);
            retval += tmp;
            retval += ", ";
            if(tmp.size() * 2 > max_length) retval += '\n';
        }
        retval += ']';
        return ;
    }
};

// TODO max length && inline!
template<> struct format_impl<value_t::Table>
{
    typedef detail::toml_default_type<value_t::Table>::type type;

    static std::basic_string<toml::charactor>
    invoke(const type& val)
    {
        std::basic_string<toml::charactor> retval;
        for(const auto&& item : val)
        {
            retval += val.first;
            retval += " = "
            retval += format(val.second);
            retval += '\n';
        }
        return ;
    }
};

template<typename traits, typename alloc>
std::basic_string<toml::charactor, traits, alloc>
format(const value& v)
{
    switch(v.type())
    {
        case value_t::Boolean : return format_impl<value_t::Boolean >::invoke(v.template cast<value_t::Boolean >());
        case value_t::Integer : return format_impl<value_t::Integer >::invoke(v.template cast<value_t::Integer >());
        case value_t::Float   : return format_impl<value_t::Float   >::invoke(v.template cast<value_t::Float   >());
        case value_t::String  : return format_impl<value_t::String  >::invoke(v.template cast<value_t::String  >());
        case value_t::Datetime: return format_impl<value_t::Datetime>::invoke(v.template cast<value_t::Datetime>());
        case value_t::Array   : return format_impl<value_t::Array   >::invoke(v.template cast<value_t::Array   >());
        case value_t::Table   : return format_impl<value_t::Table   >::invoke(v.template cast<value_t::Table   >());
        case value_t::Empty   : return format_impl<value_t::Empty   >::invoke(v.template cast<value_t::Empty   >());
        case value_t::Unknown : return format_impl<value_t::Unknown >::invoke(v.template cast<value_t::Unknown >());
        default throw std::logic_error("toml::format: unknown enum value");
    }
}

template<typename traits, typename alloc>
std::basic_string<toml::charactor, traits, alloc>
format(std::basic_string<toml::charactor, traits, alloc>&& key, const value& val)
{
    std::basic_string<charT, traits, alloc> retval(
            std::forward<std::basic_string<charT, traits, alloc>>(key));
    retval += " = ";
    retval += format(val);
    return retval;
}

// ----------------------------- stream operators -----------------------------

namespace detail
{

template<typename T = std::size_t>
struct inline_limit
{
    static_assert(std::is_same<T, std::size_t>::value, "do not instantiate this");
    static const int index;
    T limit;
    inline_limit() = default;
    ~inline_limit() = default;
    constexpr make_inline(T i): limit(i){}
    constexpr operator T() const {return limit;}

    static void callback(std::ios_base::event ev, std::ios_base& ios, int idx)
    {
        void*& info = ios.pword(idx);
        switch (ev)
        {
            case std::ios_base::erase_event:
            {
                delete static_cast<std::size_t*>(info);
                break;
            }
            case std::ios_base::copyfmt_event:
            {
                info = new std::size_t(*static_cast<std::size_t*>(info));
                break;
            }
            case std::ios_base::imbue_event:
            {
                break;
            }
        }
    }
};

template<typename T>
const int inline_limit<T>::index = std::ios_base::xalloc();

} //detail

template<typename sizeT, typename traits = std::char_traits<toml::charactor>>
std::basic_ostream<toml::charactor, traits>&
operator<<(std::basic_ostream<toml::charactor, traits>& os,
           const detail::inline_limit<sizeT>& inl)
{
    void*& info = os.pword(detail::inline_limit<sizeT>::index);
    if(!os.bad())
    {
        if(info == nullptr)
        {
            os.register_callback(detail::inline_limit<sizeT>::callback,
                                 detail::inline_limit<sizeT>::index);
            info = new std::size_t(inl.limit);
        }
        else
        {
            *static_cast<std::size_t*>(info) = inl.limit;
        }
    }
    return os;
}

constexpr static detail::inline_limit<std::size_t> forceinline =
    detail::inline_limit(std::numeric_limits<std::size_t>::max());

inline detail::inline_limit<std::size_t> make_inline(std::size_t sz)
{
    return detail::inline_limit<std::size_t>(sz);
}

template<typename T, typename traits = std::char_traits>
std::basic_ostream<toml::charactor, traits>&
operator<<(std::basic_ostream<toml::charactor, traits>& os,
           const toml::value& v)
{
    std::size_t* info =
        static_cast<std::size_t*>(os.pword(inline_limit<std::size_t>::index));
    return os << (info == nullptr ? toml::format(v) : toml::format(v, *info));
}

}
#endif // TOML11_FORMAT
