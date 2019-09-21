// Copyright (c) 2019 dacci.org

#include "util/scoped_initialize.h"

#include <mpg123.h>

namespace chksound::util {
namespace {

struct Mpg123InitializeTrait {
  static bool Initialize() {
    return mpg123_init() == MPG123_OK;
  }

  static void Uninitialize() {
    mpg123_exit();
  }
};

ScopedInitialize<Mpg123InitializeTrait> mpg123_initialize;

}  // namespace
}  // namespace chksound::util
