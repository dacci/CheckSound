// Copyright (c) 2019 dacci.org

#ifndef CHKSOUND_AUDIO_AUDIO_READER_H_
#define CHKSOUND_AUDIO_AUDIO_READER_H_

#include <filesystem>
#include <memory>

namespace chksound::audio {

class AudioReader {
 public:
  virtual ~AudioReader() {}

  virtual bool Read(double* buffer) = 0;

  virtual double GetSamplingRate() const = 0;
  virtual int GetChannels() const = 0;
};

std::unique_ptr<AudioReader> OpenAudio(const std::filesystem::path& path);

}  // namespace chksound::audio

#endif  // CHKSOUND_AUDIO_AUDIO_READER_H_
