// Copyright 2015-2019 LG Electronics, Inc.
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

#include "media/blink/neva/webmediaplayer_mse.h"

#include "base/command_line.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/layers/layer.h"
#include "cc/layers/video_layer.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/neva/media_constants.h"
#include "media/base/neva/media_platform_api.h"
#include "media/base/renderer_factory_selector.h"
#include "media/blink/neva/video_util_neva.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "ui/gfx/geometry/rect_f.h"

#define FUNC_LOG(x) DVLOG(x) << __func__
#define THIS_FUNC_LOG(x) DVLOG(x) << "[" << this << "] " << __func__

namespace media {
namespace {

static const int64_t kMinVideoHoleUpdateInterval = 100;

}  // namespace

#define BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(function) \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()),    \
   BindToCurrentLoop(                                      \
       base::Bind(function, (this->video_frame_provider_->AsWeakPtr()))))

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, base::AsWeakPtr(this))))

WebMediaPlayerMSE::WebMediaPlayerMSE(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    media::WebMediaPlayerDelegate* delegate,
    std::unique_ptr<media::RendererFactorySelector> renderer_factory_selector,
    UrlIndex* url_index,
    std::unique_ptr<VideoFrameCompositor> compositor,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params,
    std::unique_ptr<WebMediaPlayerParamsNeva> params_neva)
    : media::WebMediaPlayerImpl(frame,
                                client,
                                encrypted_client,
                                delegate,
                                std::move(renderer_factory_selector),
                                url_index,
                                std::move(compositor),
                                std::move(params)),
      additional_contents_scale_(params_neva->additional_contents_scale()),
      app_id_(params_neva->application_id().Utf8()),
      is_suspended_(false),
      is_video_offscreen_(false),
      is_fullscreen_(false),
      is_fullscreen_mode_(false),
      is_loading_(false),
      render_mode_(blink::WebMediaPlayer::RenderModeNone) {
  THIS_FUNC_LOG(1);
  // Use the null sink for our MSE player
  audio_source_provider_ = new media::WebAudioSourceProviderImpl(
      new media::NullAudioSink(media_task_runner_), media_log_.get());

  video_frame_provider_ = std::make_unique<VideoFrameProviderImpl>(
      stream_texture_factory_create_cb, vfc_task_runner_);
  video_frame_provider_->SetWebLocalFrame(frame);
  video_frame_provider_->SetWebMediaPlayerClient(client);

  // Create MediaAPIs Wrapper
  media_platform_api_ = media::MediaPlatformAPI::Create(
      main_task_runner_, media_task_runner_, client_->IsVideo(), app_id_,
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnNaturalVideoSizeChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnResumed),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnSuspended),
      BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
          &VideoFrameProviderImpl::ActiveRegionChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnError));

  base::Optional<bool> is_audio_disabled = client_->IsAudioDisabled();
  if (is_audio_disabled.has_value())
    SetDisableAudio(*is_audio_disabled);

  renderer_factory_selector_->GetCurrentFactory()->SetMediaPlatformAPI(
      media_platform_api_);
  pipeline_controller_.SetMediaPlatformAPI(media_platform_api_);

  SetRenderMode(client_->RenderMode());

  delegate_->DidMediaCreated(delegate_id_,
                             !params_neva->use_unlimited_media_policy());
}

WebMediaPlayerMSE::~WebMediaPlayerMSE() {
  THIS_FUNC_LOG(1);
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (video_layer_)
    video_layer_->StopUsingProvider();

  vfc_task_runner_->DeleteSoon(FROM_HERE, std::move(video_frame_provider_));

  if (throttleUpdateVideoHoleBoundary_.IsRunning())
    throttleUpdateVideoHoleBoundary_.Stop();

  if (media_platform_api_.get())
    media_platform_api_->Finalize();
}

// static
bool WebMediaPlayerMSE::IsAvailable() {
  return MediaPlatformAPI::IsAvailable();
}

blink::WebMediaPlayer::LoadTiming WebMediaPlayerMSE::Load(
    LoadType load_type,
    const blink::WebMediaPlayerSource& source,
    CorsMode cors_mode) {
  DCHECK(source.IsURL());
  THIS_FUNC_LOG(1);

  is_loading_ = true;
  pending_load_type_ = load_type;
  pending_source_ = blink::WebMediaPlayerSource(source.GetAsURL());
  pending_cors_mode_ = cors_mode;

  delegate_->DidMediaActivationNeeded(delegate_id_);

  return LoadTiming::kDeferred;
}

