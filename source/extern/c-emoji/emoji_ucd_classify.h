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

/* Maps a Unicode codepoint to a DFA character class.
 *
 * Predicates are tested most-specific first.  Keycap bases (0-9, *, #),
 * modifiers, and regional indicators all carry the Emoji property and would
 * otherwise fall through to EMOJI_DFA_CLASS_EMOJI, producing wrong transitions.
 */
#ifndef EMOJI_UCD_CLASSIFY_H
#define EMOJI_UCD_CLASSIFY_H
#include <stddef.h>
#include <stdint.h>

#include "emoji_ucd.h"
#include "emoji_dfa.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline emoji_dfa_class_t emoji_ucd_classify(uint32_t cp) {
  if (cp <= 0x200D) {
    // ZERO WIDTH JOINER
    if (cp == 0x200D)
      return EMOJI_DFA_CLASS_ZWJ;
    // U+00A9 COPYRIGHT SIGN; U+00AE REGISTERED SIGN
    if (cp == 0xA9 || cp == 0xAE)
      return EMOJI_DFA_CLASS_EMOJI;
    if (emoji_ucd_is_keycap_base(cp))
      return EMOJI_DFA_CLASS_KEYCAP_BASE;
    return EMOJI_DFA_CLASS_OTHER;
  }
  if (emoji_ucd_is_text_presentation_selector(cp))
    return EMOJI_DFA_CLASS_VS15;
  if (emoji_ucd_is_emoji_presentation_selector(cp))
    return EMOJI_DFA_CLASS_VS16;
  if (emoji_ucd_is_enclosing_keycap(cp))
    return EMOJI_DFA_CLASS_KEYCAP_TERM;
  if (emoji_ucd_is_tag_base(cp))
    return EMOJI_DFA_CLASS_TAG_BASE;
  if (emoji_ucd_is_tag_spec(cp))
    return EMOJI_DFA_CLASS_TAG_SPEC;
  if (emoji_ucd_is_tag_term(cp))
    return EMOJI_DFA_CLASS_TAG_TERM;
  if (emoji_ucd_is_regional_indicator(cp))
    return EMOJI_DFA_CLASS_RI;
  if (emoji_ucd_is_modifier(cp))
    return EMOJI_DFA_CLASS_MODIFIER;
  if (emoji_ucd_is_modifier_base(cp))
    return EMOJI_DFA_CLASS_MODIFIER_BASE;
  if (emoji_ucd_is_emoji(cp))
    return EMOJI_DFA_CLASS_EMOJI;
  return EMOJI_DFA_CLASS_OTHER;
}

#ifdef __cplusplus
}
#endif
#endif // EMOJI_UCD_CLASSIFY_H
