// Copyright (c) 2019 dacci.org

#ifndef CHKSOUND_APP_ANALYZER_H_
#define CHKSOUND_APP_ANALYZER_H_

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace TagLib {
namespace MPEG {

class File;

}  // namespace MPEG

namespace MP4 {

class File;

}  // namespace MP4
}  // namespace TagLib

namespace chksound {
namespace audio {

class GainAggregator;

}  // namespace audio

namespace app {

class Analyzer {
 public:
  ~Analyzer();

  static std::unique_ptr<Analyzer> CreateInstance();

  void Add(const std::filesystem::path& path);
  void Analyze();
  void Commit();

 private:
  struct Entry;

  Analyzer();

  bool AddFile(const std::filesystem::path& path);
  void AddFile(TagLib::MPEG::File* file, Entry* entry);
  void AddFile(TagLib::MP4::File* file, Entry* entry);

  void Analyze(Entry* entry);

  void Commit(const std::string& normalization, TagLib::MPEG::File* file);
  void Commit(const std::string& normalization, TagLib::MP4::File* file);

  std::shared_ptr<audio::GainAggregator> GetAggregator(const std::string& key);

  std::set<std::filesystem::path> added_;
  std::map<std::string, std::shared_ptr<audio::GainAggregator>> aggregators_;
  std::vector<Entry> entries_;

  Analyzer(const Analyzer&) = delete;
  Analyzer& operator=(const Analyzer&) = delete;
};

}  // namespace app
}  // namespace chksound

#endif  // CHKSOUND_APP_ANALYZER_H_
