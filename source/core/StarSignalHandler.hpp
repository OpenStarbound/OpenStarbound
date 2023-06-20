#ifndef STAR_SIGNAL_HANDLER_HPP
#define STAR_SIGNAL_HANDLER_HPP

#include "StarException.hpp"

namespace Star {

STAR_STRUCT(SignalHandlerImpl);

// Singleton signal handler that registers handlers for segfault, fpe,
// illegal instructions etc as well as non-fatal interrupts.
class SignalHandler {
public:
  SignalHandler();
  ~SignalHandler();

  // If enabled, will catch segfault, fpe, and illegal instructions and output
  // error information before dying.
  void setHandleFatal(bool handleFatal);
  bool handlingFatal() const;

  // If enabled, non-fatal interrupt signal will be caught and will not kill
  // the process and will instead set the interrupted flag.
  void setHandleInterrupt(bool handleInterrupt);
  bool handlingInterrupt() const;

  bool interruptCaught() const;

private:
  friend SignalHandlerImpl;

  static SignalHandlerImplUPtr s_singleton;
};

}

#endif
