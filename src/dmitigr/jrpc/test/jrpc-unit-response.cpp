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

#include "../../base/assert.hpp"
#include "../../jrpc/jrpc.hpp"

int main()
{
  try {
    namespace jrpc = dmitigr::jrpc;

    // Parse result response.
    {
      const auto res = jrpc::Response::make(R"({"jsonrpc": "2.0", "result": 19, "id": 1})");
      auto* const r = dynamic_cast<const jrpc::Result*>(res.get());
      DMITIGR_ASSERT(r);
      DMITIGR_ASSERT(r->jsonrpc() == "2.0");
      DMITIGR_ASSERT(r->id().IsInt());
      DMITIGR_ASSERT(r->id().GetInt() == 1);
      DMITIGR_ASSERT(r->data().IsInt());
      DMITIGR_ASSERT(r->data().GetInt() == 19);
      DMITIGR_ASSERT(r->to_string() == R"({"jsonrpc":"2.0","result":19,"id":1})");
    }

    // Parse error response.
    {
      const auto res = jrpc::Response::make(
        R"({"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": "1"})");
      auto* const r = dynamic_cast<jrpc::Error*>(res.get());
      DMITIGR_ASSERT(r);
      DMITIGR_ASSERT(r->jsonrpc() == "2.0");
      DMITIGR_ASSERT(r->id().IsString());
      DMITIGR_ASSERT(std::strcmp(r->id().GetString(), "1") == 0);
      DMITIGR_ASSERT(r->condition() == jrpc::Server_errc::method_not_found);
      DMITIGR_ASSERT(!r->data());
      DMITIGR_ASSERT(r->to_string() == R"({"jsonrpc":"2.0","error":{"code":-32601,"message":"Method not found"},"id":"1"})");
    }

    // Making result with null id.
    {
      jrpc::Result res;
      DMITIGR_ASSERT(res.jsonrpc() == "2.0");
      DMITIGR_ASSERT(res.id().IsNull());
      DMITIGR_ASSERT(res.data().IsNull());
      res.set_data(123);
      DMITIGR_ASSERT(res.data().IsInt());
      DMITIGR_ASSERT(res.data().GetInt() == 123);
    }

    // Making result with int id.
    {
      jrpc::Result res{1};
      DMITIGR_ASSERT(res.jsonrpc() == "2.0");
      DMITIGR_ASSERT(res.id().IsInt());
      DMITIGR_ASSERT(res.id().GetInt() == 1);
      DMITIGR_ASSERT(res.data().IsNull());
    }

    // Making result with string id.
    {
      jrpc::Result res{"id123"};
      DMITIGR_ASSERT(res.jsonrpc() == "2.0");
      DMITIGR_ASSERT(res.id().IsString());
      DMITIGR_ASSERT(std::strcmp(res.id().GetString(), "id123") == 0);
      DMITIGR_ASSERT(res.data().IsNull());
    }

    // Making error.
    {
      jrpc::Error err{jrpc::Server_errc::parse_error, jrpc::null};
      DMITIGR_ASSERT(err.jsonrpc() == "2.0");
      DMITIGR_ASSERT(err.id().IsNull());
      DMITIGR_ASSERT(!err.data());
      DMITIGR_ASSERT(err.condition() == jrpc::Server_errc::parse_error);
      err.set_data("important!");
      DMITIGR_ASSERT(err.data());
      DMITIGR_ASSERT(err.data()->IsString());
      DMITIGR_ASSERT(std::strcmp(err.data()->GetString(), "important!") == 0);
      DMITIGR_ASSERT(err.to_string() == R"({"jsonrpc":"2.0","id":null,"error":{"code":-32700,"message":"","data":"important!"}})");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
