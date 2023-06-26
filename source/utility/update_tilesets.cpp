#include "StarAssets.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarObject.hpp"
#include "StarObjectDatabase.hpp"
#include "StarRootLoader.hpp"
#include "tileset_updater.hpp"

using namespace Star;

String const InboundNode = "/tilesets/inboundnode.png";
String const OutboundNode = "/tilesets/outboundnode.png";
Vec3B const SourceLiquidBorderColor(0x80, 0x80, 0x00);

void scanMaterials(TilesetUpdater& updater) {
  auto& root = Root::singleton();
  auto materials = root.materialDatabase();

  for (String materialName : materials->materialNames()) {
    MaterialId id = materials->materialId(materialName);
    Maybe<String> path = materials->materialPath(id);
    if (!path)
      continue;
    String source = root.assets()->assetSource(*path);

    auto renderProfile = materials->materialRenderProfile(id);
    if (renderProfile == nullptr)
      continue;

    String tileset = materials->materialCategory(id);
    String imagePath = renderProfile->pieceImage(renderProfile->representativePiece, 0);
    ImageConstPtr image = root.assets()->image(imagePath);

    Tiled::Properties properties;
    properties.set("material", materialName);
    properties.set("//name", materialName);
    properties.set("//shortdescription", materials->materialShortDescription(id));
    properties.set("//description", materials->materialDescription(id));

    auto tile = make_shared<Tile>(Tile{source, "materials", tileset.toLower(), materialName, image, properties});
    updater.defineTile(tile);
  }
}

// imagePosition might not be aligned to a whole number, i.e. the image origin
// might not align with the tile grid. We do, however want Tile Objects in Tiled
// to be grid-aligned (valid positions are offset relative to the grid not
// completely free-form), so we correct the alignment by adding padding to the
// image that we export.
// We're going to ignore the fact that some objects have imagePositions that
// aren't even aligned _to pixels_ (e.g. giftsmallmonsterbox).
Vec2U objectPositionPadding(Vec2I imagePosition) {
  int pixelsX = imagePosition.x();
  int pixelsY = imagePosition.y();

  // Unsigned modulo operation gives the padding to use (in pixels)
  unsigned padX = (unsigned)pixelsX % TilePixels;
  unsigned padY = (unsigned)pixelsY % TilePixels;
  return Vec2U(padX, padY);
}

StringSet categorizeObject(String const& objectName, Vec2U imageSize) {
  if (imageSize[0] >= 256 || imageSize[1] >= 256)
    return StringSet{"huge-objects"};

  auto& root = Root::singleton();
  auto assets = root.assets();
  auto objects = root.objectDatabase();

  Json defaultCategories = assets->json("/objects/defaultCategories.config");

  auto objectConfig = objects->getConfig(objectName);

  StringSet categories;
  if (objectConfig->category != defaultCategories.getString("category"))
    categories.insert("objects-by-category/" + objectConfig->category);
  for (String const& tag : objectConfig->colonyTags)
    categories.insert("objects-by-colonytag/" + tag);
  if (objectConfig->type != defaultCategories.getString("objectType"))
    categories.insert("objects-by-type/" + objectConfig->type);
  if (objectConfig->race != defaultCategories.getString("race"))
    categories.insert("objects-by-race/" + objectConfig->race);

  if (categories.size() == 0)
    categories.insert("objects-uncategorized");

  return transform<StringSet>(categories, [](String const& category) { return category.toLower(); });
}

void drawNodes(ImagePtr const& image, Vec2I imagePosition, JsonArray nodes, String nodeImagePath) {
  ImageConstPtr nodeImage = Root::singleton().assets()->image(nodeImagePath);
  for (Json const& node : nodes) {
    Vec2I nodePos = jsonToVec2I(node) * TilePixels + Vec2I(0, TilePixels - nodeImage->height());
    Vec2U nodeImagePos = Vec2U(nodePos - imagePosition);
    image->drawInto(nodeImagePos, *nodeImage);
  }
}