void WebMediaPlayerMSE::Play() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  THIS_FUNC_LOG(1);
  if (!has_activation_permit_) {
    status_on_suspended_ = PlayingStatus;
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }
  media::WebMediaPlayerImpl::Play();
}

void WebMediaPlayerMSE::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  THIS_FUNC_LOG(1);
  if (is_suspended_) {
    status_on_suspended_ = PausedStatus;
    return;
  }
  media::WebMediaPlayerImpl::Pause();
}

void WebMediaPlayerMSE::SetRate(double rate) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!has_activation_permit_) {
    if (!client_->IsSuppressedMediaPlay())
      delegate_->DidMediaActivationNeeded(delegate_id_);
    return;
  }
  media::WebMediaPlayerImpl::SetRate(rate);
}

void WebMediaPlayerMSE::SetVolume(double volume) {
  THIS_FUNC_LOG(1);
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  media::WebMediaPlayerImpl::SetVolume(volume);
}

void WebMediaPlayerMSE::SetContentDecryptionModule(
    blink::WebContentDecryptionModule* cdm,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // call base-class implementation
  media::WebMediaPlayerImpl::SetContentDecryptionModule(cdm, result);
}

void WebMediaPlayerMSE::EnteredFullscreen() {
  WebMediaPlayerImpl::EnteredFullscreen();
  if (!is_fullscreen_mode_) {
    is_fullscreen_mode_ = true;
    UpdateVideoHoleBoundary(true);
  }
}

void WebMediaPlayerMSE::ExitedFullscreen() {
  WebMediaPlayerImpl::ExitedFullscreen();
  if (is_fullscreen_mode_) {
    is_fullscreen_mode_ = false;
    UpdateVideoHoleBoundary(true);
  }
}

void WebMediaPlayerMSE::OnSuspend() {
  THIS_FUNC_LOG(1) << " is_suspended_ was " << is_suspended_;
  if (is_suspended_) {
    delegate_->DidMediaSuspended(delegate_id_);
    return;
  }

  status_on_suspended_ = Paused() ? PausedStatus : PlayingStatus;

  if (status_on_suspended_ == PlayingStatus) {
    Pause();
    client_->RequestPause();
  }

  if (media_platform_api_.get()) {
    SuspendReason reason = client_->IsSuppressedMediaPlay()
                               ? SuspendReason::BACKGROUNDED
                               : SuspendReason::SUSPENDED_BY_POLICY;
    media_platform_api_->Suspend(reason);
  }

  is_suspended_ = true;
  has_activation_permit_ = false;

  // TODO(neva): also need to set STORAGE_BLACK for VIDEO_HOLE ?
  if (HasVideo() && RenderTexture())
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_BLACK);

  // Usually we wait until OnSuspended(), but send DidMediaSuspended()
  // immediately when media_platform_api_ is null.
  if (!media_platform_api_.get())
    delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerMSE::OnSuspended() {
  LOG(INFO) << __func__;
  delegate_->DidMediaSuspended(delegate_id_);
}

void WebMediaPlayerMSE::OnMediaActivationPermitted() {
  // If we already have activation permit, just skip.
  if (has_activation_permit_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  has_activation_permit_ = true;

  if (is_loading_) {
    OnLoadPermitted();
    return;
  } else if (is_suspended_) {
    OnResume();
    return;
  }

  Play();
  client_->RequestPlay();
  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerMSE::OnResume() {
  THIS_FUNC_LOG(1) << " is_suspended_ was " << is_suspended_;
  if (!is_suspended_) {
    delegate_->DidMediaActivated(delegate_id_);
    return;
  }

  is_suspended_ = false;

  media::MediaPlatformAPI::RestorePlaybackMode restore_playback_mode;

  restore_playback_mode = (status_on_suspended_ == PausedStatus)
                              ? media::MediaPlatformAPI::RESTORE_PAUSED
                              : media::MediaPlatformAPI::RESTORE_PLAYING;

  if (media_platform_api_.get())
    media_platform_api_->Resume(paused_time_, restore_playback_mode);
  else {
    // Usually we wait until OnResumed(), but send DidMediaActivated()
    // immediately when media_platform_api_ is null.
    delegate_->DidMediaActivated(delegate_id_);
  }
}

void WebMediaPlayerMSE::OnResumed() {
  LOG(INFO) << __func__;
#if defined(VIDEO_HOLE)
  UpdateVideoHoleBoundary(true);
#endif

  client_->RequestSeek(paused_time_.InSecondsF());

  if (status_on_suspended_ == PausedStatus) {
    Pause();
    status_on_suspended_ = UnknownStatus;
  } else {
    Play();
    client_->RequestPlay();
  }

  if (HasVideo() && RenderTexture())
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);

  delegate_->DidMediaActivated(delegate_id_);
}

void WebMediaPlayerMSE::OnLoadPermitted() {
  // call base-class implementation
  media::WebMediaPlayerImpl::Load(pending_load_type_, pending_source_,
                                  pending_cors_mode_);
}

void WebMediaPlayerMSE::OnNaturalVideoSizeChanged(
    const gfx::Size& natural_video_size) {
  LOG(INFO) << __func__ << " width: " << natural_video_size.width()
            << " , height: " << natural_video_size.height();
  natural_video_size_ = natural_video_size;
  UpdateVideoHoleBoundary(true);
}

void WebMediaPlayerMSE::OnError(PipelineStatus metadata) {
  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }
  media::WebMediaPlayerImpl::OnError(metadata);
}

