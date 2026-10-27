/*
 * Copyright (c) 2026 Christian Hansen <chansen@cpan.org>
 * <https://github.com/chansen/c-emoji>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* ICU4C-backed implementation of the emoji_ucd.h Unicode Character Database
 * predicates, used when EMOJI_UCD_USE_ICU is defined before the first
 * include of emoji_ucd.h. See emoji_ucd_builtin.h for the default,
 * dependency-free implementation.
 *
 * emoji_ucd_is_emoji(), emoji_ucd_is_modifier_base(), and
 * emoji_ucd_is_presentation() delegate to ICU4C's u_hasBinaryProperty()
 * for the Emoji, Emoji_Modifier_Base, and Emoji_Presentation properties
 * respectively, instead of consulting a bundled trie.
 *
 * Requires linking against ICU4C's common library, e.g.:
 *   cc ... $(pkg-config --cflags --libs icu-uc)
 *
 * Note — Unicode version skew: results reflect the Unicode Character
 * Database bundled with whichever ICU4C is linked, not the Unicode
 * version this project targets and ships under unicode-data/. A linked
 * ICU4C older than the Unicode version this project targets will cause
 * failures in the data-driven conformance tests, on codepoints the older 
 * UCD hasn't yet assigned emoji properties to.
 *
 * Do not include this header directly — include emoji_ucd.h, which selects
 * between this and emoji_ucd_builtin.h.
 */
#ifndef EMOJI_UCD_ICU_H
#define EMOJI_UCD_ICU_H
#include <stdint.h>
#include <stdbool.h>

#include <unicode/uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool emoji_ucd_is_modifier_base(uint32_t cp) {
  return u_hasBinaryProperty((UChar32)cp, UCHAR_EMOJI_MODIFIER_BASE);
}

static inline bool emoji_ucd_is_emoji(uint32_t cp) {
  return u_hasBinaryProperty((UChar32)cp, UCHAR_EMOJI);
}

static inline bool emoji_ucd_is_presentation(uint32_t cp) {
  return u_hasBinaryProperty((UChar32)cp, UCHAR_EMOJI_PRESENTATION);
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_UCD_ICU_H
