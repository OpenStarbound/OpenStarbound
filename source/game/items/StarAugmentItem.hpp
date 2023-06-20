#ifndef STAR_AUGMENT_ITEM_HPP
#define STAR_AUGMENT_ITEM_HPP

#include "StarItem.hpp"

namespace Star {

STAR_CLASS(AugmentItem);

class AugmentItem : public Item {
public:
  AugmentItem(Json const& config, String const& directory, Json const& parameters = JsonObject());
  AugmentItem(AugmentItem const& rhs);

  ItemPtr clone() const override;

  StringList augmentScripts() const;

  // Makes no change to the given item if the augment can't be applied.
  // Consumes itself and returns true if the augment is applied.
  // Has no effect if augmentation fails.
  ItemPtr applyTo(ItemPtr const item);
};

}

#endif