void WebMediaPlayerMSE::OnMetadata(const PipelineMetadata& metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (is_loading_) {
    is_loading_ = false;
    delegate_->DidMediaActivated(delegate_id_);
  }

  pipeline_metadata_ = metadata;

  UMA_HISTOGRAM_ENUMERATION("Media.VideoRotation",
                            metadata.video_decoder_config.video_rotation(),
                            VIDEO_ROTATION_MAX + 1);
  SetReadyState(WebMediaPlayer::kReadyStateHaveMetadata);

  if (HasVideo()) {
    // TODO(neva): In here, we don't use natural size from platform api.
    // We need to ensure that it is really fine.

    if (pipeline_metadata_.video_decoder_config.video_rotation() ==
            VIDEO_ROTATION_90 ||
        pipeline_metadata_.video_decoder_config.video_rotation() ==
            VIDEO_ROTATION_270) {
      gfx::Size size = pipeline_metadata_.natural_size;
      pipeline_metadata_.natural_size = gfx::Size(size.height(), size.width());
    }

    // TODO(neva): We don't support media::kUseSurfaceLayerForVideo feature.
    CHECK(!surface_layer_for_video_enabled_);

    DCHECK(!video_layer_);

    video_frame_provider_->SetNaturalVideoSize(pipeline_metadata_.natural_size);
    video_frame_provider_->UpdateVideoFrame();

    video_layer_ = cc::VideoLayer::Create(
        video_frame_provider_.get(),
        pipeline_metadata_.video_decoder_config.video_rotation());
    video_layer_->SetContentsOpaque(opaque_);
    client_->SetCcLayer(video_layer_.get());
  }

  if (observer_)
    observer_->OnMetadataChanged(pipeline_metadata_);

  CreateWatchTimeReporter();
  UpdatePlayState();
}

// With updating the video hole position in every frame, Sometimes scrolling a
// page with a video element showes awkward delayed video-hole movement.
// Thus, this method uses a OneShotTimer to update the video position every
// 100ms, which is the previous implementation's position update period policy.
void WebMediaPlayerMSE::UpdateVideoHoleBoundary(bool forced) {
  // TODO: Need to remove throttleUpdateVideoHoleBoundary_ after improving
  // uMediaServer's performance of video hole position update.
  // Current uMeidaServer cannot update video-hole position smoothly at times.
  if (forced || !throttleUpdateVideoHoleBoundary_.IsRunning()) {
    // NOTE: We prefer to use natural_video_size_ because it is safer to
    // calculate source_rect for SetDisplayWindow. natural_video_size_ is set by
    // platform media-pipeline and also video info is set to video sink at the
    // time.
    if (!ComputeVideoHoleDisplayRect(
            last_computed_rect_in_view_space_, natural_video_size_,
            additional_contents_scale_, client_->WebWidgetViewRect(),
            client_->ScreenRect(), is_fullscreen_mode_,
            source_rect_in_video_space_, visible_rect_in_screen_space_,
            is_fullscreen_)) {
      // visibile_rect_in_screen_space_ will be empty
      // when video position is out of the screen.
      if (visible_rect_in_screen_space_.IsEmpty() && has_visibility_) {
        is_video_offscreen_ = true;
        SetVisibility(false);
        return;
      }
      // If forced update is used or video was offscreen, it needs to update
      // even though video position is not changed.
      if (!forced && !is_video_offscreen_)
        return;
    }

    if (is_video_offscreen_) {
      SetVisibility(true);
      is_video_offscreen_ = false;
    }

    if (media_platform_api_) {
      LOG(INFO) << __func__ << " called SetDisplayWindow("
                << "out=[" << visible_rect_in_screen_space_.ToString() << "]"
                << ", in=[" << source_rect_in_video_space_.ToString() << "]"
                << ", is_fullscreen=" << is_fullscreen_ << ")";
      media_platform_api_->SetDisplayWindow(visible_rect_in_screen_space_,
                                            source_rect_in_video_space_,
                                            is_fullscreen_);
    }

    if (!forced) {
      // The OneShotTimer, throttleUpdateVideoHoleBoundary_, is for correcting
      // the position of video-hole after scrolling.
      throttleUpdateVideoHoleBoundary_.Start(
          FROM_HERE,
          base::TimeDelta::FromMilliseconds(kMinVideoHoleUpdateInterval),
          base::Bind(&WebMediaPlayerMSE::UpdateVideoHoleBoundary,
                     base::Unretained(this), false));
    }
  }
}

