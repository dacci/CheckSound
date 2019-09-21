// Copyright (c) 2019 dacci.org

#include "app/analyzer.h"

#include <taglib/commentsframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <thread>  // NOLINT(build/c++11)
#include <utility>

#include "audio/audio_reader.h"
#include "audio/gain_analysis.h"

#ifdef _MSC_VER
#define wcscasecmp _wcsicmp
#endif

namespace fs = ::std::filesystem;

namespace chksound::app {
namespace {

const fs::path kMP3 = ".mp3";
const fs::path kM4A = ".m4a";
const TagLib::ByteVector kTCMP = "TCMP";
const TagLib::ByteVector kCPIL = "cpil";

int GetAdjustment(double gain, double base) {
  return static_cast<int>(
      std::min(round(pow(10.0, -gain / 10.0) * base), 65534.0));
}

}  // namespace

struct Analyzer::Entry {
  explicit Entry(const fs::path& path) : path{path} {}

  fs::path path;
  std::shared_ptr<audio::GainAggregator> aggregator;
  std::unique_ptr<audio::GainAnalysis> analysis;
};

Analyzer::~Analyzer() = default;

std::unique_ptr<Analyzer> Analyzer::CreateInstance() {
  struct Bridge : Analyzer {};
  return std::make_unique<Bridge>();
}

void Analyzer::Add(const fs::path& path) {
  if (!fs::exists(path))
    return;

  if (fs::is_directory(path)) {
    for (auto& child : fs::recursive_directory_iterator(path))
      AddFile(child);
  } else {
    AddFile(path);
  }
}

void Analyzer::Analyze() {
  std::mutex mutex;
  auto current = entries_.begin();
  auto end = entries_.end();

  auto task = [this, &mutex, &current, &end]() {
    while (true) {
      mutex.lock();
      if (current == end) {
        mutex.unlock();
        return;
      }
      auto& entry = *current;
      ++current;
      mutex.unlock();

      Analyze(&entry);
    }
  };

  std::vector<std::thread> threads;
  for (auto i = std::thread::hardware_concurrency(); i > 0; --i)
    threads.emplace_back(task);

  for (auto& thread : threads)
    thread.join();
}

void Analyzer::Commit() {
  for (auto& entry : entries_) {
    if (entry.analysis == nullptr)
      continue;

    auto track_gain = -18.0 - entry.analysis->Loudness();
    auto track_peak = static_cast<int>(entry.analysis->Peak() * 32768);

    double album_gain;
    int album_peak;
    if (entry.aggregator != nullptr) {
      album_gain = -18.0 - entry.aggregator->Loudness();
      album_peak = static_cast<int>(entry.aggregator->Peak() * 32768);
    } else {
      album_gain = track_gain;
      album_peak = track_peak;
    }

    int values[] = {
        GetAdjustment(track_gain, 1000),
        GetAdjustment(album_gain, 1000),
        GetAdjustment(track_gain, 2500),
        GetAdjustment(album_gain, 2500),
        0,
        0,
        track_peak,
        album_peak,
        0,
        0,
    };

    std::stringstream buffer;
    for (auto value : values)
      buffer << " " << std::uppercase << std::setfill('0') << std::setw(8)
             << std::hex << value;

    auto extension = entry.path.extension();
    if (extension == kMP3) {
      TagLib::MPEG::File file(entry.path.c_str(), false);
      if (file.isValid())
        Commit(buffer.str(), &file);
    } else if (extension == kM4A) {
      TagLib::MP4::File file(entry.path.c_str(), false);
      if (file.isValid())
        Commit(buffer.str(), &file);
    }
  }
}

Analyzer::Analyzer() = default;

bool Analyzer::AddFile(const fs::path& path) {
  if (added_.find(path) != added_.end())
    return false;

  auto extension = path.extension();
  if (extension == kMP3) {
    TagLib::MPEG::File file(path.c_str(), false);
    if (file.isValid()) {
      added_.emplace(path);
      AddFile(&file, &entries_.emplace_back(path));
      return true;
    }
  } else if (extension == kM4A) {
    TagLib::MP4::File file(path.c_str(), false);
    if (file.isValid()) {
      added_.emplace(path);
      AddFile(&file, &entries_.emplace_back(path));
      return true;
    }
  }

  return false;
}

void Analyzer::AddFile(TagLib::MPEG::File* file, Entry* entry) {
  if (!file->hasID3v2Tag())
    return;

  auto tag = file->ID3v2Tag();

  auto compilation = false;
  auto& tcmp = tag->frameList(kTCMP);
  if (!tcmp.isEmpty()) {
    bool ok;
    auto value = tcmp[0]->toString().toInt(&ok);
    if (ok)
      compilation = value != 0;
  }

  if (!compilation) {
    auto artist = tag->artist();
    auto album = tag->album();
    auto group_key = artist.to8Bit(true) + '\0' + album.to8Bit(true);
    entry->aggregator = GetAggregator(group_key);
  }
}

void Analyzer::AddFile(TagLib::MP4::File* file, Entry* entry) {
  if (!file->hasMP4Tag())
    return;

  auto tag = file->tag();

  auto cpil = tag->item(kCPIL);
  if (cpil.isValid() && cpil.toBool()) {
    auto artist = tag->artist();
    auto album = tag->album();
    auto group_key = artist.to8Bit(true) + '\0' + album.to8Bit(true);
    entry->aggregator = GetAggregator(group_key);
  }
}

std::shared_ptr<audio::GainAggregator> Analyzer::GetAggregator(
    const std::string& key) {
  auto& aggregator = aggregators_[key];
  if (aggregator == nullptr)
    aggregator = std::make_shared<audio::GainAggregator>();

  return aggregator;
}

void Analyzer::Analyze(Entry* entry) {
  auto reader = chksound::audio::OpenAudio(entry->path);
  if (reader == nullptr)
    return;

  auto analysis = std::make_unique<chksound::audio::GainAnalysis>(
      reader->GetSamplingRate(), reader->GetChannels());
  if (analysis == nullptr)
    return;

  for (double samples[8]; reader->Read(samples);)
    analysis->Update(samples);

  entry->analysis = std::move(analysis);

  if (entry->aggregator != nullptr)
    entry->aggregator->Merge(entry->analysis.get());
}

void Analyzer::Commit(const std::string& normalization,
                      TagLib::MPEG::File* file) {
  auto tag = file->ID3v2Tag(true);
  TagLib::ID3v2::CommentsFrame* comment = nullptr;
  for (auto f : tag->frameList("COMM")) {
    auto frame = static_cast<TagLib::ID3v2::CommentsFrame*>(f);
    if (wcscasecmp(frame->description().toCWString(), L"iTunNORM") == 0) {
      comment = frame;
      break;
    }
  }

  if (comment == nullptr) {
    comment = new TagLib::ID3v2::CommentsFrame();
    comment->setLanguage("eng");
    comment->setDescription("iTunNORM");
    comment->setText(normalization);
    tag->addFrame(comment);
  } else {
    comment->setText(normalization);
  }

  if (!file->save(TagLib::MPEG::File::ID3v2))
    std::cerr << "failed to save" << std::endl;
}

void Analyzer::Commit(const std::string& normalization,
                      TagLib::MP4::File* file) {
  TagLib::StringList list;
  list.append(normalization);

  TagLib::MP4::Item item(list);

  auto tag = file->tag();
  tag->setItem("----:com.apple.iTunes:iTunNORM", item);

  if (!file->save())
    std::cerr << "failed to save" << std::endl;
}

}  // namespace chksound::app
