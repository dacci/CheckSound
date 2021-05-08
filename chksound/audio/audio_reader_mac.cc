// Copyright (c) 2020 dacci.org

#include <AudioToolbox/ExtendedAudioFile.h>

#include <memory>

#include "audio/audio_reader.h"

namespace chksound::audio {
namespace {

namespace fs = ::std::filesystem;

class MacAudioReader : public AudioReader {
 public:
  explicit MacAudioReader(const fs::path& path) : file_{}, format_{} {
    auto path_string = CFStringCreateWithCString(
        kCFAllocatorDefault, path.c_str(), kCFStringEncodingUTF8);
    if (!path_string)
      return;

    auto path_url = CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, path_string, kCFURLPOSIXPathStyle, false);
    CFRelease(path_string);
    if (!path_url)
      return;

    ExtAudioFileRef new_file;
    auto err = ExtAudioFileOpenURL(path_url, &new_file);
    CFRelease(path_url);
    if (err)
      return;

    do {
      uint32_t size = sizeof(format_);
      err = ExtAudioFileGetProperty(
          new_file, kExtAudioFileProperty_FileDataFormat, &size, &format_);
      if (err)
        break;

      frames_per_packet_ = format_.mFramesPerPacket;

      format_.mFormatID = kAudioFormatLinearPCM;
      format_.mFormatFlags =
          kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsSignedInteger;
      format_.mFramesPerPacket = 1;
      format_.mBitsPerChannel = 32;
      format_.mBytesPerFrame =
          format_.mBitsPerChannel / 8 * format_.mChannelsPerFrame;
      format_.mBytesPerPacket =
          format_.mFramesPerPacket * format_.mBytesPerFrame;
      err = ExtAudioFileSetProperty(
          new_file, kExtAudioFileProperty_ClientDataFormat, size, &format_);
      if (err)
        break;

      file_ = new_file;
      return;
    } while (false);

    ExtAudioFileDispose(new_file);
  }

  virtual ~MacAudioReader() {
    if (file_) {
      ExtAudioFileDispose(file_);
      file_ = nullptr;
    }
  }

  bool Read(double* buffer) override {
    if (!buffer_) {
      buffer_ = std::make_unique<int32_t[]>(format_.mChannelsPerFrame *
                                            frames_per_packet_);
      cursor_ = buffer_.get();
      limit_ = cursor_;
    }

    if (cursor_ == limit_) {
      uint32_t frames = frames_per_packet_;
      AudioBufferList buffers{1};
      buffers.mBuffers[0].mNumberChannels = format_.mChannelsPerFrame;
      buffers.mBuffers[0].mDataByteSize = format_.mBytesPerFrame * frames;
      buffers.mBuffers[0].mData = buffer_.get();
      auto err = ExtAudioFileRead(file_, &frames, &buffers);
      if (err)
        return false;

      cursor_ = buffer_.get();
      limit_ = cursor_ + format_.mChannelsPerFrame * frames;
    }

    while (cursor_ < limit_) {
      for (auto channel = 0U; channel < format_.mChannelsPerFrame; ++channel) {
        buffer[channel] = *(cursor_++) / kRange;
      }

      return true;
    }

    return false;
  }

  double GetSamplingRate() const override {
    return format_.mSampleRate;
  }

  int GetChannels() const override {
    return format_.mChannelsPerFrame;
  }

  bool valid() const {
    return file_ != nullptr;
  }

 private:
  static constexpr double kRange = 0x7FFFFFFF;

  ExtAudioFileRef file_;
  AudioStreamBasicDescription format_;

  uint32_t frames_per_packet_;
  std::unique_ptr<int32_t[]> buffer_;
  int32_t* cursor_;
  int32_t* limit_;

  MacAudioReader(const MacAudioReader&) = delete;
  MacAudioReader& operator=(const MacAudioReader&) = delete;
};

}  // namespace

std::unique_ptr<AudioReader> OpenAudio(const fs::path& path) {
  auto reader = std::make_unique<MacAudioReader>(path);
  if (!reader->valid())
    return nullptr;

  return reader;
}

}  // namespace chksound::audio