// It returns true when it succeed to calcuate boundary rectangle.
// Returning false means videolayer is not created yet
// or layer doesn't transform on the screen space(no transform tree index).
// In other word, this means video is not shown on the screen if it returns
// false.
// Note: This api should be called only from |OnDidCommitCompositorFrame|
bool WebMediaPlayerMSE::UpdateBoundaryRect() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // Check if video_layer_ is available.
  if (!video_layer_.get())
    return false;

  // Check if transform_tree_index of the layer is valid.
  if (video_layer_->transform_tree_index() == -1)
    return false;

  // Compute the geometry of video frame layer.
  gfx::RectF rect(gfx::SizeF(video_layer_->bounds()));
  video_layer_->ScreenSpaceTransform().TransformRect(&rect);

  // Store the changed geometry information when it is actually changed.
  last_computed_rect_in_view_space_ =
      gfx::Rect(rect.x(), rect.y(), rect.width(), rect.height());

  return true;
}

void WebMediaPlayerMSE::SetVisibility(bool visible) {
  has_visibility_ = visible;
  media_platform_api_->SetVisibility(has_visibility_);
}

bool WebMediaPlayerMSE::HasVisibility() const {
  return has_visibility_;
}

void WebMediaPlayerMSE::OnDidCommitCompositorFrame() {
  if (!RenderTexture()) {
    if (!UpdateBoundaryRect()) {
      // UpdateBoundaryRect fails when video layer is not in current
      // composition.
      if (HasVisibility()) {
        is_video_offscreen_ = true;
        SetVisibility(false);
      }
      return;
    }
    UpdateVideoHoleBoundary(false);
  }
}

scoped_refptr<VideoFrame> WebMediaPlayerMSE::GetCurrentFrameFromCompositor()
    const {
  TRACE_EVENT0("media", "WebMediaPlayerImpl::GetCurrentFrameFromCompositor");

  return video_frame_provider_->GetCurrentFrame();
}

void WebMediaPlayerMSE::OnSetCdm(blink::WebContentDecryptionModule* cdm) {
  const std::string ks =
      media::ToWebContentDecryptionModuleImpl(cdm)->GetKeySystem();
  LOG(INFO) << "Setting key_system to media APIs : " << ks;
  media_platform_api_->SetKeySystem(ks);
}

void WebMediaPlayerMSE::SetRenderMode(blink::WebMediaPlayer::RenderMode mode) {
  if (render_mode_ == mode)
    return;

  render_mode_ = mode;
  if (RenderTexture()) {
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_OPAQUE);
#if defined(USE_VIDEO_TEXTURE)
    if (gfx::VideoTexture::IsSupported())
      media_platform_api_->SwitchToAutoLayout();
#endif
  } else {
#if defined(VIDEO_HOLE)
    video_frame_provider_->SetStorageType(media::VideoFrame::STORAGE_HOLE);
#endif
  }
}

void WebMediaPlayerMSE::SetDisableAudio(bool disable) {
  LOG(INFO) << __func__ << " disable=" << disable;
  media_platform_api_->SetDisableAudio(disable);
}

}  // namespace media
