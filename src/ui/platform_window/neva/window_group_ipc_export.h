// Copyright 2017-2018 LG Electronics, Inc.
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

#ifndef UI_PLATFORM_WINDOW_NEVA_WINDOW_GROUP_IPC_EXPORT_H
#define UI_PLATFORM_WINDOW_NEVA_WINDOW_GROUP_IPC_EXPORT_H

#if defined(COMPONENT_BUILD)
#if defined(PLATFORM_WINDOW_WEBOS_IPC_IMPLEMENTATION)
#define IPC_MESSAGE_EXPORT_WINDOW_GROUP_CONFIGURATION \
  __attribute__((visibility("default")))
#else
#define IPC_MESSAGE_EXPORT_WINDOW_GROUP_CONFIGURATION
#endif

#else  // defined(COMPONENT_BUILD)
#define IPC_MESSAGE_EXPORT_WINDOW_GROUP_CONFIGURATION
#endif

#endif  // UI_PLATFORM_WINDOW_NEVA_WINDOW_GROUP_IPC_EXPORT_H
