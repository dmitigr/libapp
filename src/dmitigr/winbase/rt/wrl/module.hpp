// -*- C++ -*-
//
// Copyright 2024 Dmitry Igrishin
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Extension of WRL

#pragma once
#pragma comment(lib, "runtimeobject")

#include <functional>
#include <stdexcept>
#include <utility>

#include <wrl.h>

namespace dmitigr::winbase::rt::wrl::module {

namespace base = Microsoft::WRL;

/// Registers COM objects to allow (other applications) connect to them.
template<base::ModuleType Type>
void register_com_objects(std::function<void()> callback)
{
  base::Module<Type>::Create(std::move(callback));

  // Prevent COM server shutdown after activating the notification.
  base::Module<Type>::GetModule().IncrementObjectCount();

  if (FAILED(base::Module<Type>::GetModule().RegisterObjects()))
    throw std::runtime_error{"cannot register one or more COM objects"};
}

template<base::ModuleType Type>
void unregister_com_objects()
{
  if (FAILED(base::Module<Type>::GetModule().UnregisterObjects()))
    throw std::runtime_error{"cannot unregister one or more COM objects"};

  base::Module<Type>::GetModule().DecrementObjectCount();
}

template<base::ModuleType Type>
class Com_objects_registrator final {
public:
  ~Com_objects_registrator()
  {
    try {
      unregister_com_objects<Type>();
    } catch (...) {}
  }

  Com_objects_registrator(std::function<void()> callback)
  {
    register_com_objects<Type>(std::move(callback));
  }
};

} // namespace dmitigr::winbase::rt::wrl::module
