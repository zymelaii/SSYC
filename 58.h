#pragma once

#include <iostream>
#include <memory>
#include <string_view>
#include <assert.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 260
#endif

namespace slime {

class InputStreamTransformer {
public:
    void reset(std::istream *stream, std::string_view source);
    char get();

protected:
    void transform();

private:
    std::unique_ptr<std::istream> stream_;
    std::string                   buffer_;
    size_t                        cursor_;
    char                          source_[PATH_MAX];

    size_t lineMacro_;
    char   pathMacro_[PATH_MAX];
};

} // namespace slime