#include "StarMovementControllerLuaBindings.hpp"
#include "StarMovementController.hpp"
#include "StarLuaGameConverters.hpp"

namespace Star {

LuaCallbacks LuaBindings::makeMovementControllerCallbacks(MovementController* movementController) {
  LuaCallbacks callbacks;

  callbacks.registerCallback(
      "parameters", [movementController]() { return movementController->parameters().toJson(); });
  callbacks.registerCallbackWithSignature<void, Json>(
      "applyParameters", bind(&MovementController::applyParameters, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Json>(
      "resetParameters", bind(&MovementController::resetParameters, movementController, _1));
  callbacks.registerCallbackWithSignature<float>("mass", bind(&MovementController::mass, movementController));
  callbacks.registerCallbackWithSignature<PolyF>(
      "collisionPoly", bind(&MovementController::collisionPoly, movementController));
  callbacks.registerCallback("boundBox", [movementController]() {
      return movementController->collisionPoly().boundBox();
    });

  callbacks.registerCallbackWithSignature<Vec2F>("position", bind(&MovementController::position, movementController));
  callbacks.registerCallbackWithSignature<float>("xPosition", bind(&MovementController::xPosition, movementController));
  callbacks.registerCallbackWithSignature<float>("yPosition", bind(&MovementController::yPosition, movementController));
  callbacks.registerCallbackWithSignature<Vec2F>("velocity", bind(&MovementController::velocity, movementController));
  callbacks.registerCallbackWithSignature<float>("xVelocity", bind(&MovementController::xVelocity, movementController));
  callbacks.registerCallbackWithSignature<float>("yVelocity", bind(&MovementController::yVelocity, movementController));
  callbacks.registerCallbackWithSignature<float>("rotation", bind(&MovementController::rotation, movementController));
  callbacks.registerCallbackWithSignature<PolyF>(
      "collisionBody", bind(&MovementController::collisionBody, movementController));
  callbacks.registerCallbackWithSignature<RectF>(
      "collisionBoundBox", bind(&MovementController::collisionBoundBox, movementController));
  callbacks.registerCallbackWithSignature<RectF>(
      "localBoundBox", bind(&MovementController::localBoundBox, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isColliding", bind(&MovementController::isColliding, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isNullColliding", bind(&MovementController::isNullColliding, movementController));
  callbacks.registerCallbackWithSignature<bool>(
      "isCollisionStuck", bind(&MovementController::isCollisionStuck, movementController));
  callbacks.registerCallbackWithSignature<Maybe<float>>(
      "stickingDirection", bind(&MovementController::stickingDirection, movementController));
  callbacks.registerCallbackWithSignature<float>(
      "liquidPercentage", bind(&MovementController::liquidPercentage, movementController));
  callbacks.registerCallbackWithSignature<LiquidId>(
      "liquidId", bind(&MovementController::liquidId, movementController));
  callbacks.registerCallbackWithSignature<bool>("onGround", bind(&MovementController::onGround, movementController));
  callbacks.registerCallbackWithSignature<bool>("zeroG", bind(&MovementController::zeroG, movementController));
  callbacks.registerCallbackWithSignature<bool, bool>("atWorldLimit", bind(&MovementController::atWorldLimit, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "setPosition", bind(&MovementController::setPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setXPosition", bind(&MovementController::setXPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setYPosition", bind(&MovementController::setYPosition, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "translate", bind(&MovementController::translate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "setVelocity", bind(&MovementController::setVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setXVelocity", bind(&MovementController::setXVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setYVelocity", bind(&MovementController::setYVelocity, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "addMomentum", bind(&MovementController::addMomentum, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "setRotation", bind(&MovementController::setRotation, movementController, _1));
  callbacks.registerCallbackWithSignature<void, float>(
      "rotate", bind(&MovementController::rotate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "accelerate", bind(&MovementController::accelerate, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F>(
      "force", bind(&MovementController::force, movementController, _1));
  callbacks.registerCallbackWithSignature<void, Vec2F, float>(
      "approachVelocity", bind(&MovementController::approachVelocity, movementController, _1, _2));
  callbacks.registerCallbackWithSignature<void, float, float, float, bool>("approachVelocityAlongAngle",
      bind(&MovementController::approachVelocityAlongAngle, movementController, _1, _2, _3, _4));
  callbacks.registerCallbackWithSignature<void, float, float>(
      "approachXVelocity", bind(&MovementController::approachXVelocity, movementController, _1, _2));
  callbacks.registerCallbackWithSignature<void, float, float>(
      "approachYVelocity", bind(&MovementController::approachYVelocity, movementController, _1, _2));

  return callbacks;
}

}
