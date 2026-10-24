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

/* emoji_span_t: a borrowed view over an emoji sequence — src points at the  
 * first codepoint, len is the codepoint count — tagged with the sequence  
 * type and presentation style resolved by the DFA.  
 *  
 * The type and style must have been classified from a DFA snapshot of the  
 * spanned codepoints [src, src + len).  A span constructed from unrelated  
 * type/style or codepoints is meaningless and undefined.  
 *  
 * src borrows the underlying buffer: the span is only valid while the  
 * buffer remains resident and unmodified.  For a representation that  
 * survives buffer movement or reallocation, use emoji_range_t.  
 */
#ifndef EMOJI_SPAN_H
#define EMOJI_SPAN_H
#include <stddef.h>
#include <stdint.h>

#include "emoji_types.h"
#include "emoji_dfa_classify.h"
#include "emoji_range.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const uint32_t* src;
  size_t len;
  emoji_sequence_type_t type;
  emoji_presentation_style_t style;
} emoji_span_t;

// From an already-resolved type/style.
static inline emoji_span_t
emoji_span_from_type_style(const uint32_t* src,
                           size_t len,
                           emoji_sequence_type_t type,
                           emoji_presentation_style_t style) {
  return (emoji_span_t){
    .src   = src,
    .len   = len,
    .type  = type,
    .style = style
  };
}

// From a recorded DFA bitmask (type/style resolved from snapshot_bitmask).
static inline emoji_span_t
emoji_span_from_snapshot_bitmask(const uint32_t* src,
                                 size_t len,
                                 uint32_t snapshot_bitmask) {
  return emoji_span_from_type_style(src,
                                    len,
                                    emoji_dfa_classify_type(snapshot_bitmask),
                                    emoji_dfa_classify_style(snapshot_bitmask));
}

// From an index range.  buffer is the array base the range indexes into.
static inline emoji_span_t
emoji_span_from_range(const emoji_range_t* range,
                      const uint32_t* buffer) {
  return emoji_span_from_type_style(buffer + range->start,
                                    emoji_range_length(range),
                                    range->type,
                                    range->style);
}

#ifdef __cplusplus
}
#endif
#endif /* EMOJI_SPAN_H */
