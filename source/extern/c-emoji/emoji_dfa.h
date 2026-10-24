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

/* DFA core for Unicode emoji sequence recognition (UTS #51 v17.0.0).
 *
 * Every state belongs to exactly one of three categories:
 *
 *   Boundary   REJECT — the current attempt is over; the codepoint that
 *              caused it must be retried from START.
 *
 *   Accepting  TERMINAL .. RI — a complete sequence ends here but may
 *              still be extended by further input.
 *
 *   Pending    TAG_SPEC .. ZWJ — inside a valid prefix; no complete
 *              sequence yet.
 *
 * The enum values are ordered so that accepting states form a contiguous
 * range, enabling a single range check in emoji_dfa_is_accepting().
 * Do not reorder the enum without updating that function.
 *
 * <https://www.unicode.org/reports/tr51/tr51-29.html>
 */
#ifndef EMOJI_DFA_H
#define EMOJI_DFA_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  EMOJI_DFA_STATE_REJECT = 0,        // Boundary: no valid transition
  EMOJI_DFA_STATE_START,             // Idle: not inside any sequence

  // Accepting — a complete sequence ends here (may be extended)
  EMOJI_DFA_STATE_TERMINAL,          // Dead-end accept: keycap, flag pair, tag
  EMOJI_DFA_STATE_EMOJI,             // Bare emoji or ZWJ target
  EMOJI_DFA_STATE_EMOJI_ZWJ,         // ZWJ element (no VS-15)
  EMOJI_DFA_STATE_MODIFIER_BASE,     // Emoji that accepts a modifier
  EMOJI_DFA_STATE_MODIFIER_BASE_ZWJ, // ZWJ modifier base (no VS-15) 
  EMOJI_DFA_STATE_OPTIONAL_ZWJ,      // Accept + VS-16 or modifier applied
  EMOJI_DFA_STATE_KEYCAP_VS,         // Keycap base + variation selector
  EMOJI_DFA_STATE_TAG_BASE,          // U+1F3F4 (may open a tag sequence)
  EMOJI_DFA_STATE_RI,                // First regional indicator

  // Pending — inside a valid prefix, no complete sequence yet
  EMOJI_DFA_STATE_TAG_SPEC,          // Accumulating tag characters
  EMOJI_DFA_STATE_TAG_EMPTY,         // Cancel tag without any tag characters
  EMOJI_DFA_STATE_KEYCAP_BASE,       // Digit/*/# awaiting VS or keycap term
  EMOJI_DFA_STATE_ZWJ,               // ZWJ received, awaiting target emoji

  EMOJI_DFA_STATE_COUNT
} emoji_dfa_state_t;

typedef enum {
  EMOJI_DFA_CLASS_OTHER = 0,
  EMOJI_DFA_CLASS_EMOJI,              // Emoji property (catch-all)
  EMOJI_DFA_CLASS_MODIFIER_BASE,      // Emoji_Modifier_Base property
  EMOJI_DFA_CLASS_MODIFIER,           // Emoji_Modifier (Fitzpatrick)
  EMOJI_DFA_CLASS_VS15,               // U+FE0E text presentation
  EMOJI_DFA_CLASS_VS16,               // U+FE0F emoji presentation
  EMOJI_DFA_CLASS_RI,                 // Regional indicator A..Z
  EMOJI_DFA_CLASS_KEYCAP_BASE,        // 0-9, #, *
  EMOJI_DFA_CLASS_KEYCAP_TERM,        // U+20E3 combining enclosing keycap
  EMOJI_DFA_CLASS_TAG_BASE,           // U+1F3F4 waving black flag
  EMOJI_DFA_CLASS_TAG_SPEC,           // U+E0020..U+E007E tag characters
  EMOJI_DFA_CLASS_TAG_TERM,           // U+E007F cancel tag
  EMOJI_DFA_CLASS_ZWJ,                // U+200D zero width joiner
  EMOJI_DFA_CLASS_COUNT
} emoji_dfa_class_t;

/*
 * Bitmask layout for emoji_dfa_step_record():
 *
 *   bits  0..15  character classes seen during the current attempt
 *   bits 16..31  states visited during the current attempt
 *
 * Only meaningful transitions are recorded — codepoints that loop
 * on START or produce REJECT do not set bits.
 */
#define EMOJI_DFA_RECORD_STATE_SHIFT 16

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(EMOJI_DFA_STATE_REJECT == 0,
               "REJECT must be zero (implicit default in transition table)");
_Static_assert(EMOJI_DFA_STATE_COUNT <= EMOJI_DFA_RECORD_STATE_SHIFT,
               "too many states for bitmask packing");
_Static_assert(EMOJI_DFA_CLASS_COUNT <= EMOJI_DFA_RECORD_STATE_SHIFT,
               "too many classes for bitmask packing");
#endif

