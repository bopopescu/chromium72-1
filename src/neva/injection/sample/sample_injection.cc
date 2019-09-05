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

#include "injection/sample/sample_injection.h"

#include "base/bind.h"
#include "base/macros.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "injection/common/public/renderer/injection_base.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "neva/injection/grit/injection_resources.h"
#include "neva/pal_service/public/mojom/constants.mojom.h"
#include "neva/pal_service/public/mojom/sample.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "ui/base/resource/resource_bundle.h"

namespace injections {

namespace {

inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

}  // namespace

const char SampleInjectionExtension::kInjectionName[] = "v8/sample";

class SampleInjection : public gin::Wrappable<SampleInjection>,
                        public pal::mojom::SampleListener,
                        public InjectionBase {
 public:
  static gin::WrapperInfo kWrapperInfo;

  SampleInjection();
  ~SampleInjection() override;

  // exposed methods

  // Call function from platform. Return nothing
  void CallFunc(const std::string& arg1, const std::string& arg2);
  // Get cached value
  std::string GetValue() const;
  // Get platform value
  std::string GetPlatformValue() const;
  // Process data received from app (multiple callback objects case)
  bool ProcessData(gin::Arguments* args);

  // Subscribe "sample" object to literal data notifications
  void SubscribeToEvent();
  // Unsubscribe "sample" object from literal data notifications
  void UnsubscribeFromEvent();

  // not exposed methods
  void DispatchValueChanged(const std::string& val);

  void DataUpdated(const std::string& data) override;

private:
  static const char kCallFuncMethodName[];
  static const char kGetPlatformValueMethodName[];
  static const char kGetValueMethodName[];
  static const char kProcessDataMethodName[];
  static const char kSubscribeToEventMethodName[];
  static const char kUnsubscribeFromEventMethodName[];

  static const char kDispatchValueChangedName[];
  static const char kReceivedSampleUpdateName[];

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  void OnSubscribeRespond(pal::mojom::SampleListenerAssociatedRequest request);

  void OnProcessDataRespond(
      std::unique_ptr<v8::Persistent<v8::Function>> callback,
      std::string data,
      bool result);

  void ReceivedSampleUpdate(const std::string& value);

  std::string cached_app_value_;
  mojo::AssociatedBinding<pal::mojom::SampleListener> listener_binding_;
  pal::mojom::SamplePtr sample_;
};

gin::WrapperInfo SampleInjection::kWrapperInfo = {
    gin::kEmbedderNativeGin
};

const char SampleInjection::kCallFuncMethodName[] = "callFunc";
const char SampleInjection::kGetValueMethodName[] = "getValue";
const char SampleInjection::kGetPlatformValueMethodName[] = "getPlatformValue";
const char SampleInjection::kProcessDataMethodName[] = "processData";
const char SampleInjection::kSubscribeToEventMethodName[]
    = "_subscribeToEvent";
const char SampleInjection::kUnsubscribeFromEventMethodName[]
    = "_unsubscribeFromEvent";

const char SampleInjection::kDispatchValueChangedName[]
    = "_dispatchValueChanged";
const char SampleInjection::kReceivedSampleUpdateName[]
    = "_receivedSampleUpdate";

SampleInjection::SampleInjection()
    : cached_app_value_("null"),
      listener_binding_(this) {
  service_manager::Connector* connector =
      content::ChildThread::Get()->GetConnector();
  if (connector)
    connector->BindInterface(pal::mojom::kServiceName,
                             mojo::MakeRequest(&sample_));
}

SampleInjection::~SampleInjection() {
}

void SampleInjection::CallFunc(
    const std::string& arg1, const std::string& arg2) {
  sample_->CallFunc(arg1, arg2);
}

std::string SampleInjection::GetValue() const {
  return cached_app_value_;
}

std::string SampleInjection::GetPlatformValue() const {
  std::string result;
  if (sample_ && sample_->PlatformValue(&result))
    return result;
  return std::string();
}

bool SampleInjection::ProcessData(gin::Arguments* args) {
  v8::Local<v8::Function> func;
  std::string data;

  if (!args->GetNext(&func) || !args->GetNext(&data)) {
    LOG(ERROR) << __func__ << "(), wrong arguments list";
    return false;
  }

  auto callback_ptr =
      std::make_unique<v8::Persistent<v8::Function>>(
          args->isolate(),
          func);

  const std::string data_copy(data);
  sample_->ProcessData(data_copy,
      base::BindOnce(
          &SampleInjection::OnProcessDataRespond,
          base::Unretained(this),
          std::move(callback_ptr),
          std::move(data)));

  return true;
}

void SampleInjection::SubscribeToEvent() {
  sample_->Subscribe(base::BindOnce(
                     &SampleInjection::OnSubscribeRespond,
                     base::Unretained(this)));
}

void SampleInjection::UnsubscribeFromEvent() {
  listener_binding_.Unbind();
}

void SampleInjection::DispatchValueChanged(const std::string& val) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> callback_key =
      gin::StringToV8(isolate, kDispatchValueChangedName);
  if (!IsTrue(wrapper->Has(context, callback_key)))
    return;

  v8::Local<v8::Function> callback;
  if (!gin::Converter<v8::Local<v8::Function>>::FromV8(
        isolate, wrapper->Get(callback_key), &callback))
    return;

