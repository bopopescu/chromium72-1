// Copyright 2018-2019 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H
#define MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H

#include <queue>
#include <set>
#include "base/optional.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/neva/media_constants.h"
#include "media/base/neva/media_type_restriction.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class MEDIA_EXPORT MediaPlatformAPI
    : public base::RefCountedThreadSafe<media::MediaPlatformAPI> {
 public:
  using ActiveRegionCB = base::Callback<void(const blink::WebRect&)>;
  using NaturalVideoSizeChangedCB = base::Callback<void(const gfx::Size&)>;
  enum FeedType {
    Video = 1,
    Audio,
  };

  using LoadCompletedCB = base::Callback<void()>;

  enum RestorePlaybackMode { RESTORE_PAUSED, RESTORE_PLAYING };

  MediaPlatformAPI();

  static scoped_refptr<MediaPlatformAPI> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      bool video,
      const std::string& app_id,
      const NaturalVideoSizeChangedCB& natural_video_size_changed_cb,
      const base::Closure& resume_done_cb,
      const base::Closure& suspend_done_cb,
      const ActiveRegionCB& active_region_cb,
      const PipelineStatusCB& error_cb);

  // Return true if the implementation of MediaPlatformAPI is available to play
  // media.
  static bool IsAvailable();

  virtual void Initialize(const AudioDecoderConfig& audio_config,
                          const VideoDecoderConfig& video_config,
                          const PipelineStatusCB& init_cb) = 0;

  virtual void ReInitializeIfNeeded() = 0;
  // rect and in_rect should not empty.
  virtual void SetDisplayWindow(const gfx::Rect& rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) = 0;
  virtual void SetLoadCompletedCb(const LoadCompletedCB& loaded_cb) = 0;
  virtual bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
                    FeedType type) = 0;
  virtual bool Seek(base::TimeDelta time) = 0;
  virtual void Suspend(SuspendReason reason) = 0;
  virtual void Resume(base::TimeDelta paused_time,
                      RestorePlaybackMode restore_playback_mode) = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual void SetPlaybackVolume(double volume) = 0;
  virtual bool AllowedFeedVideo() = 0;
  virtual bool AllowedFeedAudio() = 0;
  virtual void Finalize() = 0;
  virtual void SetKeySystem(const std::string& key_system) = 0;
  virtual bool IsEOSReceived() = 0;
  virtual void UpdateVideoConfig(const VideoDecoderConfig& video_config) {}

  virtual void SetVisibility(bool visible) = 0;

  virtual void SwitchToAutoLayout() = 0;
  virtual void SetDisableAudio(bool disable) = 0;

  static base::Optional<MediaTypeRestriction> GetPlatformRestrictionForType(
      const std::string& type);
  static void SetMediaCodecCapability(const std::string& codec_info);
  static Json::Value SupportedCodec() { return supported_codec_; }

 protected:
  virtual ~MediaPlatformAPI();

  class BufferQueue {
   public:
    BufferQueue();
    ~BufferQueue();
    void Push(const scoped_refptr<DecoderBuffer>& buffer, FeedType type);
    const std::pair<scoped_refptr<DecoderBuffer>, FeedType> Front();
    void Pop();
    void Clear();

    bool Empty();
    size_t DataSize() const;

   private:
    using DecoderBufferPair =
        std::pair<scoped_refptr<DecoderBuffer>, MediaPlatformAPI::FeedType>;
    using DecoderBufferQueue = std::queue<DecoderBufferPair>;
    DecoderBufferQueue queue_;
    size_t data_size_;

    DISALLOW_COPY_AND_ASSIGN(BufferQueue);
  };

  std::unique_ptr<BufferQueue> buffer_queue_;

 private:
  friend class base::RefCountedThreadSafe<MediaPlatformAPI>;

  static Json::Value& supported_codec_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlatformAPI);
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H
