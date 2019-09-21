// Copyright (c) 2019 dacci.org

#include <objbase.h>

#include <mfapi.h>

#include "util/scoped_initialize.h"

namespace chksound::util {
namespace {

struct CoInitializeTrait {
  static bool Initialize(DWORD flags) {
    return SUCCEEDED(CoInitializeEx(nullptr, flags));
  }

  static void Uninitialize() {
    CoUninitialize();
  }
};

ScopedInitialize<CoInitializeTrait> co_initialize(COINIT_APARTMENTTHREADED |
                                                  COINIT_DISABLE_OLE1DDE);

struct MediaFoundationTrait {
  static bool Initialize(ULONG version, DWORD flags) {
    return SUCCEEDED(MFStartup(version, flags));
  }

  static void Uninitialize() {
    MFShutdown();
  }
};

ScopedInitialize<MediaFoundationTrait> media_foundation_initialize(
    MF_VERSION, MFSTARTUP_LITE);

}  // namespace
}  // namespace chksound::util
