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

/* Unicode emoji sequence scanner.
 *
 * Scans an array of codepoints and returns the emoji sequences found.
 *
 * Two scanning modes are provided, each in a batch and a single-match form:
 *
 *   emoji_scan_strict()      - conservative.  Only emits a sequence when the
 *                              next codepoint cleanly terminates it.
 *   emoji_scan_next_strict() - same, but returns one emoji_span_t per call.
 *
 *   emoji_scan_greedy()      - permissive.  Emits the longest valid prefix
 *                              seen whenever a boundary is reached, even if
 *                              non-emoji codepoints followed the last valid
 *                              position.
 *   emoji_scan_next_greedy() - same, but returns one emoji_span_t per call.
 *
 * Incomplete sequences at end of input (e.g. a ZWJ with no following emoji,
 * or a tag sequence without a cancel tag) are dropped by the strict variants
 * and emitted up to the last accepting state by the greedy ones.
 *
 * Batch output is written to out[] as emoji_range_t values (see
 * emoji_range.h): half-open [start, end) index ranges into the codepoints
 * array.  If more sequences are found than max_out allows, the excess are
 * silently discarded.
 *
 * The next functions emit emoji_span_t values (see emoji_span.h): borrowed
 * {src, len} views into the caller's buffer.  *position is set to the resume
 * point — the caller continues scanning at codepoints[*position .. len), or
 * slices with codepoints + *position for the next call.  eof indicates the
 * input is complete; with eof=false a sequence still pending at end of input
 * is not emitted and *position is set to its start so the caller can supply
 * more data and retry.  The next functions are stateless: each call
 * re-scans the given slice from scratch.
 *
 * None of these functions perform semantic validation.  Sequences that are
 * structurally valid but have no defined rendering (e.g. two arbitrary emoji
 * joined by ZWJ with no RGI combination) are emitted without error.  Callers
 * requiring strict RGI conformance must validate emitted sequences against
 * the emoji-sequences.txt and emoji-zwj-sequences.txt data files.
 */
#ifndef EMOJI_SCAN_H
#define EMOJI_SCAN_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "emoji_dfa.h"
#include "emoji_ucd_classify.h"
#include "emoji_range.h"
#include "emoji_span.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline size_t emoji_scan_strict(const uint32_t* codepoints,
                                       size_t len,
                                       emoji_range_t* out,
                                       size_t max_out) {
  emoji_dfa_state_t state = EMOJI_DFA_STATE_START;
  size_t start = 0, count = 0;
  uint32_t recorded_bitmask = 0;

  for (size_t i = 0; i < len && count < max_out; i++) {
    emoji_dfa_class_t klass = emoji_ucd_classify(codepoints[i]);
    emoji_dfa_state_t next = emoji_dfa_step_record(state, klass, &recorded_bitmask);

    if (emoji_dfa_is_boundary(next)) {
      if (emoji_dfa_is_accepting(state))
        out[count++] = emoji_range_from_snapshot_bitmask(start,
                                                         i,
                                                         recorded_bitmask);
      recorded_bitmask = 0;
      next  = emoji_dfa_step_record(EMOJI_DFA_STATE_START, klass, &recorded_bitmask);
      start = i;
    }

    state = next;
    if (emoji_dfa_is_start(state))
      start = i + 1;
  }

  if (start < len && count < max_out && emoji_dfa_is_accepting(state)) {
    out[count++] = emoji_range_from_snapshot_bitmask(start,
                                                     len,
                                                     recorded_bitmask);
  }
  return count;
}

