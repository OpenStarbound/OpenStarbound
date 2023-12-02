#ifndef STAR_ITEM_HPP
#define STAR_ITEM_HPP

#include "StarSet.hpp"
#include "StarDrawable.hpp"
#include "StarItemDescriptor.hpp"
#include "StarQuests.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(GenericItem);

STAR_EXCEPTION(ItemException, StarException);

/**
 * Item logic.
 *
 * \attention
 * A lot of data is directly stored in this class, so strings and json
 * float around even to places where they are not needed. Consider making
 * the item class a handle to an item loaded to memory.
 */
class Item {
  public:
    /**
    * Config here is the configurationloaded directly from assets, directory is
    * the asset path this config was found in, that other assets should be
    * loaded relative to.
    */
    Item(Json config, String directory, Json parameters = JsonObject());

    /**
    * For items which do not come from files
    */
    Item();

    virtual ~Item();

    virtual ItemPtr clone() const = 0;

    /**
    * Unique identifying item name
    */
    String name() const;

    /**
    * Number of this item that is available.
    */
    uint64_t count() const;
    /**
    * Sets the new item count, up to a max of the maximum stack size.  If this
    * value is over stack size, returns the overflow.  If 'overfill' is set to
    * true, thenwill fill past max stack level.
    */
    uint64_t setCount(uint64_t count, bool overfill = false);

    /**
    * Is this item type stackable with the givenitem type at all?  Base class
    * implementationcompares name(), and m_parameters fields and returns true
    * if they are both the same, similarly to matches.
    */
    virtual bool stackableWith(ItemConstPtr const &item) const;
    uint64_t maxStack() const;

    /**
    * Returnhow many of the givenitem could be shifted into this item, taking
    * into acount whether the item is stackable at all, as well as maxStack and
    * the count available.
    */
    uint64_t couldStack(ItemConstPtr const &item) const;

    /**
    * If the givenitem is stackable with this one, takes as many from the given
    * item as possible and shifts it into this item's count.  Returns true if
    * any items at all were shifted.
    */
    bool stackWith(ItemPtr const &item);

    /**
    * Does this item match the givenitem or itemDescriptor
    */
    bool matches(ItemDescriptor const &descriptor, bool exactMatch = false) const;
    bool matches(ItemConstPtr const &other, bool exactMatch = false) const;

    /**
    * List of itemdescriptors for which the current item could be used inthe
    * place of
    * inrecipes and the like.
    */
    List<ItemDescriptor> matchingDescriptors() const;

    /**
    * If the givennumber of this item is available, consumes that number and
    * returns true, otherwise returns false.
    */
    bool consume(uint64_t count);

    /**
    * Take as many of this item as possible up to the givenmax (default is all)
    * and returnthe new set.  Implementationuses clone() method.
    */
    ItemPtr take(uint64_t max = NPos);

    /**
    * count() is 0
    */
    bool empty() const;

    /**
    * Builds a descriptor out of name(), count(), and m_parameters
    */
    ItemDescriptor descriptor() const;

    String description() const;
    String friendlyName() const;

    Rarity rarity() const;
    uint64_t price() const;

    virtual List<Drawable> iconDrawables() const;
    virtual List<Drawable> dropDrawables() const;
    String largeImage() const;

    String tooltipKind() const;
    virtual String category() const;

    virtual String pickupSound() const;

    bool twoHanded() const;
    float timeToLive() const;

    List<ItemDescriptor> learnBlueprintsOnPickup() const;
    StringMap<String> collectablesOnPickup() const;

    List<QuestArcDescriptor> pickupQuestTemplates() const;
    StringSet itemTags() const;
    bool hasItemTag(String const &itemTag) const;

    /**
    * Returneither a parameter givento the item or a config value, if no such
    * parameter exists.
    */
    Json instanceValue(String const &name, Json const &def = Json()) const;

    /**
    * Returns the full set of configurationvalues merged with parameters
    */
    Json instanceValues() const;

    /**
    * Returns just the base config
    */
    Json config() const;

    /**
    * Returns just the dynamic parameters
    */
    Json parameters() const;

    static bool itemsEqual(ItemConstPtr const &a, ItemConstPtr const &b);

  protected:
    void setMaxStack(uint64_t maxStack);
    void setDescription(String const &description);
    void setRarity(Rarity rarity);
    void setPrice(uint64_t price);
    // icon drawables are pixels, not tile, based
    void setIconDrawables(List<Drawable> drawables);
    void setTwoHanded(bool twoHanded);
    void setTimeToLive(float timeToLive);

    void setInstanceValue(String const &name, Json const &val);

    String const &directory() const;

  private:
    Json m_config;
    String m_directory;

    String m_name;
    uint64_t m_count;
    Json m_parameters;

    uint64_t m_maxStack;
    String m_shortDescription;
    String m_description;
    Rarity m_rarity;
    List<Drawable> m_iconDrawables;
    bool m_twoHanded;
    float m_timeToLive;
    uint64_t m_price;
    String m_tooltipKind;
    String m_largeImage;
    String m_category;
    StringSet m_pickupSounds;

    List<ItemDescriptor> m_matchingDescriptors;
    List<ItemDescriptor> m_learnBlueprintsOnPickup;
    StringMap<String> m_collectablesOnPickup;
};

class GenericItem : public Item {
public:
  GenericItem(Json const &config, String const &directory,
              Json const &parameters);
  virtual ItemPtr clone() const;
};

inline uint64_t itemSafeCount(ItemPtr const &item) {
  return item ? item->count() : 0;
}

inline bool itemSafeTwoHanded(ItemPtr const &item) {
  return item && item->twoHanded();
}

inline bool itemSafeOneHanded(ItemPtr const &item) {
  return item && !item->twoHanded();
}

inline ItemDescriptor itemSafeDescriptor(ItemPtr const &item) {
  return item ? item->descriptor() : ItemDescriptor();
}
} // namespace Star

#endif
