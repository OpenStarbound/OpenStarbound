#ifndef STAR_INPUT_EVENT_HPP
#define STAR_INPUT_EVENT_HPP

#include "StarString.hpp"
#include "StarBiMap.hpp"
#include "StarVariant.hpp"
#include "StarVector.hpp"

namespace Star {

enum class Key : uint16_t {
  Backspace,
  Tab,
  Clear,
  Return,
  Escape,
  Space,
  Exclaim,
  QuotedBL,
  Hash,
  Dollar,
  Ampersand,
  Quote,
  LeftParen,
  RightParen,
  Asterisk,
  Plus,
  Comma,
  Minus,
  Period,
  Slash,
  Zero,
  One,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,
  Colon,
  Semicolon,
  Less,
  Equals,
  Greater,
  Question,
  At,
  LeftBracket,
  Backslash,
  RightBracket,
  Caret,
  Underscore,
  Backquote,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Delete,
  Kp0,
  Kp1,
  Kp2,
  Kp3,
  Kp4,
  Kp5,
  Kp6,
  Kp7,
  Kp8,
  Kp9,
  Kp_period,
  Kp_divide,
  Kp_multiply,
  Kp_minus,
  Kp_plus,
  Kp_enter,
  Kp_equals,
  Up,
  Down,
  Right,
  Left,
  Insert,
  Home,
  End,
  PageUp,
  PageDown,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  NumLock,
  CapsLock,
  ScrollLock,
  RShift,
  LShift,
  RCtrl,
  LCtrl,
  RAlt,
  LAlt,
  RGui,
  LGui,
  AltGr,
  Compose,
  Help,
  PrintScreen,
  SysReq,
  Pause,
  Menu,
  Power
};
extern EnumMap<Key> const KeyNames;

enum class KeyMod : uint16_t {
  NoMod = 0x0000,
  LShift = 0x0001,
  RShift = 0x0002,
  LCtrl = 0x0040,
  RCtrl = 0x0080,
  LAlt = 0x0100,
  RAlt = 0x0200,
  LGui = 0x0400,
  RGui = 0x0800,
  Num = 0x1000,
  Caps = 0x2000,
  AltGr = 0x4000,
  Scroll = 0x8000
};
extern EnumMap<KeyMod> const KeyModNames;

KeyMod operator|(KeyMod a, KeyMod b);
KeyMod operator&(KeyMod a, KeyMod b);
KeyMod operator~(KeyMod a);
KeyMod& operator|=(KeyMod& a, KeyMod b);
KeyMod& operator&=(KeyMod& a, KeyMod b);

enum class MouseButton : uint8_t {
  Left,
  Middle,
  Right,
  FourthButton,
  FifthButton
};
extern EnumMap<MouseButton> const MouseButtonNames;

enum class MouseWheel : uint8_t {
  Up,
  Down
};
extern EnumMap<MouseWheel> const MouseWheelNames;

typedef uint32_t ControllerId;

enum class ControllerAxis : uint8_t {
  LeftX,
  LeftY,
  RightX,
  RightY,
  TriggerLeft,
  TriggerRight,
  Invalid = 255
};
extern EnumMap<ControllerAxis> const ControllerAxisNames;

enum class ControllerButton : uint8_t {
  A,
  B,
  X,
  Y,
  Back,
  Guide,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DPadUp,
  DPadDown,
  DPadLeft,
  DPadRight,
  Misc1,
  Paddle1,
  Paddle2,
  Paddle3,
  Paddle4,
  Touchpad,
  Invalid = 255
};
extern EnumMap<ControllerButton> const ControllerButtonNames;

struct KeyDownEvent {
  Key key;
  KeyMod mods;
};

struct KeyUpEvent {
  Key key;
};

struct TextInputEvent {
  String text;
};

struct MouseMoveEvent {
  Vec2I mouseMove;
  Vec2I mousePosition;
};

struct MouseButtonDownEvent {
  MouseButton mouseButton;
  Vec2I mousePosition;
};

struct MouseButtonUpEvent {
  MouseButton mouseButton;
  Vec2I mousePosition;
};

struct MouseWheelEvent {
  MouseWheel mouseWheel;
  Vec2I mousePosition;
};

struct ControllerAxisEvent {
  ControllerId controller;
  ControllerAxis controllerAxis;
  float controllerAxisValue;
};

struct ControllerButtonDownEvent {
  ControllerId controller;
  ControllerButton controllerButton;
};

struct ControllerButtonUpEvent {
  ControllerId controller;
  ControllerButton controllerButton;
};

typedef Variant<
    KeyDownEvent,
    KeyUpEvent,
    TextInputEvent,
    MouseMoveEvent,
    MouseButtonDownEvent,
    MouseButtonUpEvent,
    MouseWheelEvent,
    ControllerAxisEvent,
    ControllerButtonDownEvent,
    ControllerButtonUpEvent>
    InputEvent;

inline KeyMod operator|(KeyMod a, KeyMod b) {
  return (KeyMod)((uint16_t)a | (uint16_t)b);
}

inline KeyMod operator&(KeyMod a, KeyMod b) {
  return (KeyMod)((uint16_t)a & (uint16_t)b);
}

inline KeyMod operator~(KeyMod a) {
  return (KeyMod) ~(uint16_t)a;
}

inline KeyMod& operator|=(KeyMod& a, KeyMod b) {
  uint16_t a_cast = (uint16_t)a;
  a_cast |= (uint16_t)b;
  a = (KeyMod)a_cast;
  return a;
}

inline KeyMod& operator&=(KeyMod& a, KeyMod b) {
  uint16_t a_cast = (uint16_t)a;
  a_cast &= (uint16_t)b;
  a = (KeyMod)a_cast;
  return a;
}

}

#endif
