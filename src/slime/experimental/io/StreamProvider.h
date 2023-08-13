#pragma once

#include <iostream>
#include <memory>
#include <type_traits>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>

namespace slime {

template <typename Provider, typename CharType = char>
class StreamProvider;

template <
    template <typename>
    typename Provider,
    typename InStreamPtr,
    typename CharType>
class StreamProvider<Provider<InStreamPtr>, CharType> {
private:
    template <typename T>
    struct is_unique_ptr_type : std::false_type {};

    template <typename... Args>
    struct is_unique_ptr_type<std::unique_ptr<Args...>> : std::true_type {};

public:
    static_assert(
        is_unique_ptr_type<InStreamPtr>::value,
        "std::unique_ptr<...> is expected for StreamProvider::pointer_type");

    using stream_type  = std::decay_t<typename InStreamPtr::element_type>;
    using deleter_type = typename InStreamPtr::deleter_type;
    using pointer_type = std::unique_ptr<stream_type, deleter_type>;
    using char_type    = std::decay_t<CharType>;

    static_assert(
        std::is_base_of_v<std::istream, stream_type>,
        "type derived from std::istream is expected for "
        "StreamProvider::stream_type");

    static_assert(
        std::is_same_v<char_type, char> || std::is_same_v<char_type, wchar_t>
            || std::is_same_v<char_type, char16_t>
            || std::is_same_v<char_type, char32_t>,
        "char_type is expected to be one of char, wchar_t, char16_t, char32_t");

    using provider_type = Provider<pointer_type>;
    using this_type     = StreamProvider<Provider<InStreamPtr>, CharType>;

    static_assert(std::is_same_v<Provider<InStreamPtr>, provider_type>);

public:
    StreamProvider(InStreamPtr &stream)
        : StreamProvider(std::move(stream)) {}

    StreamProvider(InStreamPtr &&stream)
        : stream_(stream.release())
        , charcode_{EOS}
        , cursor_{static_cast<char>(EOS)}
        , ready_{false}
        , err_{false}
        , errEnabled_{false} {
        next_ = stream_->get();
    }

    StreamProvider(StreamProvider &&other)
        : StreamProvider(std::move(other.stream_)) {}

    StreamProvider(StreamProvider &other)            = delete;
    StreamProvider &operator=(StreamProvider &other) = delete;

public:
    static inline constexpr char_type EOS = -1; //<! end of stream

    char_type get() {
        using callee_type = decltype(&provider_type::get);
        using value_type  = std::invoke_result_t<callee_type, provider_type *>;
        static_assert(std::is_same_v<value_type, char_type>);
        errEnabled_ = true;
        return !err() && !empty() && setjmp(errjmp_) == 0
                 ? (charcode_ = static_cast<provider_type *>(this)->get())
                 : (charcode_ = EOS);
    }

    char_type currentChar() const {
        return charcode_;
    }

    char currentByte() const {
        return cursor_;
    }

    char nextByte() const {
        return next_;
    }

    bool eos() const {
        return ready_ && cursor_ == EOS;
    }

    bool err() const {
        return err_;
    }

    bool empty() const {
        return next_ == EOS;
    }

    this_type &super() {
        return *this;
    }

protected:
    [[noreturn]] void raiseError() {
        err_ = true;
        if (!errEnabled_) { abort(); }
        errEnabled_ = false;
        longjmp(errjmp_, true);
    }

    bool tryReadNextByte() {
        cursor_ = next_;
        ready_  = true;
        next_   = stream_->get();
        return cursor_ != EOS;
    }

private:
    jmp_buf      errjmp_;
    pointer_type stream_;
    char_type    charcode_;
    char         cursor_;
    char         next_;
    bool         ready_;
    bool         err_;
    bool         errEnabled_;
};

} // namespace slime
