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

// Extension of Windows.Data.Xml.Dom

#pragma once
#pragma comment(lib, "runtimeobject")

#include <stdexcept>
#include <string>

#include <windows.data.xml.dom.h>
#include <wrl.h>

namespace dmitigr::winbase::rt::data::xml::dom {

namespace base = ABI::Windows::Data::Xml::Dom;
namespace wrl = Microsoft::WRL;

/// @returns An instance of base::IXmlDocument.
inline auto create_xml_document_from_string(const std::wstring& input)
{
  wrl::ComPtr<base::IXmlDocument> result;

  if (FAILED(Windows::Foundation::ActivateInstance(wrl::Wrappers::HStringReference(
        RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(), &result)))
    throw std::runtime_error{"cannot activate instance of"
      " RuntimeClass_Windows_Data_Xml_Dom_XmlDocument"};

  wrl::ComPtr<base::IXmlDocumentIO> io;
  if (FAILED(result.As(&io)))
    throw std::runtime_error{"cannot represent IXmlDocument as IXmlDocumentIO"};

  if (FAILED(io->LoadXml(wrl::Wrappers::HStringReference(input.c_str()).Get())))
    throw std::runtime_error{"cannot load XML input to IXmlDocumentIO"};

  return result;
}

} // dmitigr::winbase::rt::data::xml::dom
