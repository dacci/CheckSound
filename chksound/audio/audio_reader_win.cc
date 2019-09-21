// Copyright (c) 2019 dacci.org

#include "audio/audio_reader.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <wrl/client.h>

namespace chksound::audio {
namespace {

namespace fs = ::std::filesystem;
using ::Microsoft::WRL::ComPtr;

class WindowsAudioReader : public AudioReader {
 public:
  explicit WindowsAudioReader(const fs::path& path)
      : channels_{}, sampling_rate_{}, bits_{}, range_{}, cursor_{}, limit_{} {
    auto result = MFCreateSourceReaderFromURL(path.c_str(), nullptr,
                                              reader_.GetAddressOf());
    if (FAILED(result))
      return;

    do {
      ComPtr<IMFMediaType> media_type;
      result = MFCreateMediaType(media_type.GetAddressOf());
      if (FAILED(result))
        break;

      result = media_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
      if (FAILED(result))
        break;

      result = media_type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
      if (FAILED(result))
        break;

      result = reader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                            nullptr, media_type.Get());
      if (FAILED(result))
        break;

      result =
          reader_->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                       media_type.ReleaseAndGetAddressOf());
      if (FAILED(result))
        break;

      result = media_type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels_);
      if (FAILED(result))
        break;

      result = media_type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,
                                     &sampling_rate_);
      if (FAILED(result))
        break;

      result = media_type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits_);
      if (FAILED(result))
        break;
      if (24 < bits_)
        break;
      if (bits_ & 7)  // assert multiple of 8
        break;

      range_ = 1 << (bits_ - 1);

      return;
    } while (false);

    reader_->Release();
  }

  bool Read(double* buffer) override {
    while (cursor_ == limit_) {
      DWORD flags;
      auto result = reader_->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
                                        nullptr, &flags, nullptr,
                                        sample_.ReleaseAndGetAddressOf());
      if (FAILED(result))
        return false;
      if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        return false;
      if (sample_ == nullptr)
        continue;

      result =
          sample_->ConvertToContiguousBuffer(buffer_.ReleaseAndGetAddressOf());
      if (FAILED(result))
        return false;

      DWORD length;
      result = buffer_->Lock(&cursor_, nullptr, &length);
      if (FAILED(result))
        return false;

      limit_ = cursor_ + length;
      break;
    }

    while (cursor_ < limit_) {
      for (UINT32 channel = 0; channel < channels_; ++channel) {
        int data = cursor_[bits_ / 8 - 1] & 0x80 ? 0xFFFFFFFF << bits_ : 0;
        for (UINT32 b = 0; b < bits_; b += 8) {
          data |= *cursor_++ << b;
        }

        buffer[channel] = data / range_;
      }

      return true;
    }

    return false;
  }

  double GetSamplingRate() const override {
    return sampling_rate_;
  }

  int GetChannels() const override {
    return channels_;
  }

  bool valid() const {
    return reader_ != nullptr;
  }

 private:
  ComPtr<IMFSourceReader> reader_;
  UINT32 channels_;
  UINT32 sampling_rate_;
  UINT32 bits_;
  double range_;

  ComPtr<IMFSample> sample_;
  ComPtr<IMFMediaBuffer> buffer_;
  BYTE* cursor_;
  BYTE* limit_;

  WindowsAudioReader(const WindowsAudioReader&) = delete;
  WindowsAudioReader& operator=(const WindowsAudioReader&) = delete;
};

}  // namespace

std::unique_ptr<AudioReader> OpenAudio(const fs::path& path) {
  auto reader = std::make_unique<WindowsAudioReader>(path);
  if (!reader->valid())
    return nullptr;

  return reader;
}

}  // namespace chksound::audio
