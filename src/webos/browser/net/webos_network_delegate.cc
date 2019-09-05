// Copyright 2016-2018 LG Electronics, Inc.
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

#include "webos/browser/net/webos_network_delegate.h"

#include "base/files/file_util.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#include "webos/browser/webos_webview_renderer_state.h"

namespace webos {

WebOSNetworkDelegate::WebOSNetworkDelegate() : allow_all_access_(true) {
  // Hardcoded path for hotfix
  std::string securityPolicySettings;
  if (!base::ReadFileToString(base::FilePath("/etc/wam/security_policy.conf"),
                              &securityPolicySettings))
    return;
  std::istringstream iss(securityPolicySettings);
  std::string str;
  while (std::getline(iss, str)) {
    if (str.find("Allowed") != std::string::npos)
      ParsePathsFromSettings(allowed_target_paths_, iss);

    if (str.find("Trusted") != std::string::npos)
      ParsePathsFromSettings(allowed_trusted_target_paths_, iss);
  }

  allow_all_access_ = !allowed_target_paths_.size();
}

bool WebOSNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  if (allow_all_access_)
    return true;

  return IsAccessAllowed(request, absolute_path);
}

int WebOSNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    net::CompletionOnceCallback callback,
    GURL* new_url) {
  if (app_runtime::GetAppRuntimeContentBrowserClient()->DoNotTrack()) {
    static const char DNTHeader[] = "DNT";
    request->SetExtraRequestHeaderByName(DNTHeader, "1", true);
  }
  // TODO: Implement below functionality
  //  bool is_websocket_request = request && request->url().SchemeIsWSOrWSS();
  //  if (is_websocket_request) {
  //    for (auto& it : extra_websocket_headers_) {
  //      const std::pair<std::string, std::string>& header = it;
  //      request->SetExtraRequestHeaderByName(header.first, header.second,
  //      true);
  //    }
  //  }
  //
  //  const content::ResourceRequestInfoImpl* resourceInfo =
  //      static_cast<content::ResourceRequestInfoImpl*>(
  //          request->GetUserData(NULL));
  //
  //  WebOSWebViewRendererState::WebViewInfo web_view_info;
  //  if (resourceInfo &&
  //      WebOSWebViewRendererState::GetInstance()->GetInfo(
  //          resourceInfo->GetChildID(), resourceInfo->GetRouteID(),
  //          &web_view_info)) {
  //    std::string accept_language = web_view_info.accept_language;
  //    request->SetExtraRequestHeaderByName(
  //        net::HttpRequestHeaders::kAcceptLanguage, accept_language, false);
  //  }
  return net::OK;
}

void WebOSNetworkDelegate::ParsePathsFromSettings(
    std::vector<std::string>& paths,
    std::istringstream& stream) {
  std::string str;
  do {
    std::getline(stream, str);
    if (str.find("size=") != std::string::npos)
      break;
    std::string::size_type path_pos = str.find("=");
    if (path_pos != std::string::npos) {
      paths.push_back(str.substr(path_pos + 1));
    }
  } while (str != "");
}

bool WebOSNetworkDelegate::IsAccessAllowed(const net::URLRequest& request,
                                           const base::FilePath& path) const {
  base::File::Info file_info;
  // Deny access if cannot get file information
  if (!base::GetFileInfo(path, &file_info))
    return false;

  // Deny directory access
  if (file_info.is_directory || path.EndsWithSeparator())
    return false;

  const base::FilePath stripped_path(path.StripTrailingSeparators());

  // 1. Resources in globally permitted paths
  for (const auto& target_path : allowed_target_paths_) {
    const base::FilePath white_listed_path(target_path);
    // base::FilePath::operator== should probably handle trailing separators.
    if (white_listed_path == stripped_path || white_listed_path.IsParent(path))
      return true;
  }

  const content::ResourceRequestInfo* resource_request_info =
      content::ResourceRequestInfo::ForRequest(&request);

  if (!resource_request_info)
    return true;  // not a webview call, allow access???

  // PlzNavigate: navigation requests are created with a valid FrameTreeNode ID
  // and invalid RenderProcessHost and RenderFrameHost IDs.
  WebOSWebViewRendererState::WebViewInfo web_view_info;
  if (resource_request_info->GetFrameTreeNodeId() != -1) {
    if (!WebOSWebViewRendererState::GetInstance()->GetInfoForFrameTreeNodeId(
            resource_request_info->GetFrameTreeNodeId(), &web_view_info))
      return true;  // not a webview call, allow access???
  } else if (!WebOSWebViewRendererState::GetInstance()->GetInfo(
                 resource_request_info->GetChildID(),
                 resource_request_info->GetRouteID(), &web_view_info)) {
    return true;  // not a webview call, allow access???
  }

  std::string caller_path = web_view_info.app_path;
  std::string trust_level = web_view_info.trust_level;

  // in following we handle schemes set by AppRuntime applications
  // strip leading separators until only one is left
  while (caller_path.find("//") == 0)
    caller_path = caller_path.substr(1);

  // 2. Resources in app folder path
  const base::FilePath white_listed_path =
      base::FilePath(caller_path).StripTrailingSeparators();
  if (white_listed_path == stripped_path || white_listed_path.IsParent(path))
    return true;

  // 3. Resources for trusted app
  if (trust_level == "trusted") {
    for (const auto& trusted_target_path : allowed_trusted_target_paths_) {
      // Strip trailingseparators, this allows using both /foo/ and /foo in
      // security_config file
      const base::FilePath white_listed_path =
          base::FilePath(trusted_target_path).StripTrailingSeparators();
      // base::FilePath::operator== should probably handle trailing separators.
      if (white_listed_path == stripped_path ||
          white_listed_path.IsParent(path))
        return true;
    }
  }

  // 4. Resources for app with web security disabled
  if (!web_view_info.web_security_enabled)
    return true;  // allow access???

  return false;
}

}  // namespace webos
