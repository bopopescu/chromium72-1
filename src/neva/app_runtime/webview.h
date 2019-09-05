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

#ifndef NEVA_APP_RUNTIME_WEBVIEW_H_
#define NEVA_APP_RUNTIME_WEBVIEW_H_

#include <set>

#include "base/memory/memory_pressure_listener.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/media_stream_request.h"
#include "neva/app_runtime/public/app_runtime_constants.h"
#include "ui/gfx/geometry/size.h"

namespace content {
class WebContents;
struct WebPreferences;
}  // namespace content

namespace app_runtime {

class AppRuntimeEvent;
class AppRuntimeWebViewHostImpl;
class WebAppInjectionManager;
class WebViewDelegate;
class WebViewProfile;

class WebView : public content::WebContentsDelegate,
                public content::WebContentsObserver {
 public:
  enum Attribute {
    AllowRunningInsecureContent,
    AllowScriptsToCloseWindows,
    AllowUniversalAccessFromFileUrls,
    RequestQuotaEnabled,
    SuppressesIncrementalRendering,
    DisallowScrollbarsInMainFrame,
    DisallowScrollingInMainFrame,
#if defined(HBBTV)
    HbbtvEnabled,
    HbbtvAdvancedAdInsertionEnabled,
    HbbtvClearMediaPlayerEnabled,
#endif
    JavascriptCanOpenWindows,
    SpatialNavigationEnabled,
    SupportsMultipleWindows,
    CSSNavigationEnabled,
    V8DateUseSystemLocaloffset,
    AllowLocalResourceLoad,
    LocalStorageEnabled,
    WebSecurityEnabled,
    XFrameOptionsCrossOriginAllowed,
    KeepAliveWebApp,
    AdditionalFontFamilyEnabled,
    BackHistoryAPIDisabled,
    ForceVideoTexture,
  };

  enum FontFamily {
    StandardFont,
    FixedFont,
    SerifFont,
    SansSerifFont,
    CursiveFont,
    FantasyFont
  };

  static void SetFileAccessBlocked(bool blocked);

  WebView(int width, int height, WebViewProfile* profile = nullptr);
  ~WebView() override;

  void SetDelegate(WebViewDelegate* delegate);
  void CreateWebContents();
  content::WebContents* GetWebContents();
  void AddUserStyleSheet(const std::string& sheet);
  std::string UserAgent() const;
  void LoadUrl(const GURL& url);
  void StopLoading();
  void LoadExtension(const std::string& name);
  void ClearExtensions();
  void ReplaceBaseURL(const GURL& new_url);
  const std::string& GetUrl();

  void SuspendDOM();
  void ResumeDOM();
  void SuspendMedia();
  void ResumeMedia();
  void SuspendPaintingAndSetVisibilityHidden();
  void ResumePaintingAndSetVisibilityVisible();
  bool SetSkipFrame(bool enable);

  bool IsActiveOnNonBlankPaint() const {
    return active_on_non_blank_paint_;
  }

  void CommitLoadVisually();
  std::string DocumentTitle() const;
  void RunJavaScript(const std::string& js_code);
  void RunJavaScriptInAllFrames(const std::string& js_code);
  void Reload();
  int RenderProcessPid() const;
  bool IsDrmEncrypted(const std::string& url);
  std::string DecryptDrm(const std::string& url);

  int DevToolsPort() const;
  void SetInspectable(bool enable);
  void AddCustomPluginDir(const std::string& directory);
  void SetBackgroundColor(int r, int g, int b, int a);
  void SetAllowFakeBoldText(bool allow);
  void SetShouldSuppressDialogs(bool suppress);
  void SetAppId(const std::string& app_id);

  // SetSecurityOrigin is used for changing the security origin for local
  // URLs (kFileScheme). It's needed to set the unique application origin
  // of local storage. This works only for renderer per application model.
  void SetSecurityOrigin(const std::string& identifier);
  void SetAcceptLanguages(const std::string& languages);
  void SetUseLaunchOptimization(bool enabled, int delay_ms);
  void SetUseEnyoOptimization(bool enabled);
  void SetAppPreloadHint(bool is_preload);
  void SetTransparentBackground(bool enable);
  void SetBoardType(const std::string& board_type);
  void SetMediaCodecCapability(const std::string& capability);
  void SetSearchKeywordForCustomPlayer(bool enabled);
  void SetSupportDolbyHDRContents(bool support);
  void SetUseUnlimitedMediaPolicy(bool enabled);
  void UpdatePreferencesAttribute(WebView::Attribute attribute, bool enable);
  void SetNetworkQuietTimeout(double timeout);
  void SetFontFamily(WebView::FontFamily fontFamily, const std::string& font);
  void SetFontHintingNone();
  void SetFontHintingSlight();
  void SetFontHintingMedium();
  void SetFontHintingFull();
  void SetActiveOnNonBlankPaint(bool active);
  void SetViewportSize(int width, int height);
  void NotifyMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel level);
  void SetVisible(bool visible);
  void SetDatabaseIdentifier(const std::string& identifier);
  void SetVisibilityState(WebPageVisibilityState visibility_state);
  void DeleteWebStorages(const std::string& identifier);
  void SetFocus(bool focus);
  double GetZoomFactor();
  void SetZoomFactor(double factor);
  void SetDoNotTrack(bool dnt);
  void ForwardAppRuntimeEvent(AppRuntimeEvent* event);
  bool CanGoBack() const;
  void GoBack();
  void RequestGetCookies(const std::string& url);
  void SetAdditionalContentsScale(float scale_x, float scale_y);
  void SetHardwareResolution(int width, int height);
  void SetEnableHtmlSystemKeyboardAttr(bool enable);

