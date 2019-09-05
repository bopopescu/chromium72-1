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

#ifndef MEDIA_BASE_NEVA_MEDIA_TYPE_RESTRICTION_H_
#define MEDIA_BASE_NEVA_MEDIA_TYPE_RESTRICTION_H_

namespace media {

struct MediaTypeRestriction {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int channels;

  MediaTypeRestriction(int width,
                       int height,
                       int frame_rate,
                       int bit_rate,
                       int channels)
      : width(width),
        height(height),
        frame_rate(frame_rate),
        bit_rate(bit_rate),
        channels(channels) {}

  // Checks to see if the specified |restriction| is supported.
  bool IsSatisfied(const MediaTypeRestriction& restriction) const {
    if (width && width < restriction.width)
      return false;
    if (height && height < restriction.height)
      return false;
    if (frame_rate && frame_rate < restriction.frame_rate)
      return false;
    if (bit_rate && bit_rate < restriction.bit_rate)
      return false;
    if (channels && channels < restriction.channels)
      return false;
    return true;
  }
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_MEDIA_TYPE_RESTRICTION_H_
