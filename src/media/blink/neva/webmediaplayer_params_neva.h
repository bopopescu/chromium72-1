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

#ifndef MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_
#define MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_

#include "media/blink/media_blink_export.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_string.h"

namespace media {

class MEDIA_BLINK_EXPORT WebMediaPlayerParamsNeva {
 public:
  WebMediaPlayerParamsNeva(
    blink::WebFloatPoint additional_contents_scale,
    blink::WebString application_id);

  WebMediaPlayerParamsNeva();
  ~WebMediaPlayerParamsNeva() {};

  blink::WebFloatPoint additional_contents_scale() const {
    return additional_contents_scale_;
  }

  blink::WebString application_id() const {
    return application_id_;
  }

  bool use_unlimited_media_policy() const {
    return use_unlimited_media_policy_;
  }

  void set_additional_contents_scale (
      const blink::WebFloatPoint additional_contents_scale) {
    additional_contents_scale_ = additional_contents_scale;
  }

  void set_application_id(const blink::WebString application_id) {
    application_id_ = application_id;
  }

  void set_use_unlimited_media_policy(const bool use_unlimited_media_policy) {
    use_unlimited_media_policy_ = use_unlimited_media_policy;
  }

 protected:
  blink::WebFloatPoint additional_contents_scale_;
  blink::WebString application_id_;
  bool use_unlimited_media_policy_ = false;
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBMEDIAPLAYER_PARAMS_NEVA_H_
