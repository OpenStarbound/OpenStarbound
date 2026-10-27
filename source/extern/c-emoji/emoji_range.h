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

/* emoji_range_t: a half-open index range [start, end) — end is the index
 * of the first codepoint after the sequence, not the last codepoint in it
 * — tagged with its resolved sequence type and presentation style.
 */
#ifndef EMOJI_RANGE_H
#define EMOJI_RANGE_H
#include <stddef.h>
#include <stdint.h>

#include "emoji_types.h"
#include "emoji_dfa_classify.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t start;
  size_t end;
  emoji_sequence_type_t type;
  emoji_presentation_style_t style;
} emoji_range_t;

// From an already-resolved type/style.
static inline emoji_range_t
emoji_range_from_type_style(size_t start,
                            size_t end,
                            emoji_sequence_type_t type,
                            emoji_presentation_style_t style) {
  return (emoji_range_t){
    .start = start,
    .end   = end,
    .type  = type,
    .style = style
  };
}

// Resolves type/style from a recorded bitmask.
static inline emoji_range_t
emoji_range_from_snapshot_bitmask(size_t start,
                                  size_t end,
                                  uint32_t snapshot_bitmask) {
  return emoji_range_from_type_style(start,
                                     end,
                                     emoji_dfa_classify_type(snapshot_bitmask),
                                     emoji_dfa_classify_style(snapshot_bitmask));
}

// Number of codepoints in the range.
static inline size_t  
emoji_range_length(const emoji_range_t* r) {  
  return r->end - r->start;  
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_RANGE_H
