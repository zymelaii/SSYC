#pragma once

#include "StreamProvider.h"

namespace slimec {

template <typename InStreamPtr>
class DefaultStreamProvider final
    : public StreamProvider<DefaultStreamProvider<InStreamPtr>> {
public:
    using base_type = StreamProvider<DefaultStreamProvider<InStreamPtr>>;
    using char_type = typename base_type::char_type;

    static_assert(std::is_same_v<char_type, char>);

    friend base_type;

public:
    DefaultStreamProvider(InStreamPtr &stream)
        : base_type(stream) {}

    DefaultStreamProvider(InStreamPtr &&stream)
        : base_type(stream) {}

    DefaultStreamProvider(DefaultStreamProvider &&other)
        : base_type(other) {}

    DefaultStreamProvider(DefaultStreamProvider &other)            = delete;
    DefaultStreamProvider &operator=(DefaultStreamProvider &other) = delete;

private:
    char_type get() {
        using self = base_type;
        if (self::tryReadNextByte() && self::currentByte() < 128) {
            return self::currentByte();
        }
        self::raiseError();
    }
};

} // namespace slimec
