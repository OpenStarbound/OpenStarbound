#include "StarItemSlotWidget.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarWidgetParsing.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarItem.hpp"
#include "StarDurabilityItem.hpp"
#include "StarAssets.hpp"
#include "StarGameTypes.hpp"

namespace Star {

ItemSlotWidget::ItemSlotWidget(ItemPtr const& item, String const& backingImage)
  : m_item(item), m_backingImage(backingImage) {
  m_drawBackingImageWhenFull = false;
  m_drawBackingImageWhenEmpty = true;
  m_progress = 1;

  auto assets = Root::singleton().assets();
  auto interfaceConfig = assets->json("/interface.config");
  m_countPosition = TextPositioning(jsonToVec2F(interfaceConfig.get("itemCountRightAnchor")), HorizontalAnchor::RightAnchor);
  m_countFontMode = FontMode::Normal;
  m_textStyle = interfaceConfig.get("itemSlotTextStyle");
  m_itemDraggableArea = jsonToRectI(interfaceConfig.get("itemDraggableArea"));
  m_durabilityOffset = jsonToVec2I(interfaceConfig.get("itemIconDurabilityOffset"));

  auto newItemIndicatorConfig = interfaceConfig.get("newItemAnimation");
  m_newItemIndicator = Animation(newItemIndicatorConfig);
  // End animation before it begins, only display when triggered
  m_newItemIndicator.update(newItemIndicatorConfig.getDouble("animationCycle") * newItemIndicatorConfig.getDouble("loops", 1.0f));

  Json highlightAnimationConfig = interfaceConfig.get("highlightAnimation");
  m_highlightAnimation = Animation(highlightAnimationConfig);
  m_highlightEnabled = false;

  Vec2I backingImageSize;
  if (m_backingImage.size()) {
    auto imgMetadata = Root::singleton().imageMetadataDatabase();
    backingImageSize = Vec2I(imgMetadata->imageSize(m_backingImage));
  }
  setSize(m_itemDraggableArea.max().piecewiseMax(backingImageSize));

  WidgetParser parser;

  parser.construct(assets->json("/interface/itemSlot.config").get("config"), this);
  m_durabilityBar = fetchChild<ProgressWidget>("durabilityBar");
  m_durabilityBar->hide();
  m_showDurability = false;
  m_showCount = true;
  m_showRarity = true;
  m_showLinkIndicator = false;

  disableScissoring();
}

void ItemSlotWidget::update(float dt) {
  if (m_item)
    m_newItemIndicator.update(dt);
  if (m_highlightEnabled)
    m_highlightAnimation.update(dt);
  Widget::update(dt);
}

bool ItemSlotWidget::sendEvent(InputEvent const& event) {
  if (m_visible) {
    if (auto mouseButton = event.ptr<MouseButtonDownEvent>()) {
      if (mouseButton->mouseButton == MouseButton::Left
        || (m_rightClickCallback && mouseButton->mouseButton == MouseButton::Right)
        || (m_middleClickCallback && mouseButton->mouseButton == MouseButton::Middle)) {
        Vec2I mousePos = *context()->mousePosition(event);
        RectI itemArea = m_itemDraggableArea.translated(screenPosition());
        if (itemArea.contains(mousePos)) {
          if (mouseButton->mouseButton == MouseButton::Right)
            m_rightClickCallback(this);
          else if (mouseButton->mouseButton == MouseButton::Middle)
            m_middleClickCallback(this);
          else
            m_callback(this);
          return true;
        }
      }
    }
  }

  return false;
}

void ItemSlotWidget::setCallback(WidgetCallbackFunc callback) {
  m_callback = callback;
}

void ItemSlotWidget::setRightClickCallback(WidgetCallbackFunc callback) {
  m_rightClickCallback = callback;
}

void ItemSlotWidget::setMiddleClickCallback(WidgetCallbackFunc callback) {
  m_middleClickCallback = callback;
}

void ItemSlotWidget::setItem(ItemPtr const& item) {
  m_item = item;
}

ItemPtr ItemSlotWidget::item() const {
  return m_item;
}

void ItemSlotWidget::setProgress(float progress) {
  m_progress = progress;
}

void ItemSlotWidget::setBackingImageAffinity(bool full, bool empty) {
  m_drawBackingImageWhenFull = full;
  m_drawBackingImageWhenEmpty = empty;
}

void ItemSlotWidget::setCountPosition(TextPositioning textPositioning) {
  m_countPosition = textPositioning;
}

void ItemSlotWidget::setCountFontMode(FontMode fontMode) {
  m_countFontMode = fontMode;
}

void ItemSlotWidget::showDurability(bool show) {
  m_showDurability = show;
}

void ItemSlotWidget::showCount(bool show) {
  m_showCount = show;
}

void ItemSlotWidget::showRarity(bool showRarity) {
  m_showRarity = showRarity;
}

void ItemSlotWidget::showLinkIndicator(bool showLinkIndicator) {
  m_showLinkIndicator = showLinkIndicator;
}

void ItemSlotWidget::indicateNew() {
  m_newItemIndicator.reset();
}

void ItemSlotWidget::setHighlightEnabled(bool highlight) {
  if (!m_highlightEnabled && highlight)
    m_highlightAnimation.reset();
  m_highlightEnabled = highlight;
}

void ItemSlotWidget::renderImpl() {
  if (m_item) {
    if (m_drawBackingImageWhenFull && m_backingImage != "")
      context()->drawInterfaceQuad(m_backingImage, Vec2F(screenPosition()));

    List<Drawable> iconDrawables = m_item->iconDrawables();

    if (m_showRarity) {
      String border = rarityBorder(m_item->rarity());
      context()->drawInterfaceQuad(border, Vec2F(screenPosition()));
    }

    if (m_showLinkIndicator) {
      // TODO: Hardcoded
      context()->drawInterfaceQuad(String("/interface/inventory/itemlinkindicator.png"), Vec2F(screenPosition() - Vec2I(1, 1)));
    }

    for (auto i : iconDrawables)
      context()->drawInterfaceDrawable(i, Vec2F(screenPosition() + size() / 2));

    if (!m_newItemIndicator.isComplete())
      context()->drawInterfaceDrawable(m_newItemIndicator.drawable(1.0), Vec2F(screenPosition() + size() / 2), Color::White.toRgba());

    if (m_showDurability) {
      if (auto durabilityItem = as<DurabilityItem>(m_item)) {
        float amount = durabilityItem->durabilityStatus();
        m_durabilityBar->setCurrentProgressLevel(amount);

        if (amount < 1)
          m_durabilityBar->show();
        else
          m_durabilityBar->hide();
      } else {
        m_durabilityBar->hide();
      }
    }

if (m_item->count() > 1 && m_showCount) {  // we don't need to tell people that there's only 1 of something
    std::string formattedCount;

    if (m_item->count() >= 1000000000000000) { // Quadrillion (q)
        formattedCount = toString(m_item->count() / 1000000000000000) + "q";
    } else if (m_item->count() >= 1000000000000) { // Trillion (t)
        formattedCount = toString(m_item->count() / 1000000000000) + "t";
    } else if (m_item->count() >= 1000000000) { // Billion (b)
        formattedCount = toString(m_item->count() / 1000000000) + "b";
    } else if (m_item->count() >= 1000000) { // Million (m)
        formattedCount = toString(m_item->count() / 1000000) + "m";
    } else if (m_item->count() >= 1000) { // Thousand (k)
        formattedCount = toString(m_item->count() / 10000) + "k";
    } else {
        formattedCount = toString(m_item->count());
    }

    context()->setTextStyle(m_textStyle);
    context()->setFontMode(m_countFontMode);
    context()->renderInterfaceText(formattedCount, m_countPosition.translated(Vec2F(screenPosition())));
    context()->clearTextStyle();
}

  } else if (m_drawBackingImageWhenEmpty && m_backingImage != "") {
    context()->drawInterfaceQuad(m_backingImage, Vec2F(screenPosition()));
    int frame = (int)roundf(m_progress * 18); // TODO: Hardcoded lol
    context()->drawInterfaceQuad(String(strf("/interface/cooldown.png:{}", frame)), Vec2F(screenPosition()));
  }

  if (m_highlightEnabled) {
    context()->drawInterfaceDrawable(m_highlightAnimation.drawable(1.0), Vec2F(screenPosition() + size() / 2), Color::White.toRgba());
  }

  if (!m_item)
    m_durabilityBar->hide();
}

}