static inline size_t emoji_scan_greedy(const uint32_t* codepoints,
                                       size_t len,
                                       emoji_range_t* out,
                                       size_t max_out) {
  emoji_dfa_state_t state = EMOJI_DFA_STATE_START;
  size_t start = 0, end = 0, count = 0;
  bool has_accept = false;
  uint32_t recorded_bitmask = 0, accepted_bitmask = 0;

  for (size_t i = 0; i < len && count < max_out; i++) {
    emoji_dfa_class_t klass = emoji_ucd_classify(codepoints[i]);
    emoji_dfa_state_t next = emoji_dfa_step_record(state, klass, &recorded_bitmask);

    if (emoji_dfa_is_boundary(next)) {
      if (has_accept) {
        out[count++] = emoji_range_from_snapshot_bitmask(start,
                                                         end,
                                                         accepted_bitmask);
        has_accept = false;
      }
      recorded_bitmask = 0;
      next  = emoji_dfa_step_record(EMOJI_DFA_STATE_START, klass, &recorded_bitmask);
      start = i;
    }

    state = next;
    if (emoji_dfa_is_start(state))
      start = i + 1;

    if (emoji_dfa_is_accepting(state)) {
      accepted_bitmask = recorded_bitmask;
      has_accept = true;
      end = i + 1;
    }
  }

  if (has_accept && count < max_out)
    out[count++] = emoji_range_from_snapshot_bitmask(start,
                                                     end,
                                                     accepted_bitmask);
  return count;
}

static inline bool emoji_scan_next_strict(const uint32_t* codepoints,
                                          size_t len,
                                          emoji_span_t* out,
                                          size_t* position,
                                          bool eof) {
  emoji_dfa_state_t state = EMOJI_DFA_STATE_START;
  size_t start = 0;
  uint32_t recorded_bitmask = 0;

  for (size_t i = 0; i < len; i++) {
    emoji_dfa_class_t klass = emoji_ucd_classify(codepoints[i]);
    emoji_dfa_state_t next = emoji_dfa_step_record(state, klass, &recorded_bitmask);

    if (emoji_dfa_is_boundary(next)) {
      if (emoji_dfa_is_accepting(state)) {
        *out = emoji_span_from_snapshot_bitmask(codepoints + start,
                                                i - start,
                                                recorded_bitmask);
        *position = i;
        return true;
      }
      recorded_bitmask = 0;
      next  = emoji_dfa_step_record(EMOJI_DFA_STATE_START, klass, &recorded_bitmask);
      start = i;
    }

    state = next;
    if (emoji_dfa_is_start(state))
      start = i + 1;
  }

  if (eof && start < len && emoji_dfa_is_accepting(state)) {
    *out = emoji_span_from_snapshot_bitmask(codepoints + start,
                                            len - start,
                                            recorded_bitmask);
    *position = len;
    return true;
  }
  *position = (!eof && !emoji_dfa_is_start(state)) ? start : len;
  return false;
}

static inline bool emoji_scan_next_greedy(const uint32_t* codepoints,
                                          size_t len,
                                          emoji_span_t* out,
                                          size_t* position,
                                          bool eof) {
  emoji_dfa_state_t state = EMOJI_DFA_STATE_START;
  size_t start = 0, end = 0;
  bool has_accept = false;
  uint32_t recorded_bitmask = 0, accepted_bitmask = 0;

  for (size_t i = 0; i < len; i++) {
    emoji_dfa_class_t klass = emoji_ucd_classify(codepoints[i]);
    emoji_dfa_state_t next = emoji_dfa_step_record(state, klass, &recorded_bitmask);

    if (emoji_dfa_is_boundary(next)) {
      if (has_accept) {
        *out = emoji_span_from_snapshot_bitmask(codepoints + start,
                                                end - start,
                                                accepted_bitmask);
        *position = end;
        return true;
      }
      recorded_bitmask = 0;
      next  = emoji_dfa_step_record(EMOJI_DFA_STATE_START, klass, &recorded_bitmask);
      start = i;
    }

    state = next;
    if (emoji_dfa_is_start(state))
      start = i + 1;

    if (emoji_dfa_is_accepting(state)) {
      accepted_bitmask = recorded_bitmask;
      has_accept = true;
      end = i + 1;
    }
  }

  if (eof && has_accept) {
    *out = emoji_span_from_snapshot_bitmask(codepoints + start,
                                            end - start,
                                            accepted_bitmask);
    *position = end;
    return true;
  }
  *position = (!eof && !emoji_dfa_is_start(state)) ? start : len;
  return false;
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_SCAN_H