  void RequestInjectionLoading(const std::string& injection_name);
  void RequestClearInjections();
  bool IsKeyboardVisible() const;
  void ResetStateToMarkNextPaintForContainer();
  void DropAllPeerConnections(DropPeerConnectionReason reason);
  void SetV8SnapshotPath(const std::string& v8_snapshot_path);
  void SetV8ExtraFlags(const std::string& v8_extra_flags);
  void SetUseNativeScroll(bool use_native_scroll);

  // WebContentsDelegate
  void LoadProgressChanged(content::WebContents* source,
                           double progress) override;

  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void CloseContents(content::WebContents* source) override;

  bool ShouldSuppressDialogs(content::WebContents* source) override;
  gfx::Size GetSizeForNewRenderView(
      content::WebContents* web_contents) const override;

  void DidFrameFocused() override;
  void UpdatePreferences();

  void EnterFullscreenModeForTab(
      content::WebContents* web_contents,
      const GURL& origin,
      const blink::WebFullscreenOptions& options) override;

  void ExitFullscreenModeForTab(content::WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(
      const content::WebContents* web_contents) const override;

  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const GURL& security_origin,
                                  content::MediaStreamType type) override;

  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      content::MediaResponseCallback callback) override;

  bool DecidePolicyForResponse(bool isMainFrame,
                               int statusCode,
                               const GURL& url,
                               const base::string16& statusText) override;
  void OverrideWebkitPrefs(content::WebPreferences* prefs) override;

  void DidHistoryBackOnTopPage() override;

  // WebContentsObserver
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void DidStartLoading() override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& candidates) override;
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override;

  bool OnMessageReceived(const IPC::Message& message) override;
  void RenderProcessCreated(base::ProcessHandle handle) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentLoadedInFrame(content::RenderFrameHost* frame_host) override;
  void DidDropAllPeerConnections(
      content::DropPeerConnectionReason reason) override;
  void DidReceiveCompositorFrame() override;

  void SetSSLCertErrorPolicy(SSLCertErrorPolicy policy) {
    ssl_cert_error_policy_ = policy;
  }

  SSLCertErrorPolicy GetSSLCertErrorPolicy() const {
    return ssl_cert_error_policy_;
  }

  // Profile
  WebViewProfile* GetProfile() const;
  void SetProfile(WebViewProfile* profile);

 private:

  void SendGetCookiesResponse(const net::CookieList& cookie_list);

  void UpdatePreferencesAttributeForPrefs(content::WebPreferences* prefs,
                                          WebView::Attribute attribute,
                                          bool enable);

  void NotifyRenderWidgetWasResized();

  static bool ConvertVisibilityState(WebPageVisibilityState from,
                                     blink::mojom::PageVisibilityState& to);
  void UpdateViewportScaleFactor();
  void SetDisallowScrollbarsInMainFrame(bool disallow);
  void GrantLoadLocalResources();

  WebViewDelegate* webview_delegate_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<content::WebPreferences> web_preferences_;
  std::unique_ptr<WebAppInjectionManager> injection_manager_;
  std::unique_ptr<AppRuntimeWebViewHostImpl> webview_host_impl_;

  bool should_suppress_dialogs_ = false;
  bool active_on_non_blank_paint_ = false;
  bool full_screen_ = false;
  bool enable_skip_frame_ = false;
  int width_;
  int height_;
  gfx::Size viewport_size_;
  std::string document_title_;
  std::set<std::string> injected_css_;
  SSLCertErrorPolicy ssl_cert_error_policy_ = SSL_CERT_ERROR_POLICY_DEFAULT;
  WebViewProfile* profile_ = nullptr;
  std::map<WebView::Attribute, bool> webview_preferences_list_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_WEBVIEW_H_
