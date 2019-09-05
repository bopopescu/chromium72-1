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

#ifndef NEVA_PAL_SERVICE_PAL_SERVICE_EXPORT_H_
#define NEVA_PAL_SERVICE_PAL_SERVICE_EXPORT_H_

#if defined(COMPONENT_BUILD)
 #if defined(PAL_SERVICE_IMPLEMENTATION)
  #define PAL_SERVICE_EXPORT __attribute__((visibility("default")))
 #else
  #define PAL_SERVICE_EXPORT
 #endif
#else  // defined(COMPONENT_BUILD)
 #define PAL_SERVICE_EXPORT
#endif

#endif  // NEVA_PAL_SERVICE_PAL_SERVICE_EXPORT_H_
