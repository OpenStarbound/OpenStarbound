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

/* Unicode Character Database predicates for emoji property detection (UTS #51).
 *
 * emoji_ucd_is_emoji(), emoji_ucd_is_modifier_base(), and
 * emoji_ucd_is_presentation() test the Emoji, Emoji_Modifier_Base, and
 * Emoji_Presentation binary properties respectively.
 *
 * By default these are implemented by the bundled, dependency-free tries in
 * emoji_ucd_builtin.h. Define EMOJI_UCD_USE_ICU before the first include of
 * this header to use the ICU4C-backed implementation in emoji_ucd_icu.h
 * instead — see that file for what it requires and where it can disagree
 * with the builtin tables.
 *
 * Note that the Emoji property does not imply emoji presentation.  Digits,
 * # and * are Emoji but text-default; emoji_ucd_is_presentation() resolves
 * the ambiguity for codepoints where no variation selector was seen.
 */
#ifndef EMOJI_UCD_H
#define EMOJI_UCD_H
#include <stdint.h>
#include <stdbool.h>

#if defined(EMOJI_UCD_USE_ICU)
#  include "emoji_ucd_icu.h"
#else
#  include "emoji_ucd_builtin.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// U+0023 NUMBER SIGN, U+002A ASTERISK, U+0030..U+0039 DIGIT ONE..NINE,
static inline bool emoji_ucd_is_keycap_base(uint32_t cp) {
  return cp == 0x0023 || cp == 0x002A || (cp >= 0x0030 && cp <= 0x0039);
}

// U+20E3 COMBINING ENCLOSING KEYCAP
static inline bool emoji_ucd_is_enclosing_keycap(uint32_t cp) {
  return cp == 0x20E3;
}

// U+1F3F4 WAVING BLACK FLAG
static inline bool emoji_ucd_is_tag_base(uint32_t cp) {
  return cp == 0x1F3F4;
}

// U+E0020 TAG SPACE to U+E007E TAG TILDE
static inline bool emoji_ucd_is_tag_spec(uint32_t cp) {
  return cp >= 0xE0020 && cp <= 0xE007E;
}

// U+E007F CANCEL TAG
static inline bool emoji_ucd_is_tag_term(uint32_t cp) {
  return cp == 0xE007F;
}

// U+FE0E VARIATION SELECTOR-15
static inline bool emoji_ucd_is_text_presentation_selector(uint32_t cp) {
  return cp == 0xFE0E;
}

// U+FE0F VARIATION SELECTOR-16
static inline bool emoji_ucd_is_emoji_presentation_selector(uint32_t cp) {
  return cp == 0xFE0F;
}

// U+FE0E..U+FE0F VARIATION SELECTOR 15-16
static inline bool emoji_ucd_is_presentation_selector(uint32_t cp) {
  return cp == 0xFE0E || cp == 0xFE0F;
}

// REGIONAL INDICATOR SYMBOL LETTER A..Z
static inline bool emoji_ucd_is_regional_indicator(uint32_t cp) {
  return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

// EMOJI MODIFIER FITZPATRICK TYPE 1-6
static inline bool emoji_ucd_is_modifier(uint32_t cp) {
  return cp >= 0x1F3FB && cp <= 0x1F3FF;
}

// U+200D ZERO WIDTH JOINER
static inline bool emoji_ucd_is_zwj(uint32_t cp) {
  return cp == 0x200D;
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_UCD_H
