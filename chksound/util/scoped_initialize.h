// Copyright (c) 2019 dacci.org

#ifndef CHKSOUND_UTIL_SCOPED_INITIALIZE_H_
#define CHKSOUND_UTIL_SCOPED_INITIALIZE_H_

#include <utility>

namespace chksound::util {

template <class Trait>
class ScopedInitialize {
 public:
  template <class... Args>
  explicit ScopedInitialize(Args&&... args)
      : succeeded_{Trait::Initialize(std::forward<Args>(args)...)} {}

  ~ScopedInitialize() {
    if (succeeded_)
      Trait::Uninitialize();
  }

 private:
  const bool succeeded_;

  ScopedInitialize(const ScopedInitialize&) = delete;
  ScopedInitialize& operator=(const ScopedInitialize&) = delete;
};

}  // namespace chksound::util

#endif  // CHKSOUND_UTIL_SCOPED_INITIALIZE_H_
