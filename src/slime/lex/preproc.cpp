#include "preproc.h"

#include <string>
#include <string.h>

#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#define realpath(rel, abs) _fullpath((abs), (rel), PATH_MAX)
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <linux/limits.h>
#endif

namespace slime {

void InputStreamTransformer::reset(
    std::istream *stream, std::string_view source) {
    assert(stream != nullptr);
    stream_.reset(stream);
    buffer_.clear();
    cursor_    = 0;
    lineMacro_ = 0;
    if (source.empty()) {
        source_[0]    = '\0'; //<! input is from stdin
        pathMacro_[0] = '\0';
    } else {
        (void)realpath(source.data(), source_);
        std::string tmp(source_);
        auto        pos = tmp.find('\\', 0);
        while (pos != tmp.npos) {
            tmp.replace(pos, 1, "\\\\");
            pos = tmp.find('\\', pos + 2);
        }
        strcpy(pathMacro_, tmp.c_str());
    }
}

char InputStreamTransformer::get() {
    assert(stream_.get() != nullptr);
    if (cursor_ < buffer_.size()) { return buffer_[cursor_++]; }
    assert(cursor_ == buffer_.size());
    if (stream_->eof()) { return EOF; }
    std::getline(*stream_.get(), buffer_);
    ++lineMacro_;
    transform();
    cursor_ = 0;
    return buffer_[cursor_++];
}

void InputStreamTransformer::transform() {
    //! replace builtin macros
    if (buffer_.size() >= 12) {
#ifdef USE_LIBSLIME
        char replacement[32 + PATH_MAX]{};
        sprintf(
            replacement,
            "__slime_starttime(\"%s\", %zu)",
            pathMacro_,
            lineMacro_);
        auto pos = buffer_.find("starttime()");
        while (pos != buffer_.npos) {
            buffer_.replace(pos, sizeof("starttime()") - 1, replacement);
            pos = buffer_.find("starttime()");
        }
        sprintf(
            replacement,
            "__slime_stoptime(\"%s\", %zu)",
            pathMacro_,
            lineMacro_);
        pos = buffer_.find("stoptime()");
        while (pos != buffer_.npos) {
            buffer_.replace(pos, sizeof("stoptime()") - 1, replacement);
            pos = buffer_.find("stoptime()");
        }
#else
        char replacement[32]{};
        sprintf(replacement, "_sysy_starttime(%zu)", lineMacro_);
        auto pos = buffer_.find("starttime()");
        while (pos != buffer_.npos) {
            buffer_.replace(pos, sizeof("starttime()") - 1, replacement);
            pos = buffer_.find("starttime()");
        }
        sprintf(replacement, "_sysy_stoptime(%zu)", lineMacro_);
        pos = buffer_.find("stoptime()");
        while (pos != buffer_.npos) {
            buffer_.replace(pos, sizeof("stoptime()") - 1, replacement);
            pos = buffer_.find("stoptime()");
        }
#endif
    }

    //! append newline chars
#ifdef __WIN32__
    buffer_ += "\r\n";
#elif defined(__linux__)
    buffer_ += "\n";
#elif defined(__APPLE__)
    buffer_ += "\r";
#else
    buffer_ += "\n";
#endif
}

} // namespace slime
