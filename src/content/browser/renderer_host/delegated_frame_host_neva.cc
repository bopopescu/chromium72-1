// Copyright 2019 LG Electronics, Inc.
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

#include "content/browser/renderer_host/delegated_frame_host_neva.h"

#include "base/command_line.h"
#include "base/task/post_task.h"
#include "cc/base/switches.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace content {

namespace {
const int kBackgroundCleanupDelayMs = 1000;

viz::CompositorFrame CreateDelegatedFrame(float scale_factor, gfx::Size size) {
  viz::CompositorFrame frame;
  frame.metadata.device_scale_factor = scale_factor;

  std::unique_ptr<viz::RenderPass> pass = viz::RenderPass::Create();
  pass->SetNew(1, gfx::Rect(size), gfx::Rect(), gfx::Transform());
  frame.render_pass_list.push_back(std::move(pass));
  return frame;
}

}  // namespace

class ClosedKeepAliveWebAppTrigger : public viz::BeginFrameObserverBase {
 public:
  ClosedKeepAliveWebAppTrigger(DelegatedFrameHost* host) : host_(host) {
    task_runner_ = base::CreateSingleThreadTaskRunnerWithTraits(
        {content::BrowserThread::UI});
    begin_frame_source_ = std::make_unique<viz::DelayBasedBeginFrameSource>(
        std::make_unique<viz::DelayBasedTimeSource>(task_runner_.get()),
        viz::BeginFrameSource::kNotRestartableId);
    begin_frame_source_->AddObserver(this);
  }

  ~ClosedKeepAliveWebAppTrigger() override = default;

  // viz::BeginFrameObserverBase
  bool OnBeginFrameDerivedImpl(const viz::BeginFrameArgs& args) override {
    host_->OnBeginFrame(args,
                        base::flat_map<uint32_t, gfx::PresentationFeedback>());
    return true;
  }

  void OnBeginFrameSourcePausedChanged(bool paused) override {}

 private:
  DelegatedFrameHost* host_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<viz::SyntheticBeginFrameSource> begin_frame_source_;
};

////////////////////////////////////////////////////////////////////////////////
// DelegatedFrameHost

DelegatedFrameHost::DelegatedFrameHost(const viz::FrameSinkId& frame_sink_id,
                                       DelegatedFrameHostClient* client,
                                       bool should_register_frame_sink_id)
    : neva::DelegatedFrameHost(
          frame_sink_id,
          static_cast<neva::DelegatedFrameHostClient*>(client),
          should_register_frame_sink_id),
      use_aggressive_release_policy_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              cc::switches::kEnableAggressiveReleasePolicy)),
      weak_factory_(this) {}

DelegatedFrameHost::~DelegatedFrameHost() {}

void DelegatedFrameHost::WasShown(
    const viz::LocalSurfaceId& new_local_surface_id,
    const gfx::Size& new_dip_size,
    bool record_presentation_time) {
  // Calling origin procedure
  neva::DelegatedFrameHost::WasShown(new_local_surface_id, new_dip_size,
                                     record_presentation_time);
  if (use_aggressive_release_policy_) {
    background_cleanup_task_.Cancel();
    if (!deferred_resume_drawing_ && was_hidden_) {
      deferred_resume_drawing_ = true;
      was_hidden_ = false;
    } else {
      if (compositor_)
        compositor_->ResumeDrawing();
    }
  }

  DelegatedFrameHostClient* client =
      static_cast<DelegatedFrameHostClient*>(client_);
  if (!compositor_ && client->DelegatedFrameHostIsKeepAliveWebApp())
    keep_alive_trigger_ = std::make_unique<ClosedKeepAliveWebAppTrigger>(this);
}

void DelegatedFrameHost::WasHidden() {
  if (use_aggressive_release_policy_ && support_ && did_first_swap_ &&
      GetCurrentSurfaceId().is_valid())
    support_->SubmitCompositorFrame(
        GetCurrentSurfaceId().local_surface_id(),
        CreateDelegatedFrame(active_device_scale_factor_, surface_dip_size_));

  // Calling origin procedure
  neva::DelegatedFrameHost::WasHidden();

  if (use_aggressive_release_policy_) {
    if (compositor_)
      compositor_->SuspendDrawing();
    was_hidden_ = true;
    EvictDelegatedFrame();
    background_cleanup_task_.Reset(base::BindOnce(
        &DelegatedFrameHost::DoBackgroundCleanup, weak_factory_.GetWeakPtr()));
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, background_cleanup_task_.callback(),
        base::TimeDelta::FromMilliseconds(kBackgroundCleanupDelayMs));
  }
}

void DelegatedFrameHost::DoBackgroundCleanup() {
  viz::FrameEvictionManager::GetInstance()->PurgeAllUnlockedFrames();
}

void DelegatedFrameHost::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame,
    base::Optional<viz::HitTestRegionList> hit_test_region_list) {
  // Calling origin procedure
  neva::DelegatedFrameHost::SubmitCompositorFrame(
      local_surface_id, std::move(frame), std::move(hit_test_region_list));

  if (keep_alive_trigger_)
    keep_alive_trigger_.reset();

  if (compositor_ && deferred_resume_drawing_) {
    deferred_resume_drawing_ = false;
    compositor_->ResumeDrawing();
    compositor_->ScheduleFullRedraw();
  }

  did_first_swap_ = true;
}

void DelegatedFrameHost::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  active_device_scale_factor_ = surface_info.device_scale_factor();

  // Calling origin procedure
  neva::DelegatedFrameHost::OnFirstSurfaceActivation(surface_info);
}

void DelegatedFrameHost::AttachToCompositor(ui::Compositor* compositor) {
  if (!compositor)
    return;

  if (keep_alive_trigger_)
    keep_alive_trigger_.reset();

  neva::DelegatedFrameHost::AttachToCompositor(compositor);
}

}  // namespace content
