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
#include "../../base/utility.hpp"
#include "../../jrpc/jrpc.hpp"
#include "../../math/interval.hpp"

int main()
{
  try {
    namespace jrpc = dmitigr::jrpc;
    namespace math = dmitigr::math;
    using dmitigr::with_catch;

    // Parse request.
    {
      auto req = jrpc::Request::from_json(R"({"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 1})");
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "subtract");
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(req.params()->IsArray());
      DMITIGR_ASSERT(req.has_parameters());
      DMITIGR_ASSERT(req.parameter_count() == 2);
      DMITIGR_ASSERT(req.id());
      DMITIGR_ASSERT(req.id()->IsInt());
      DMITIGR_ASSERT(req.id()->GetInt() == 1);

      DMITIGR_ASSERT(req.parameter(0));
      DMITIGR_ASSERT(req.parameter(0).value()->IsInt());
      DMITIGR_ASSERT(req.parameter(0).value()->GetInt() == 42);
      DMITIGR_ASSERT(req.parameter(1));
      DMITIGR_ASSERT(req.parameter(1).value()->IsInt());
      DMITIGR_ASSERT(req.parameter(1).value()->GetInt() == 23);
      DMITIGR_ASSERT(req.to_string() == R"({"jsonrpc":"2.0","method":"subtract","params":[42,23],"id":1})");

      req.set_parameter(3, 7);
      DMITIGR_ASSERT(req.parameter_count() == 3 + 1);
      DMITIGR_ASSERT(req.parameter(2));
      DMITIGR_ASSERT(req.parameter(2).value()->IsNull());
      DMITIGR_ASSERT(req.parameter(3));
      DMITIGR_ASSERT(req.parameter(3).value()->IsInt());
      DMITIGR_ASSERT(req.parameter(3).value()->GetInt() == 7);
      DMITIGR_ASSERT(req.to_string() == R"({"jsonrpc":"2.0","method":"subtract","params":[42,23,null,7],"id":1})");

      req.reset_parameters(jrpc::Parameters_notation::positional);
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(!req.has_parameters());
      DMITIGR_ASSERT(req.parameter_count() == 0);
      DMITIGR_ASSERT(req.to_string() == R"({"jsonrpc":"2.0","method":"subtract","params":[],"id":1})");

      req.omit_parameters();
      DMITIGR_ASSERT(!req.params());
      DMITIGR_ASSERT(!req.has_parameters());
      DMITIGR_ASSERT(req.parameter_count() == 0);
      DMITIGR_ASSERT(req.to_string() == R"({"jsonrpc":"2.0","method":"subtract","id":1})");

      req.set_parameter(0, 10);
      req.set_parameter(1, 5.5);
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(req.params()->IsArray());
      DMITIGR_ASSERT(req.has_parameters());
      DMITIGR_ASSERT(req.parameter_count() == 2);
      DMITIGR_ASSERT(req.parameter(0));
      DMITIGR_ASSERT(req.parameter(0).value()->IsInt());
      DMITIGR_ASSERT(req.parameter(0).value()->GetInt() == 10);
      DMITIGR_ASSERT(req.parameter(1));
      DMITIGR_ASSERT(req.parameter(1).value()->IsFloat());
      DMITIGR_ASSERT(req.parameter(1).value()->GetFloat() == 5.5);
      DMITIGR_ASSERT(req.to_string() == R"({"jsonrpc":"2.0","method":"subtract","id":1,"params":[10,5.5]})");
    }

    // Parse notification.
    {
      const auto req = jrpc::Request::from_json(R"({"jsonrpc": "2.0", "method": "update", "params": [1,2,3,4,5]})");
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "update");
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(req.params()->IsArray());
      DMITIGR_ASSERT(req.has_parameters());
      DMITIGR_ASSERT(req.parameter_count() == 5);
      DMITIGR_ASSERT(!req.id());
    }

    // Parse invalid request 1.
    {
      const auto f = []
      {
        jrpc::Request::from_json(R"({"jsonrpc": "2.1", "method": "subtract", "params": [42, 23], "id": 1})");
      };
      DMITIGR_ASSERT(with_catch<jrpc::Exception>(f));
      try {
        f();
      } catch (const jrpc::Error& e) {
        DMITIGR_ASSERT(e.jsonrpc() == "2.0");
        DMITIGR_ASSERT(e.id().IsInt());
        DMITIGR_ASSERT(e.id().GetInt() == 1);
        DMITIGR_ASSERT(e.code() == jrpc::Server_errc::invalid_request);
        DMITIGR_ASSERT(!e.data());
      }
    }

    // Parse invalid request 2.
    {
      const auto f = []
      {
        jrpc::Request::from_json(R"({"excess": 0, "jsonrpc": "2.0", "method": "subtract", "params": [2, 1], "id": 1})");
      };
      DMITIGR_ASSERT(with_catch<jrpc::Exception>(f));
      try {
        f();
      } catch (const jrpc::Error& e) {
        DMITIGR_ASSERT(e.jsonrpc() == "2.0");
        DMITIGR_ASSERT(e.id().IsInt());
        DMITIGR_ASSERT(e.id().GetInt() == 1);
        DMITIGR_ASSERT(e.code() == jrpc::Server_errc::invalid_request);
        DMITIGR_ASSERT(!e.data());
      }
    }

    // Parse invalid notification.
    {
      const auto f = []
      {
        jrpc::Request::from_json(R"({"jsonrpc": "2.0", "METHOD": "subtract", "params": [42, 23]})");
      };
      DMITIGR_ASSERT(with_catch<jrpc::Exception>(f));
      try {
        f();
      } catch (const jrpc::Error& e) {
        DMITIGR_ASSERT(e.jsonrpc() == "2.0");
        DMITIGR_ASSERT(e.id().IsNull());
        DMITIGR_ASSERT(e.code() == jrpc::Server_errc::invalid_request);
        DMITIGR_ASSERT(!e.data());
      }
    }

    // Making request with Null ID and empty (not omitted) params.
    {
      jrpc::Request req{jrpc::null, "foo"};
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "foo");
      DMITIGR_ASSERT(!req.params());
      DMITIGR_ASSERT(req.id());
      DMITIGR_ASSERT(req.id()->IsNull());
      req.reset_parameters(jrpc::Parameters_notation::named);
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(req.params()->IsObject());
      DMITIGR_ASSERT(req.parameter_count() == 0);
    }

    // Making request with int ID.
    {
      const jrpc::Request req{3, "bar"};
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "bar");
      DMITIGR_ASSERT(!req.params());
      DMITIGR_ASSERT(req.id());
      DMITIGR_ASSERT(req.id()->IsInt());
      DMITIGR_ASSERT(req.id()->GetInt() == 3);
    }

    // Making request with string ID.
    {
      const jrpc::Request req{"Id123", "baz"};
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "baz");
      DMITIGR_ASSERT(!req.params());
      DMITIGR_ASSERT(req.id());
      DMITIGR_ASSERT(req.id()->IsString());
      DMITIGR_ASSERT(std::strcmp(req.id()->GetString(), "Id123") == 0);
    }

    // Making notification.
    {
      jrpc::Request req{"move"};
      DMITIGR_ASSERT(req.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req.method() == "move");
      DMITIGR_ASSERT(!req.params());
      DMITIGR_ASSERT(!req.id());

      req.set_parameter("x", 10);
      req.set_parameter("y", 20);
      DMITIGR_ASSERT(req.params());
      DMITIGR_ASSERT(req.params()->IsObject());
      DMITIGR_ASSERT(req.parameter_count() == 2);
      DMITIGR_ASSERT(req.parameter("x") && req.parameter("x").value()->IsInt() && req.parameter("x").value()->GetInt() == 10);
      DMITIGR_ASSERT(req.parameter("y") && req.parameter("y").value()->IsInt() && req.parameter("y").value()->GetInt() == 20);
    }

    // Convenient methods.
    {
      jrpc::Request req{4, "foo"};
      req.set_parameter("n", {});
      req.set_parameter("x", 10);
      req.set_parameter("y", 20);
      req.set_parameter("s", "foo");

      {
        const auto [y, x, s, n] = req.parameters_mandatory("y", "x", "s", "n");
        DMITIGR_ASSERT(y && x && s && n);
      }

      {
        const auto [y, x, s] = req.parameters_not_null("y", "x", "s");
        DMITIGR_ASSERT(y && x && s);
      }

      {
        const auto [zr] = req.parameters("z");
        DMITIGR_ASSERT(!zr);
        try {
          const auto z = zr.not_null<int>(math::Interval{1, 2000});
          (void)z;
        } catch (const jrpc::Error& e) {
          DMITIGR_ASSERT(e.code() == jrpc::Server_errc::invalid_params);
        }
      }

      {
        const auto [x, s, y] = req.parameters("x", "s", "y");
        DMITIGR_ASSERT(x && s && y);
      }

      {
        const auto [x, y, s, z] = req.parameters("x", "y", "s", "z");
        DMITIGR_ASSERT(x && y && s && !z);
      }

      {
        const auto [x, z] = req.parameters("x", "z");
        DMITIGR_ASSERT(x && !z);
      }

      {
        const auto x = req.parameter("x").not_null<int>();
        const auto y = req.parameter("y").not_null<std::int32_t>();
        const auto z = req.parameter("z").optional<int>();
        DMITIGR_ASSERT(x == 10);
        DMITIGR_ASSERT(y == 20);
        DMITIGR_ASSERT(!z);
      }

      {
        const auto x = req.parameter("x").not_null<int>({10,20});
        (void)x;
        const auto s = req.parameter("s").not_null<std::string_view>({"bar","baz","foo"});
        (void)s;
      }
    }

    // Copying request
    {
      const jrpc::Request req{"copy"};
      const jrpc::Request req_copy = req;
      DMITIGR_ASSERT(req_copy.jsonrpc() == "2.0");
      DMITIGR_ASSERT(req_copy.method() == "copy");
      DMITIGR_ASSERT(!req_copy.params());
      DMITIGR_ASSERT(!req_copy.id());
    }

    // Request::set_parameter() overload.
    {
      jrpc::Request req{1, "foo"};
      req.set_parameter("doc", rapidjson::Document{});
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
