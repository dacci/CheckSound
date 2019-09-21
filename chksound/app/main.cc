// Copyright (c) 2019 dacci.org

#include "app/analyzer.h"

#ifdef _UNICODE
int wmain(int argc, const wchar_t* const* argv) {
#else
int main(int argc, const char* const* argv) {
#endif
  auto analyzer = chksound::app::Analyzer::CreateInstance();

  for (auto i = 1; i < argc; ++i)
    analyzer->Add(argv[i]);

  analyzer->Analyze();
  analyzer->Commit();

  return 0;
}
