#pragma once

#include <stddef.h>

namespace slimec {

/*!
 * \brief encode unicode to utf-8 bytes
 *
 * \param [in] code unicode
 * \param [in, out] dest output buffer
 * \param [in] n size of buffer
 *
 * \return required size if dest is nullptr, otherwise size of encoded bytes
 */
size_t unicodeToUtf8(char32_t code, char *dest, size_t n);

} // namespace slimec