void defineObjectOrientation(TilesetUpdater& updater,
    String const& objectName,
    List<ObjectOrientationPtr> const& orientations,
    int orientationIndex) {
  auto& root = Root::singleton();
  auto assets = root.assets();
  auto objects = root.objectDatabase();

  ObjectOrientationPtr orientation = orientations[orientationIndex];

  Vec2I imagePosition = Vec2I(orientation->imagePosition * TilePixels);

  List<ImageConstPtr> layers;
  unsigned width = 0, height = 0;
  for (auto const& imageLayer : orientation->imageLayers) {
    String imageName = AssetPath::join(imageLayer.imagePart().image).replaceTags(StringMap<String>{}, true, "default");

    ImageConstPtr image = assets->image(imageName);
    layers.append(image);
    width = max(width, image->width());
    height = max(height, image->height());
  }

  Vec2U imagePadding = objectPositionPadding(imagePosition);
  imagePosition -= Vec2I(imagePadding);

  // Padding is added to the right hand side as well as the left so that
  // when objects are flipped in the editor, they're still aligned correctly.
  Vec2U imageSize(width + 2 * imagePadding.x(), height + imagePadding.y());

  ImagePtr combinedImage = make_shared<Image>(imageSize, PixelFormat::RGBA32);
  combinedImage->fill(Vec4B(0, 0, 0, 0));
  for (ImageConstPtr const& layer : layers) {
    combinedImage->drawInto(imagePadding, *layer);
  }

  // Overlay the image with the wiring nodes:
  auto objectConfig = objects->getConfig(objectName);

  drawNodes(combinedImage, imagePosition, objectConfig->config.getArray("inputNodes", {}), InboundNode);
  drawNodes(combinedImage, imagePosition, objectConfig->config.getArray("outputNodes", {}), OutboundNode);

  ObjectPtr example = objects->createObject(objectName);

  Tiled::Properties properties;
  properties.set("object", objectName);
  properties.set("imagePositionX", imagePosition.x());
  properties.set("imagePositionY", imagePosition.y());
  properties.set("//shortdescription", example->shortDescription());
  properties.set("//description", example->description());

  if (orientation->directionAffinity.isValid()) {
    Direction direction = *orientation->directionAffinity;
    if (orientation->flipImages)
      direction = -direction;
    properties.set("tilesetDirection", DirectionNames.getRight(direction));
  }

  StringSet tilesets = categorizeObject(objectName, imageSize);

  // tileName becomes part of the filename for the tile's image. Different
  // orientations require different images, so the tileName must be different
  // for each orientation.
  String tileName = objectName;
  if (orientationIndex != 0)
    tileName += "_orientation" + toString(orientationIndex);
  properties.set("//name", tileName);

  String source = assets->assetSource(objectConfig->path);

  for (String const& tileset : tilesets) {
    TilePtr tile = make_shared<Tile>(Tile{source, "objects", tileset, tileName, combinedImage, properties});
    updater.defineTile(tile);
  }
}

void scanObjects(TilesetUpdater& updater) {
  auto& root = Root::singleton();
  auto objects = root.objectDatabase();

  for (String const& objectName : objects->allObjects()) {
    auto orientations = objects->getOrientations(objectName);
    if (orientations.size() < 1) {
      Logger::warn("Object %s has no orientations and will not be exported", objectName);
      continue;
    }

    // Always export the first orientation
    ObjectOrientationPtr orientation = orientations[0];
    defineObjectOrientation(updater, objectName, orientations, 0);

    // If there are more than 2 orientations or the imagePositions are different
    // then horizontal flipping in the editor is not enough to get all the
    // orientations and display them correctly, so we export each orientation
    // as a separate tile.
    for (unsigned i = 1; i < orientations.size(); ++i) {
      if (i >= 2 || orientation->imagePosition != orientations[i]->imagePosition)
        defineObjectOrientation(updater, objectName, orientations, i);
    }
  }
}

void scanLiquids(TilesetUpdater& updater) {
  auto& root = Root::singleton();
  auto liquids = root.liquidsDatabase();
  auto assets = root.assets();

  Vec2U imageSize(TilePixels, TilePixels);

  for (auto liquid : liquids->allLiquidSettings()) {
    ImagePtr image = make_shared<Image>(imageSize, PixelFormat::RGBA32);
    image->fill(liquid->liquidColor);

    String assetSource = assets->assetSource(liquid->path);

    Tiled::Properties properties;
    properties.set("liquid", liquid->name);
    properties.set("//name", liquid->name);
    auto tile = make_shared<Tile>(Tile{assetSource, "liquids", "liquids", liquid->name, image, properties});
    updater.defineTile(tile);

    ImagePtr sourceImage = make_shared<Image>(imageSize, PixelFormat::RGBA32);
    sourceImage->copyInto(Vec2U(), *image.get());
    sourceImage->fillRect(Vec2U(), Vec2U(image->width(), 1), SourceLiquidBorderColor);
    sourceImage->fillRect(Vec2U(), Vec2U(1, image->height()), SourceLiquidBorderColor);

    String sourceName = liquid->name + "_source";
    properties.set("source", true);
    properties.set("//name", sourceName);
    properties.set("//shortdescription", "Endless " + liquid->name);
    auto sourceTile = make_shared<Tile>(Tile{assetSource, "liquids", "liquids", sourceName, sourceImage, properties});
    updater.defineTile(sourceTile);
  }
}

int main(int argc, char** argv) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});

    rootLoader.setSummary("Updates Tiled JSON tilesets in unpacked assets directories");

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    TilesetUpdater updater;

    for (String source : root->assets()->assetSources()) {
      Logger::info("Assets source: \"%s\"", source);
      updater.defineAssetSource(source);
    }

    scanMaterials(updater);
    scanObjects(updater);
    scanLiquids(updater);

    updater.exportTilesets();

    return 0;
  } catch (std::exception const& e) {
    cerrf("exception caught: %s\n", outputException(e, true));
    return 1;
  }
}
