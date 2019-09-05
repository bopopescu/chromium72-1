// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NEVA_SETTINGS_NEVA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NEVA_SETTINGS_NEVA_H_

#include "third_party/blink/renderer/core/frame/settings_delegate.h"

namespace blink {

class SettingsNeva {
 public:
  SettingsNeva()
      : back_history_api_enabled_(false),
        webos_native_scroll_enabled_(false),
        keep_alive_web_app_(false),
        gpu_rasterization_allowed_(true) {}

  void SetBackHistoryAPIEnabled(bool enable) {
    back_history_api_enabled_ = enable;
  }
  bool BackHistoryAPIEnabled() const { return back_history_api_enabled_; }

  void SetWebOSNativeScrollEnabled(bool enable) {
    webos_native_scroll_enabled_ = enable;
  }
  bool WebOSNativeScrollEnabled() { return webos_native_scroll_enabled_; }

  void SetKeepAliveWebApp(bool keep_alive) { keep_alive_web_app_ = keep_alive; }
  bool KeepAliveWebApp() const { return keep_alive_web_app_; }

  void SetGpuRasterizationAllowed(bool allowed) {
    gpu_rasterization_allowed_ = allowed;
  }
  bool GpuRasterizationAllowed() { return gpu_rasterization_allowed_; }

 private:
  bool back_history_api_enabled_ : 1;
  bool webos_native_scroll_enabled_ : 1;
  bool keep_alive_web_app_ : 1;
  bool gpu_rasterization_allowed_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_NEVA_SETTINGS_NEVA_H_