  const int argc = 1;
  v8::Local<v8::Value> argv[] = { gin::StringToV8(isolate, val) };
  ALLOW_UNUSED_LOCAL(callback->Call(context, wrapper, argc, argv));
}


void SampleInjection::DataUpdated(const std::string& data) {
  ReceivedSampleUpdate(data);
}

gin::ObjectTemplateBuilder SampleInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SampleInjection>::
      GetObjectTemplateBuilder(isolate)
          .SetMethod(kCallFuncMethodName, &SampleInjection::CallFunc)
          .SetMethod(kGetValueMethodName, &SampleInjection::GetValue)
          .SetMethod(kGetPlatformValueMethodName,
                     &SampleInjection::GetPlatformValue)
          .SetMethod(kProcessDataMethodName, &SampleInjection::ProcessData)
          .SetMethod(kSubscribeToEventMethodName,
                     &SampleInjection::SubscribeToEvent)
          .SetMethod(kUnsubscribeFromEventMethodName,
                     &SampleInjection::UnsubscribeFromEvent);
}

void SampleInjection::OnSubscribeRespond(
    pal::mojom::SampleListenerAssociatedRequest request) {
  LOG(INFO) << __func__ << "()";
  listener_binding_.Bind(std::move(request));
}

void SampleInjection::OnProcessDataRespond(
    std::unique_ptr<v8::Persistent<v8::Function>> callback,
    std::string data,
    bool result) {
  LOG(INFO) << __func__ << "(): data = " << data
             << ", result = " << result;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper)) {
    LOG(ERROR) << __func__ << "(): can not get wrapper!";
    return;
  }

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Function> local_callback = callback->Get(isolate);

  const int argc = 1;
  v8::Local<v8::Value> argv[] = {
      gin::Converter<bool>::ToV8(isolate, result),
  };

  ALLOW_UNUSED_LOCAL(local_callback->Call(context, wrapper, argc, argv));
}

void SampleInjection::ReceivedSampleUpdate(const std::string& value) {
  LOG(INFO) << __func__ << "(): value = " << value;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;

  v8::Local<v8::Context> context = wrapper->CreationContext();
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> callback_key = gin::StringToV8(
      isolate, kReceivedSampleUpdateName);

  if (!IsTrue(wrapper->Has(context, callback_key)))
    return;

  v8::Local<v8::Function> callback;
  if (!gin::Converter<v8::Local<v8::Function>>::FromV8(
      isolate, wrapper->Get(callback_key), &callback))
    return;

  const int argc = 1;
  v8::Local<v8::Value> argv[] = { gin::StringToV8(isolate, value) };
  ALLOW_UNUSED_LOCAL(callback->Call(context, wrapper, argc, argv));
}

// static
void SampleInjectionExtension::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> navigator;
  if (!gin::Converter<v8::Local<v8::Object>>::FromV8(
      isolate, global->Get(gin::StringToV8(isolate, "navigator")), &navigator))
    return;

  if (IsTrue(navigator->Has(context, gin::StringToV8(isolate, "sample"))))
    return;

  if (!CreateSampleObject(isolate, navigator).IsEmpty())
    RunSupplementJS(isolate, context);
}

void SampleInjectionExtension::Uninstall(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> navigator_name = gin::StringToV8(isolate, "navigator");
  v8::Local<v8::Object> navigator;
  if (gin::Converter<v8::Local<v8::Object>>::FromV8(
          isolate, global->Get(navigator_name), &navigator)) {
    v8::Local<v8::String> sample_name = gin::StringToV8(isolate, "sample");
    if (IsTrue(navigator->Has(context, sample_name)))
      ALLOW_UNUSED_LOCAL(navigator->Delete(context, sample_name));
  }
}

// static
v8::MaybeLocal<v8::Object> SampleInjectionExtension::CreateSampleObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> parent) {
  gin::Handle<SampleInjection> sample =
      gin::CreateHandle(isolate, new SampleInjection());
  parent->Set(gin::StringToV8(isolate, "sample"), sample.ToV8());
  return sample->GetWrapper(isolate);
}

// static
void SampleInjectionExtension::DispatchValueChanged(
    blink::WebLocalFrame* frame,
    const std::string& val) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> navigator;
  if (!gin::Converter<v8::Local<v8::Object>>::FromV8(
      isolate, global->Get(gin::StringToV8(isolate, "navigator")), &navigator))
    return;

  SampleInjection* sample_injection = nullptr;
  if (!gin::Converter<SampleInjection*>::FromV8(
          isolate,
          navigator->Get(gin::StringToV8(isolate, "sample")),
          &sample_injection))
    return;

  sample_injection->DispatchValueChanged(val);
}

void SampleInjectionExtension::RunSupplementJS(
    v8::Isolate* isolate, v8::Local<v8::Context> context) {
  const std::string supplement_js(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_SAMPLE_INJECTION_JS));
  v8::Local<v8::Script> local_script;
  v8::MaybeLocal<v8::Script> script = v8::Script::Compile(
       context, gin::StringToV8(isolate, supplement_js.c_str()));
  if (script.ToLocal(&local_script)) {
     auto res = local_script->Run(context);
     ALLOW_UNUSED_LOCAL(res);
  }
}

}  // namespace injections
