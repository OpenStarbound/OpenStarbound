#pragma once

#include "StarBaseScriptPane.hpp"

namespace Star {

STAR_CLASS(BindingsMenu);

class BindingsMenu : public BaseScriptPane {
public:
  BindingsMenu(Json const& config);

  virtual void show() override;
  void displayed() override;
  void dismissed() override;

private:

};

}
