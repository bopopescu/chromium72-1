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

#ifndef NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_
#define NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_

#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "neva/pal_service/public/mojom/system_servicebridge.mojom.h"

namespace pal {

class SystemServiceBridgeDelegate;

class SystemServiceBridgeImpl : public mojom::SystemServiceBridge {
 public:
  SystemServiceBridgeImpl();
  ~SystemServiceBridgeImpl() override;

  // mojom::SystemServiceBridge
  void Connect(const std::string& name,
               const std::string& appid,
               ConnectCallback callback) override;
  void Call(const std::string& uri, const std::string& payload) override;
  void Cancel() override;

 private:
  void OnResponse(const std::string& payload);

  std::unique_ptr<SystemServiceBridgeDelegate> delegate_;
  mojo::AssociatedInterfacePtr<mojom::SystemServiceBridgeClient> client_;

  DISALLOW_COPY_AND_ASSIGN(SystemServiceBridgeImpl);
};

class SystemServiceBridgeProviderImpl : public mojom::SystemServiceBridgeProvider {
 public:
  SystemServiceBridgeProviderImpl();
  ~SystemServiceBridgeProviderImpl() override;

  void AddBinding(mojom::SystemServiceBridgeProviderRequest request);

  // mojom::SystemServiceBridgeProvider
  void GetSystemServiceBridge(mojom::SystemServiceBridgeRequest request) override;

 private:
  mojo::BindingSet<mojom::SystemServiceBridgeProvider> bindings_;
  DISALLOW_COPY_AND_ASSIGN(SystemServiceBridgeProviderImpl);
};

}  // namespace pal

#endif  // NEVA_PAL_SERVICE_SYSTEM_SERVICEBRIDGE_H_
