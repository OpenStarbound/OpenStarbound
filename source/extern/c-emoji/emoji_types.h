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

/* Emoji types.
 *
 * emoji_sequence_type_t identifies the structural type of an sequence:
 *   BASIC    - a single emoji codepoint with an optional variation selector.
 *   KEYCAP   - a digit, # or * followed by an optional VS and U+20E3.
 *   FLAG     - a pair of regional indicator letters (e.g. 🇺🇸).
 *   TAG      - a base emoji followed by tag characters and a cancel tag
 *              (e.g. 🏴󠁧󠁢󠁥󠁮󠁧󠁿).
 *   MODIFIER - a modifier base followed by a Fitzpatrick skin tone modifier.
 *   ZWJ      - two or more emoji joined by U+200D zero width joiners.
 *
 * emoji_presentation_style_t indicates how the sequence should be rendered:
 *   UNSPECIFIED  - no variation selector present; consult
 *                  emoji_ucd_is_presentation().
 *   TEXT         - VS-15 (U+FE0E) was present; render as text.
 *   EMOJI        - VS-16 (U+FE0F) was present; render as emoji.
 */
#ifndef EMOJI_TYPES_H
#define EMOJI_TYPES_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EMOJI_SEQUENCE_BASIC,
  EMOJI_SEQUENCE_KEYCAP,
  EMOJI_SEQUENCE_FLAG,
  EMOJI_SEQUENCE_TAG,
  EMOJI_SEQUENCE_MODIFIER,
  EMOJI_SEQUENCE_ZWJ,
} emoji_sequence_type_t;

typedef enum {
  EMOJI_PRESENTATION_UNSPECIFIED,
  EMOJI_PRESENTATION_TEXT,
  EMOJI_PRESENTATION_EMOJI,
} emoji_presentation_style_t;

#ifdef __cplusplus
}
#endif
#endif // EMOJI_TYPES_H