/* clang-format off */
static const uint8_t
emoji_dfa_table[EMOJI_DFA_STATE_COUNT][EMOJI_DFA_CLASS_COUNT] = {
  [EMOJI_DFA_STATE_START] = {
    [EMOJI_DFA_CLASS_OTHER]         = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_VS15]          = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_TAG_SPEC]      = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_TAG_TERM]      = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_KEYCAP_TERM]   = EMOJI_DFA_STATE_START,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_START,

    [EMOJI_DFA_CLASS_RI]            = EMOJI_DFA_STATE_RI,
    [EMOJI_DFA_CLASS_EMOJI]         = EMOJI_DFA_STATE_EMOJI,
    [EMOJI_DFA_CLASS_MODIFIER_BASE] = EMOJI_DFA_STATE_MODIFIER_BASE,
    [EMOJI_DFA_CLASS_MODIFIER]      = EMOJI_DFA_STATE_TERMINAL,
    [EMOJI_DFA_CLASS_KEYCAP_BASE]   = EMOJI_DFA_STATE_KEYCAP_BASE,
    [EMOJI_DFA_CLASS_TAG_BASE]      = EMOJI_DFA_STATE_TAG_BASE,
  },
  [EMOJI_DFA_STATE_KEYCAP_BASE] = {
    [EMOJI_DFA_CLASS_VS15]          = EMOJI_DFA_STATE_KEYCAP_VS,
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_KEYCAP_VS,
    [EMOJI_DFA_CLASS_KEYCAP_TERM]   = EMOJI_DFA_STATE_TERMINAL,
  },
  [EMOJI_DFA_STATE_KEYCAP_VS] = {
    [EMOJI_DFA_CLASS_KEYCAP_TERM]   = EMOJI_DFA_STATE_TERMINAL,
  },
  [EMOJI_DFA_STATE_RI] = {
    [EMOJI_DFA_CLASS_RI]            = EMOJI_DFA_STATE_TERMINAL,
  },
  [EMOJI_DFA_STATE_EMOJI] = {
    [EMOJI_DFA_CLASS_VS15]          = EMOJI_DFA_STATE_TERMINAL,
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
  },
  [EMOJI_DFA_STATE_EMOJI_ZWJ] = {
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
  },
  [EMOJI_DFA_STATE_MODIFIER_BASE] = {
    [EMOJI_DFA_CLASS_VS15]          = EMOJI_DFA_STATE_TERMINAL,
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
    [EMOJI_DFA_CLASS_MODIFIER]      = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
  },
  [EMOJI_DFA_STATE_MODIFIER_BASE_ZWJ] = {
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
    [EMOJI_DFA_CLASS_MODIFIER]      = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
  },
  [EMOJI_DFA_STATE_TAG_BASE] = {
    [EMOJI_DFA_CLASS_VS15]          = EMOJI_DFA_STATE_TERMINAL,
    [EMOJI_DFA_CLASS_VS16]          = EMOJI_DFA_STATE_OPTIONAL_ZWJ,
    [EMOJI_DFA_CLASS_TAG_SPEC]      = EMOJI_DFA_STATE_TAG_SPEC,
    [EMOJI_DFA_CLASS_TAG_TERM]      = EMOJI_DFA_STATE_TAG_EMPTY,
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
  },
  [EMOJI_DFA_STATE_TAG_SPEC] = {
    [EMOJI_DFA_CLASS_TAG_SPEC]      = EMOJI_DFA_STATE_TAG_SPEC,
    [EMOJI_DFA_CLASS_TAG_TERM]      = EMOJI_DFA_STATE_TERMINAL,
  },
  [EMOJI_DFA_STATE_OPTIONAL_ZWJ] = {
    [EMOJI_DFA_CLASS_ZWJ]           = EMOJI_DFA_STATE_ZWJ,
  },
  [EMOJI_DFA_STATE_ZWJ] = {
    [EMOJI_DFA_CLASS_EMOJI]         = EMOJI_DFA_STATE_EMOJI_ZWJ,
    [EMOJI_DFA_CLASS_MODIFIER_BASE] = EMOJI_DFA_STATE_MODIFIER_BASE_ZWJ,
  }
};
/* clang-format on */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(EMOJI_DFA_STATE_RI == EMOJI_DFA_STATE_TERMINAL + 6,
               "accepting states must be contiguous from TERMINAL to RI");
#endif

static inline bool emoji_dfa_is_accepting(emoji_dfa_state_t state) {
  return (unsigned)(state - EMOJI_DFA_STATE_TERMINAL) <=
         (unsigned)(EMOJI_DFA_STATE_RI - EMOJI_DFA_STATE_TERMINAL);
}

static inline bool emoji_dfa_is_boundary(emoji_dfa_state_t state) {
  return state == EMOJI_DFA_STATE_REJECT;
}

static inline bool emoji_dfa_is_start(emoji_dfa_state_t state) {
  return state == EMOJI_DFA_STATE_START;
}

static inline emoji_dfa_state_t emoji_dfa_step(emoji_dfa_state_t state,
                                               emoji_dfa_class_t klass) {
  return (emoji_dfa_state_t)emoji_dfa_table[state][klass];
}

static inline emoji_dfa_state_t emoji_dfa_step_record(emoji_dfa_state_t state,
                                                      emoji_dfa_class_t klass,
                                                      uint32_t* recorded_bitmask) {
  state = emoji_dfa_step(state, klass);
  if (state != EMOJI_DFA_STATE_START && state != EMOJI_DFA_STATE_REJECT)
    *recorded_bitmask |= (UINT32_C(1) << klass) | 
                         (UINT32_C(1) << (state + EMOJI_DFA_RECORD_STATE_SHIFT));
  return state;
}

static inline bool emoji_dfa_recorded_bitmask_contains_class(uint32_t recorded_bitmask,
                                                             emoji_dfa_class_t klass) {
  return recorded_bitmask & (UINT32_C(1) << klass);
}

static inline bool emoji_dfa_recorded_bitmask_contains_state(uint32_t recorded_bitmask,
                                                             emoji_dfa_state_t state) {
  return recorded_bitmask & (UINT32_C(1) << (state + EMOJI_DFA_RECORD_STATE_SHIFT));
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_DFA_H
