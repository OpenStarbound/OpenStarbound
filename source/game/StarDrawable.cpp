#include "StarDrawable.hpp"
#include "StarColor.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarAssets.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarGameTypes.hpp"
#include "StarRoot.hpp"

namespace Star {

Drawable::ImagePart& Drawable::ImagePart::addDirectives(Directives const& directives, bool keepImageCenterPosition) {
  if (!directives)
    return *this;

  if (keepImageCenterPosition) {
    auto imageMetadata = Root::singleton().imageMetadataDatabase();
    Vec2F imageSize = Vec2F(imageMetadata->imageSize(image));
    image.directives += directives;
    Vec2F newImageSize = Vec2F(imageMetadata->imageSize(image));

    // If we are trying to maintain the image center, PRE translate the image by
    // the change in size / 2
    transformation *= Mat3F::translation((imageSize - newImageSize) / 2);
  } else {
    image.directives += directives;
  }

  return *this;
}

Drawable::ImagePart& Drawable::ImagePart::addDirectivesGroup(DirectivesGroup const& directivesGroup, bool keepImageCenterPosition) {
  if (directivesGroup.empty())
    return *this;

  if (keepImageCenterPosition) {
    auto imageMetadata = Root::singleton().imageMetadataDatabase();
    Vec2F imageSize = Vec2F(imageMetadata->imageSize(image));
    for (Directives const& directives : directivesGroup.list())
      image.directives += directives;
    Vec2F newImageSize = Vec2F(imageMetadata->imageSize(image));

    // If we are trying to maintain the image center, PRE translate the image by
    // the change in size / 2
    transformation *= Mat3F::translation((imageSize - newImageSize) / 2);
  } else {
    for (Directives const& directives : directivesGroup.list())
       image.directives += directives;
  }

  return *this;
}

Drawable::ImagePart& Drawable::ImagePart::removeDirectives(bool keepImageCenterPosition) {
  if (keepImageCenterPosition) {
    auto imageMetadata = Root::singleton().imageMetadataDatabase();
    Vec2F imageSize = Vec2F(imageMetadata->imageSize(image));
    image.directives.clear();
    Vec2F newImageSize = Vec2F(imageMetadata->imageSize(image));

    // If we are trying to maintain the image center, PRE translate the image by
    // the change in size / 2
    transformation *= Mat3F::translation((imageSize - newImageSize) / 2);
  } else {
    image.directives.clear();
  }

  return *this;
}

Drawable Drawable::makeLine(Line2F const& line, float lineWidth, Color const& color, Vec2F const& position) {
  Drawable drawable;
  drawable.part = LinePart{move(line), lineWidth};
  drawable.color = color;
  drawable.position = position;

  return drawable;
}

Drawable Drawable::makePoly(PolyF poly, Color const& color, Vec2F const& position) {
  Drawable drawable;
  drawable.part = PolyPart{move(poly)};
  drawable.color = color;
  drawable.position = position;

  return drawable;
}

Drawable Drawable::makeImage(AssetPath image, float pixelSize, bool centered, Vec2F const& position, Color const& color) {
  Drawable drawable;
  Mat3F transformation = Mat3F::identity();
  if (centered) {
    auto imageMetadata = Root::singleton().imageMetadataDatabase();
    Vec2F imageSize = Vec2F(imageMetadata->imageSize(image));
    transformation.translate(-imageSize / 2);
  }

  transformation.scale(pixelSize);

  drawable.part = ImagePart{move(image), move(transformation)};
  drawable.position = position;
  drawable.color = color;

  return drawable;
}

Drawable::Drawable()
  : color(Color::White), fullbright(false) {}

Drawable::Drawable(Json const& json) {
  if (auto line = json.opt("line")) {
    part = LinePart{jsonToLine2F(*line), json.getFloat("width")};
  } else if (auto poly = json.opt("poly")) {
    part = PolyPart{jsonToPolyF(*poly)};
  } else if (auto image = json.opt("image")) {
    auto imageString = image->toString();
    Mat3F transformation = Mat3F::identity();
    if (auto transformationConfig = json.opt("transformation")) {
      transformation = jsonToMat3F(*transformationConfig);
    } else {
      if (json.getBool("centered", true)) {
        auto imageMetadata = Root::singleton().imageMetadataDatabase();
        Vec2F imageSize = Vec2F(imageMetadata->imageSize(imageString));
        transformation.translate(-imageSize / 2);
      }
      if (auto rotation = json.optFloat("rotation"))
        transformation.rotate(*rotation);
      if (json.getBool("mirrored", false))
        transformation.scale(Vec2F(-1, 1));
      if (auto scale = json.optFloat("scale"))
        transformation.scale(*scale);
    }

    part = ImagePart{move(imageString), move(transformation)};
  }
  position = json.opt("position").apply(jsonToVec2F).value();
  color = json.opt("color").apply(jsonToColor).value(Color::White);
  fullbright = json.getBool("fullbright", false);
}

Json Drawable::toJson() const {
  JsonObject json;
  if (auto line = part.ptr<LinePart>()) {
    json.set("line", jsonFromLine2F(line->line));
    json.set("width", line->width);
  } else if (auto poly = part.ptr<PolyPart>()) {
    json.set("poly", jsonFromPolyF(poly->poly));
  } else if (auto image = part.ptr<ImagePart>()) {
    json.set("image", AssetPath::join(image->image));
    json.set("transformation", jsonFromMat3F(image->transformation));
  }

  json.set("position", jsonFromVec2F(position));
  json.set("color", jsonFromColor(color));
  json.set("fullbright", fullbright);

  return json;
}

void Drawable::translate(Vec2F const& translation) {
  position += translation;
}

void Drawable::rotate(float rotation, Vec2F const& rotationCenter) {
  if (auto line = part.ptr<LinePart>())
    line->line.rotate(rotation);
  else if (auto poly = part.ptr<PolyPart>())
    poly->poly.rotate(rotation);
  else if (auto image = part.ptr<ImagePart>())
    image->transformation.rotate(rotation);

  position = (position - rotationCenter).rotate(rotation) + rotationCenter;
}

void Drawable::scale(float scaling, Vec2F const& scaleCenter) {
  scale(Vec2F::filled(scaling), scaleCenter);
}

void Drawable::scale(Vec2F const& scaling, Vec2F const& scaleCenter) {
  if (auto line = part.ptr<LinePart>())
    line->line.scale(scaling);
  else if (auto poly = part.ptr<PolyPart>())
    poly->poly.scale(scaling);
  else if (auto image = part.ptr<ImagePart>())
    image->transformation.scale(scaling);

  position = (position - scaleCenter).piecewiseMultiply(scaling) + scaleCenter;
}

void Drawable::transform(Mat3F const& transformation) {
  Vec2F localTranslation = transformation.transformVec2(Vec2F());
  Mat3F localTransform = Mat3F::translation(-localTranslation) * transformation;

  if (auto line = part.ptr<LinePart>())
    line->line.transform(localTransform);
  else if (auto poly = part.ptr<PolyPart>())
    poly->poly.transform(localTransform);
  else if (auto image = part.ptr<ImagePart>())
    image->transformation = localTransform * image->transformation;

  position = transformation.transformVec2(position);
}

void Drawable::rebase(Vec2F const& newBase) {
  if (auto line = part.ptr<LinePart>())
    line->line.translate(position - newBase);
  else if (auto poly = part.ptr<PolyPart>())
    poly->poly.translate(position - newBase);
  else if (auto image = part.ptr<ImagePart>())
    image->transformation.translate(position - newBase);

  position = newBase;
}

RectF Drawable::boundBox(bool cropImages) const {
  RectF boundBox = RectF::null();
  if (auto line = part.ptr<LinePart>()) {
    boundBox.combine(line->line.min());
    boundBox.combine(line->line.max());

  } else if (auto poly = part.ptr<PolyPart>()) {
    boundBox.combine(poly->poly.boundBox());

  } else if (auto image = part.ptr<ImagePart>()) {
    auto imageMetadata = Root::singleton().imageMetadataDatabase();
    RectF imageRegion = RectF::null();
    if (cropImages) {
      RectU nonEmptyRegion = imageMetadata->nonEmptyRegion(image->image);
      if (!nonEmptyRegion.isNull())
        imageRegion = RectF(nonEmptyRegion);
    } else {
      imageRegion = RectF::withSize(Vec2F(), Vec2F(imageMetadata->imageSize(image->image)));
    }

    if (!imageRegion.isNull()) {
      boundBox.combine(image->transformation.transformVec2(Vec2F(imageRegion.xMin(), imageRegion.yMin())));
      boundBox.combine(image->transformation.transformVec2(Vec2F(imageRegion.xMax(), imageRegion.yMin())));
      boundBox.combine(image->transformation.transformVec2(Vec2F(imageRegion.xMin(), imageRegion.yMax())));
      boundBox.combine(image->transformation.transformVec2(Vec2F(imageRegion.xMax(), imageRegion.yMax())));
    }
  }

  if (!boundBox.isNull())
    boundBox.translate(position);

  return boundBox;
}

DataStream& operator>>(DataStream& ds, Drawable::LinePart& line) {
  ds >> line.line;
  ds >> line.width;
  return ds;
}

DataStream& operator<<(DataStream& ds, Drawable::LinePart const& line) {
  ds << line.line;
  ds << line.width;
  return ds;
}

DataStream& operator>>(DataStream& ds, Drawable::PolyPart& poly) {
  ds >> poly.poly;
  return ds;
}

DataStream& operator<<(DataStream& ds, Drawable::PolyPart const& poly) {
  ds << poly.poly;
  return ds;
}

// I need to find out if this is for network serialization or not eventually
DataStream& operator>>(DataStream& ds, Drawable::ImagePart& image) {
  ds >> image.image;
  ds >> image.transformation;
  return ds;
}

DataStream& operator<<(DataStream& ds, Drawable::ImagePart const& image) {
  ds << image.image;
  ds << image.transformation;
  return ds;
}

DataStream& operator>>(DataStream& ds, Drawable& drawable) {
  ds >> drawable.part;
  ds >> drawable.position;
  ds >> drawable.color;
  ds >> drawable.fullbright;
  return ds;
}

DataStream& operator<<(DataStream& ds, Drawable const& drawable) {
  ds << drawable.part;
  ds << drawable.position;
  ds << drawable.color;
  ds << drawable.fullbright;
  return ds;
}

}
