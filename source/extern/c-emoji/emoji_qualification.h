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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* emoji_qualification_status_t: UTS #51 ED-18/ED-18a/ED-19 qualification
 * for a classified emoji sequence.
 *
 * Every function in this header takes an emoji_span_t by value.  The
 * span's type/style must have been resolved from a DFA classification
 * of exactly the codepoints the span views — e.g. an emoji_scan_*()
 * range converted via emoji_span_from_range(), or emoji_span_from_*()
 * fed a real DFA bitmask.  A span whose type/style don't match its
 * codepoints is undefined behavior: nothing here re-classifies.
 */
#ifndef EMOJI_QUALIFICATION_H
#define EMOJI_QUALIFICATION_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "emoji_types.h"
#include "emoji_span.h"
#include "emoji_ucd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EMOJI_QUALIFICATION_UNQUALIFIED,
  EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED,
  EMOJI_QUALIFICATION_FULLY_QUALIFIED
} emoji_qualification_status_t;

/* Resolves an EMOJI_SEQUENCE_ZWJ span.  Only span.src/span.len are read.
 * The span must be a structurally valid ZWJ chain from a real DFA
 * classification - undefined behavior otherwise. */
static inline emoji_qualification_status_t
emoji_qualification_resolve_zwj(emoji_span_t span) {
  bool any_missing = false, first_missing = false;
  size_t i = 0;

  while (i < span.len) {
    size_t base = i++;
    bool has_modifier = false, has_vs16 = false;

    if (i < span.len && emoji_ucd_is_modifier(span.src[i])) {
      has_modifier = true;
      i++;
    }

    if (i < span.len && emoji_ucd_is_emoji_presentation_selector(span.src[i])) {
      has_vs16 = true;
      i++;
    }

    bool missing = !has_vs16 &&
                   !has_modifier &&
                   !emoji_ucd_is_presentation(span.src[base]);
    any_missing |= missing;

    if (base == 0)
      first_missing = missing;

    if (i < span.len && emoji_ucd_is_zwj(span.src[i]))
      i++;
  }

  if (!any_missing)
    return EMOJI_QUALIFICATION_FULLY_QUALIFIED;
  if (first_missing)
    return EMOJI_QUALIFICATION_UNQUALIFIED;
  return EMOJI_QUALIFICATION_MINIMALLY_QUALIFIED;
}

/* Resolves a classified span.  BASIC/KEYCAP inspect the base codepoint,
 * ZWJ sequences walk every element; FLAG/TAG/MODIFIER are decided by
 * span.type alone. */
static inline emoji_qualification_status_t
emoji_qualification_resolve(emoji_span_t span) {
  assert(span.src != NULL);
  assert(span.len > 0);

  switch (span.type) {
    case EMOJI_SEQUENCE_ZWJ:
      return emoji_qualification_resolve_zwj(span);
    case EMOJI_SEQUENCE_BASIC:
    case EMOJI_SEQUENCE_KEYCAP:
      if (emoji_ucd_is_presentation(span.src[0]) || span.style == EMOJI_PRESENTATION_EMOJI)
        return EMOJI_QUALIFICATION_FULLY_QUALIFIED;
      return EMOJI_QUALIFICATION_UNQUALIFIED;
    default:
      // FLAG, TAG, MODIFIER: selectors never participate.
      return EMOJI_QUALIFICATION_FULLY_QUALIFIED;
  }
}

/* Rewrites span to its unqualified form per UTS #51 ED-19: strips all
 * variation selectors (VS-15/VS-16).  Returns the codepoints required;
 * if the return exceeds dst_capacity, dst holds a truncated copy. */
static inline size_t
emoji_qualification_rewrite_unqualified(emoji_span_t span,
                                        uint32_t* dst,
                                        size_t dst_capacity) {
  size_t n = 0;
  for (size_t i = 0; i < span.len; i++) {
    if (emoji_ucd_is_presentation_selector(span.src[i]))
      continue;
    if (n < dst_capacity)
      dst[n] = span.src[i];
    n++;
  }
  return n;
}

/* Rewrites span to its fully-qualified form per UTS #51 ED-18: strips
 * all selectors, then emits VS-16 after each element that requires it
 * (text-default bases and keycap bases, including ZWJ elements).
 * Modifier-absorbed positions get no selector.
 *
 * Returns the codepoints written, or the codepoints needed if
 * dst_capacity is insufficient (dst untouched past dst_capacity). */
static inline size_t
emoji_qualification_rewrite_fully(emoji_span_t span,
                                  uint32_t* dst,
                                  size_t dst_capacity) {
  if (span.type == EMOJI_SEQUENCE_FLAG ||
      span.type == EMOJI_SEQUENCE_TAG) {
    // FLAG/TAG spans contain no selectors; copy through verbatim.
    for (size_t i = 0; i < span.len && i < dst_capacity; i++)
      dst[i] = span.src[i];
    return span.len;
  }

  size_t n = 0;
  for (size_t i = 0; i < span.len; i++) {
    uint32_t cp = span.src[i];

    // Selectors are stripped; required VS-16 is re-emitted below.
    if (emoji_ucd_is_presentation_selector(cp))
      continue;

    if (n < dst_capacity)
      dst[n] = cp;
    n++;

    // ZWJ, modifiers, and keycap terminators never take a selector.
    if (emoji_ucd_is_zwj(cp) || emoji_ucd_is_modifier(cp) ||
        emoji_ucd_is_enclosing_keycap(cp))
      continue;

    // A base followed by a modifier takes no selector of its own.
    if (i + 1 < span.len && emoji_ucd_is_modifier(span.src[i + 1]))
      continue;

    if (!emoji_ucd_is_presentation(cp)) {
      if (n < dst_capacity)
        dst[n] = 0xFE0F; /* VARIATION SELECTOR-16 */
      n++;
    }
  }
  return n;
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_QUALIFICATION_H
