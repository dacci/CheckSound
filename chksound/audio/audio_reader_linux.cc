// Copyright (c) 2019 dacci.org

#include "audio/audio_reader.h"

#include <mpg123.h>

namespace fs = std::filesystem;

namespace chksound::audio {
namespace {

class Mpg123AudioReader : public AudioReader {
 public:
  explicit Mpg123AudioReader(const std::filesystem::path& path)
      : handle_{}, cursor_{}, limit_{} {
    int err;
    auto handle = mpg123_new(nullptr, &err);
    if (handle == nullptr)
      return;

    do {
      err = mpg123_open(handle, path.c_str());
      if (err != MPG123_OK)
        break;

      err = mpg123_getformat(handle, &rate_, &channels_, &encoding_);
      if (err != MPG123_OK)
        break;

      if ((encoding_ & MPG123_ENC_FLOAT) != 0)
        break;

      if ((encoding_ & MPG123_ENC_SIGNED) == 0)
        break;

      if (encoding_ & MPG123_ENC_8)
        bits_ = 8;
      else if (encoding_ & MPG123_ENC_16)
        bits_ = 16;
      else if (encoding_ & MPG123_ENC_24)
        bits_ = 24;
#if 0
      else if (encoding_ & MPG123_ENC_32)
        bits_ = 32;
#endif
      else
        break;

      range_ = 1 << (bits_ - 1);

      handle_ = handle;
      return;
    } while (false);

    mpg123_delete(handle);
  }

  ~Mpg123AudioReader() override {
    if (handle_ != nullptr) {
      mpg123_delete(handle_);
      handle_ = nullptr;
    }
  }

  bool Read(double* buffer) override {
    if (cursor_ == limit_) {
      off_t offset;
      size_t bytes;
      auto err = mpg123_decode_frame(handle_, &offset, &cursor_, &bytes);
      if (err != MPG123_OK)
        return false;

      limit_ = cursor_ + bytes;
    }

    while (cursor_ < limit_) {
      for (auto channel = 0; channel < channels_; ++channel) {
        auto data = cursor_[bits_ / 8 - 1] & 0x80 ? 0xFFFFFFFF << bits_ : 0;
        for (auto b = 0; b < bits_; b += 8) {
          data |= *cursor_++ << b;
        }

        buffer[channel] = data / range_;
      }

      return true;
    }

    return false;
  }

  double GetSamplingRate() const override {
    return rate_;
  }

  int GetChannels() const override {
    return channels_;
  }

  bool valid() const {
    return handle_ != nullptr;
  }

 private:
  mpg123_handle* handle_;
  long rate_;  // NOLINT(runtime/int)
  int channels_;
  int encoding_;
  int bits_;
  double range_;

  unsigned char* cursor_;
  unsigned char* limit_;

  Mpg123AudioReader(const Mpg123AudioReader&) = delete;
  Mpg123AudioReader& operator=(const Mpg123AudioReader&) = delete;
};

}  // namespace

std::unique_ptr<AudioReader> OpenAudio(const std::filesystem::path& path) {
  auto reader = std::make_unique<Mpg123AudioReader>(path);
  if (reader == nullptr || !reader->valid())
    return nullptr;

  return reader;
}

}  // namespace chksound::audio
