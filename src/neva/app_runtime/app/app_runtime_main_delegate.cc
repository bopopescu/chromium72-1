// Copyright 2016-2019 LG Electronics, Inc.
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

#include "neva/app_runtime/app/app_runtime_main_delegate.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"
#include "neva/app_runtime/browser/app_runtime_quota_permission_delegate.h"
#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"
#include "services/service_manager/embedder/switches.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

namespace {

bool SubprocessNeedsResourceBundle(const std::string& process_type) {
  return process_type == service_manager::switches::kZygoteProcess ||
         process_type == switches::kPpapiPluginProcess ||
         process_type == switches::kPpapiBrokerProcess ||
         process_type == switches::kGpuProcess ||
         process_type == switches::kRendererProcess ||
         process_type == switches::kUtilityProcess;
}

static net::NetworkDelegate* g_network_delegate = nullptr;
static app_runtime::AppRuntimeQuotaPermissionDelegate*
    g_quota_permission_delegate = nullptr;

struct BrowserClientTraits
    : public base::internal::DestructorAtExitLazyInstanceTraits<
          app_runtime::AppRuntimeContentBrowserClient> {
  static app_runtime::AppRuntimeContentBrowserClient* New(void* instance) {
    return new app_runtime::AppRuntimeContentBrowserClient(
        g_network_delegate, g_quota_permission_delegate);
  }
};

base::LazyInstance<
    app_runtime::AppRuntimeContentBrowserClient, BrowserClientTraits>
        g_app_runtime_content_browser_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<app_runtime::AppRuntimeContentRendererClient>::DestructorAtExit
    g_app_runtime_content_renderer_client = LAZY_INSTANCE_INITIALIZER;

const char kLocaleResourcesDirName[] = "neva_locales";
const char kResourcesFileName[] = "app_runtime_content.pak";
}  // namespace

namespace app_runtime {

AppRuntimeContentBrowserClient* GetAppRuntimeContentBrowserClient() {
  return g_app_runtime_content_browser_client.Pointer();
}

void SetNetworkDelegate(net::NetworkDelegate* p) {
  g_network_delegate = p;
}

void SetQuotaPermissionDelegate(
    app_runtime::AppRuntimeQuotaPermissionDelegate* p) {
  g_quota_permission_delegate = p;
}

AppRuntimeMainDelegate::AppRuntimeMainDelegate() {}

AppRuntimeMainDelegate::~AppRuntimeMainDelegate() {}

bool AppRuntimeMainDelegate::BasicStartupComplete(int* exit_code) {
  content::SetContentClient(&content_client_);
  return false;
}

void AppRuntimeMainDelegate::PreSandboxStartup() {
  InitializeResourceBundle();
}

void AppRuntimeMainDelegate::ProcessExiting(const std::string& process_type) {
  if (SubprocessNeedsResourceBundle(process_type))
    ui::ResourceBundle::CleanupSharedInstance();
}

void AppRuntimeMainDelegate::PreMainMessageLoopRun() {
}

void AppRuntimeMainDelegate::InitializeResourceBundle() {
  base::FilePath pak_file;
#if defined(USE_CBE)
  bool r = base::PathService::Get(base::DIR_ASSETS, &pak_file);
#else
  bool r = base::PathService::Get(base::DIR_MODULE, &pak_file);
#endif  // defined(USE_CBE)
  DCHECK(r);
  ui::ResourceBundle::InitSharedInstanceWithPakPath(
      pak_file.Append(FILE_PATH_LITERAL(kResourcesFileName)));

  base::PathService::Override(ui::DIR_LOCALES,
                              pak_file.AppendASCII(kLocaleResourcesDirName));
}

content::ContentBrowserClient*
AppRuntimeMainDelegate::CreateContentBrowserClient() {
  g_app_runtime_content_browser_client.Pointer()->SetBrowserExtraParts(this);
  return g_app_runtime_content_browser_client.Pointer();
}

content::ContentRendererClient*
AppRuntimeMainDelegate::CreateContentRendererClient() {
  return g_app_runtime_content_renderer_client.Pointer();
}

}  // namespace app_runtime
