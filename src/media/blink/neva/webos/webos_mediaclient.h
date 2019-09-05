// Copyright 2018 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_
#define MEDIA_BLINK_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/neva/media_constants.h"
#include "media/base/neva/media_track_info.h"
#include "media/base/pipeline.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

class WebOSMediaClient {
 public:
  enum BufferingState {
    kHaveMetadata,
    kLoadCompleted,
    kPreloadCompleted,
    kPrerollCompleted,
    kWebOSBufferingStart,
    kWebOSBufferingEnd,
    kWebOSNetworkStateLoading,
    kWebOSNetworkStateLoaded
  };

  enum Preload {
    PreloadNone,
    PreloadMetaData,
    PreloadAuto,
  };

  // common callbacks
  typedef base::Callback<void(const std::string& detail)> UpdateUMSInfoCB;
  typedef base::Callback<void(WebOSMediaClient::BufferingState)>
      BufferingStateCB;
  typedef base::Callback<void(bool playing)> PlaybackStateCB;

  typedef base::Callback<void(const gfx::Rect&)> ActiveRegionCB;

  typedef base::Callback<void(
      const std::vector<MediaTrackInfo>& audio_track_info)>
      AddAudioTrackCB;
  typedef base::Callback<void(const std::string& id,
                              const std::string& kind,
                              const std::string& language,
                              bool enabled)>
      AddVideoTrackCB;
  typedef base::Callback<void(const std::string& type,
                              const std::vector<uint8_t>& init_data)>
      EncryptedCB;

  virtual ~WebOSMediaClient() {}

  static WebOSMediaClient* Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner);

  virtual void Load(bool video,
                    double current_time,
                    bool is_local_source,
                    const std::string& app_id,
                    const std::string& url,
                    const std::string& mime_type,
                    const std::string& referrer,
                    const std::string& user_agent,
                    const std::string& cookies,
                    const std::string& payload,
                    const PlaybackStateCB& playback_state_cb,
                    const base::Closure& ended_cb,
                    const media::PipelineStatusCB& seek_cb,
                    const media::PipelineStatusCB& error_cb,
                    const BufferingStateCB& buffering_state_cb,
                    const base::Closure& duration_change_cb,
                    const base::Closure& video_size_change_cb,
                    const base::Closure& video_display_window_change_cb,
                    const AddAudioTrackCB& add_audio_track_cb,
                    const AddVideoTrackCB& add_video_track_cb,
                    const UpdateUMSInfoCB& update_ums_info_cb,
                    const base::Closure& focus_cb,
                    const ActiveRegionCB& active_region_cb,
                    const base::Closure& waiting_for_decryption_key_cb,
                    const EncryptedCB& encrypted_cb) = 0;
  virtual void Seek(base::TimeDelta time,
                    const media::PipelineStatusCB& seek_cb) = 0;
  virtual float GetPlaybackRate() const = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual double GetPlaybackVolume() const = 0;
  virtual void SetPlaybackVolume(double volume, bool forced = false) = 0;
  virtual bool SelectTrack(const MediaTrackType type,
                           const std::string& id) = 0;
  virtual void Suspend(SuspendReason reason) = 0;
  virtual void Resume() = 0;
  virtual bool IsRecoverableOnResume() = 0;
  virtual void SetPreload(Preload preload) = 0;
  virtual bool IsPreloadable(const std::string& content_media_option) = 0;
  virtual std::string MediaId() = 0;

  virtual double GetDuration() const = 0;
  virtual void SetDuration(double duration) = 0;
  virtual double GetCurrentTime() = 0;
  virtual void SetCurrentTime(double time) = 0;

  virtual double BufferEnd() const = 0;
  virtual bool HasAudio() = 0;
  virtual bool HasVideo() = 0;
  virtual gfx::Size GetNaturalVideoSize() = 0;
  virtual void SetNaturalVideoSize(const gfx::Size& size) = 0;
  // outRect and inRect should not empty.
  virtual bool SetDisplayWindow(const gfx::Rect& outRect,
                                const gfx::Rect& inRect,
                                bool fullScreen,
                                bool forced = false) = 0;
  virtual void SetVisibility(bool visible) = 0;
  virtual bool Visibility() = 0;
  virtual void SetFocus() = 0;
  virtual bool Focus() = 0;
  virtual void SwitchToAutoLayout() = 0;
  virtual bool DidLoadingProgress() = 0;
  virtual bool UsesIntrinsicSize() const = 0;
  virtual void Unload() = 0;
  virtual bool IsSupportedBackwardTrickPlay() = 0;
  virtual bool IsSupportedPreload() = 0;
  virtual bool CheckUseMediaPlayerManager(const std::string& media_option) = 0;
  virtual void SetDisableAudio(bool) = 0;
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBOS_WEBOS_MEDIACLIENT_H_
