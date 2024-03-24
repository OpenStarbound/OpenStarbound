#pragma once

#include "StarAssets.hpp"

namespace Star {

STAR_CLASS(Configuration);

STAR_EXCEPTION(RootException, StarException);

class RootBase {
public:
  static RootBase* singletonPtr();
  static RootBase& singleton();

  virtual AssetsConstPtr assets() = 0;
  virtual ConfigurationPtr configuration() = 0;
protected:
  RootBase();

  static atomic<RootBase*> s_singleton;
};

}