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

/* Presentation style resolution for emoji sequences.
 *
 * emoji_presentation_resolve() combines the variation-selector style with the
 * sequence type and the Emoji_Presentation property of the leading codepoint
 * to produce a fully resolved EMOJI or TEXT result — UNSPECIFIED is never
 * returned.
 *
 * emoji_presentation_resolve_unspecified() is the inner resolver for sequences
 * known to contain no variation selector. Called directly when the caller has
 * already established that style == EMOJI_PRESENTATION_UNSPECIFIED.
 */
#ifndef EMOJI_PRESENTATION_H
#define EMOJI_PRESENTATION_H
#include <stdint.h>
#include <assert.h>

#include "emoji_types.h"
#include "emoji_ucd.h"
#include "emoji_span.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline emoji_presentation_style_t
emoji_presentation_resolve_unspecified(emoji_sequence_type_t type,
                                       uint32_t              leading_codepoint) {
  // Consult sequence type and Emoji_Presentation property
  switch (type) {
    case EMOJI_SEQUENCE_BASIC:
      return emoji_ucd_is_presentation(leading_codepoint)
               ? EMOJI_PRESENTATION_EMOJI
               : EMOJI_PRESENTATION_TEXT;
    case EMOJI_SEQUENCE_KEYCAP:
      // 0-9, #, * have Emoji_Presentation=No
      return EMOJI_PRESENTATION_TEXT;
    case EMOJI_SEQUENCE_FLAG:
      // U+1F1E6..U+1F1FF REGIONAL INDICATOR A..Z have Emoji_Presentation=Yes
    case EMOJI_SEQUENCE_TAG:
      // U+1F3F4 WAVING BLACK FLAG has Emoji_Presentation=Yes
    case EMOJI_SEQUENCE_MODIFIER:
      // Modifier sequences have emoji presentation (UTS #51 s2.4)
    case EMOJI_SEQUENCE_ZWJ:
      // RGI ZWJ sequences render as emoji (emoji-zwj-sequences.txt)
      return EMOJI_PRESENTATION_EMOJI;
    default:
      // Unreachable: all defined sequence types are handled above
      return EMOJI_PRESENTATION_EMOJI;
  }
}

static inline emoji_presentation_style_t
emoji_presentation_resolve(emoji_sequence_type_t      type,
                           emoji_presentation_style_t style,
                           uint32_t                   leading_codepoint) {
  if (style != EMOJI_PRESENTATION_UNSPECIFIED)
    return style;
  return emoji_presentation_resolve_unspecified(type, leading_codepoint);
}

static inline emoji_presentation_style_t
emoji_presentation_resolve_span(emoji_span_t span) {
  assert(span.src != NULL && span.len > 0);
  return span.style == EMOJI_PRESENTATION_UNSPECIFIED
       ? emoji_presentation_resolve_unspecified(span.type, span.src[0])
       : span.style;
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_PRESENTATION_H
