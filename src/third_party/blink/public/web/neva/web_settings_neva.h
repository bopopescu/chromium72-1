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

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_

namespace blink {

class WebSettingsNeva {
 public:
  virtual void SetBackHistoryAPIEnabled(bool) = 0;
  virtual void SetWebOSNativeScrollEnabled(bool) = 0;
  virtual void SetKeepAliveWebApp(bool) = 0;
  virtual bool KeepAliveWebApp() = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_NEVA_WEB_SETTINGS_NEVA_H_
