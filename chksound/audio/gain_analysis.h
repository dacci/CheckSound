// Copyright (c) 2019 dacci.org

#ifndef CHKSOUND_AUDIO_GAIN_ANALYSIS_H_
#define CHKSOUND_AUDIO_GAIN_ANALYSIS_H_

#include <mutex>         // NOLINT(build/c++11)
#include <shared_mutex>  // NOLINT(build/include_order)

#ifdef _MSC_VER
#pragma warning(push)
// nonstandard extension used: zero-sized array in struct/union
#pragma warning(disable : 4200)
#endif

#include <lib1770.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace chksound::audio {

class GainAnalysis {
 public:
  GainAnalysis(double sampling_rate, int channels)
      : channels_{channels},
        stats_{lib1770_stats_new()},
        block_{lib1770_block_new(sampling_rate, 400, 4)},
        pre_{lib1770_pre_new_lfe(sampling_rate, channels, LIB1770_LFE)},
        peak_{} {
    lib1770_block_add_stats(block_, stats_);
    lib1770_pre_add_block(pre_, block_);
  }

  ~GainAnalysis() {
    lib1770_pre_close(pre_);
    lib1770_block_close(block_);
    lib1770_stats_close(stats_);
  }

  void Update(double* samples) {
    std::scoped_lock<std::shared_mutex> lock(mutex_);

    lib1770_pre_add_sample(pre_, samples);

    for (auto i = 0; i < channels_; ++i) {
      auto sample = fabs(samples[i]);
      if (peak_ < sample)
        peak_ = sample;
    }
  }

  double Loudness() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return lib1770_stats_get_mean(stats_, -10);
  }

  double Peak() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return peak_;
  }

 private:
  friend class GainAggregator;

  std::shared_mutex mutex_;

  const int channels_;
  lib1770_stats_t* const stats_;
  lib1770_block_t* const block_;
  lib1770_pre_t* const pre_;
  double peak_;

  GainAnalysis(const GainAnalysis&) = delete;
  GainAnalysis& operator=(const GainAnalysis&) = delete;
};

class GainAggregator {
 public:
  GainAggregator() : stats_{lib1770_stats_new()}, peak_{} {}

  ~GainAggregator() {
    lib1770_stats_close(stats_);
  }

  void Merge(const GainAnalysis* analysis) {
    std::scoped_lock<std::shared_mutex> lock(mutex_);

    lib1770_stats_merge(stats_, analysis->stats_);

    if (peak_ < analysis->peak_)
      peak_ = analysis->peak_;
  }

  double Loudness() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return lib1770_stats_get_mean(stats_, -10);
  }

  double Peak() {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    return peak_;
  }

 private:
  std::shared_mutex mutex_;

  lib1770_stats_t* const stats_;
  double peak_;

  GainAggregator(const GainAggregator&) = delete;
  GainAggregator& operator=(const GainAggregator&) = delete;
};

}  // namespace chksound::audio

#endif  // CHKSOUND_AUDIO_GAIN_ANALYSIS_H_
