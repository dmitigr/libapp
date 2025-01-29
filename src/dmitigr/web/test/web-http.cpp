// -*- C++ -*-
//
// Copyright 2023 Dmitry Igrishin
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

#include "../../web/http.hpp"

namespace url = dmitigr::url;
namespace web = dmitigr::web;
namespace ws = dmitigr::ws;

int main()
{
  auto* const req = reinterpret_cast<ws::Http_request*>(1);
  std::shared_ptr<ws::Http_io> io;
  url::Query_string qs;
  auto httper = web::Httper::make(nullptr, web::Config{});
  httper->add_public("/auth/.*")
    .set_docroot("/")
    .set_auth_checker({})
    .set_auth_prompter({})
    .set_before_tpler({})
    .add_tpler("/", [](auto&, const auto&){})
    .set_after_tpler({})
    .set_gener({});
  (*httper)(*req, io);
}
