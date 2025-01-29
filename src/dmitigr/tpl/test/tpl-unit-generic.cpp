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
#include "../../tpl/generic.hpp"

int main()
{
  try {
    namespace tpl = dmitigr::tpl;
    using tpl::Generic;

    {
      const Generic t;
      DMITIGR_ASSERT(t.parameter_count() == 0);
      DMITIGR_ASSERT(!t.has_parameters());
      DMITIGR_ASSERT(!t.has_unbound_parameters());
      DMITIGR_ASSERT(t.to_string("", "") == "");
      DMITIGR_ASSERT(t.to_output().res == "");
    }

    static const auto make_tpl = [](const std::string_view input)
    {
      auto [err, res] = Generic::make(input, "{{ ", " }}");
      DMITIGR_ASSERT(!err);
      return res;
    };

    static const auto to_string = [](const Generic& tpl)
    {
      return tpl.to_string("{{ ", " }}");
    };

    {
      const std::string input{"Hello {{ name }}! Dear {{ name }}, we wish you {{ wish }}!"};
      auto t = make_tpl(input);
      DMITIGR_ASSERT(t.parameter_count() == 2);
      DMITIGR_ASSERT(t.parameter_index("name") == 0);
      DMITIGR_ASSERT(t.parameter_index("wish") == 1);
      DMITIGR_ASSERT(t.parameter(0)->name() == "name");
      DMITIGR_ASSERT(t.parameter(1)->name() == "wish");
      DMITIGR_ASSERT(!t.parameter(0)->value());
      DMITIGR_ASSERT(!t.parameter("name")->value());
      DMITIGR_ASSERT(!t.parameter(1)->value());
      DMITIGR_ASSERT(!t.parameter("wish")->value());
      DMITIGR_ASSERT(t.has_parameter("name"));
      DMITIGR_ASSERT(t.has_parameter("wish"));
      DMITIGR_ASSERT(t.has_parameters());
      DMITIGR_ASSERT(t.has_unbound_parameters());
      //
      t.parameter("name")->set_value("Dima");
      t.parameter("wish")->set_value("luck");
      DMITIGR_ASSERT(!t.has_unbound_parameters());
      DMITIGR_ASSERT(t.parameter("name")->value() == "Dima");
      DMITIGR_ASSERT(t.parameter("wish")->value() == "luck");
      DMITIGR_ASSERT(to_string(t) == input);
      DMITIGR_ASSERT(t.to_output().res == "Hello Dima! Dear Dima, we wish you luck!");
      //
      t.parameter("name")->set_value("Olga");
      DMITIGR_ASSERT(t.to_output().res == "Hello Olga! Dear Olga, we wish you luck!");
    }

    {
      const std::string input{"Hello {{name}}!"};
      const auto t = make_tpl(input);
      DMITIGR_ASSERT(t.parameter_count() == 0);
      DMITIGR_ASSERT(!t.has_parameters());
      DMITIGR_ASSERT(!t.has_unbound_parameters());
      DMITIGR_ASSERT(to_string(t) == input);
      DMITIGR_ASSERT(t.to_output().res == "Hello {{name}}!");
    }

    {
      const std::string input{"Hello {{  name}}!"};
      const auto t = make_tpl(input);
      DMITIGR_ASSERT(t.parameter_count() == 0);
      DMITIGR_ASSERT(!t.has_parameters());
      DMITIGR_ASSERT(!t.has_unbound_parameters());
      DMITIGR_ASSERT(to_string(t) == input);
      DMITIGR_ASSERT(t.to_output().res == "Hello {{  name}}!");
    }

    {
      const std::string input{"var foo = {<<<json!!};"};
      auto [e, t] = Generic::make(input, "<<<", "!!");
      DMITIGR_ASSERT(!e);
      DMITIGR_ASSERT(t.parameter_count() == 1);
      DMITIGR_ASSERT(t.has_parameter("json"));
      DMITIGR_ASSERT(t.to_string("<<<", "!!") == input);
      t.parameter("json")->set_value("name : 'Dima', age : 36");
      DMITIGR_ASSERT(t.to_output().res == "var foo = {name : 'Dima', age : 36};");
    }

    {
      const std::string input{"Parameter {{{{ name }}}}!"};
      auto t = make_tpl(input);
      DMITIGR_ASSERT(t.parameter_count() == 0);
      DMITIGR_ASSERT(!t.has_parameter("name"));
      DMITIGR_ASSERT(to_string(t) == input);
    }

    {
      const std::string input1{"Text1 {{ p1 }}, text3 {{ p3 }}, text2 {{ p2 }}."};
      auto t1 = make_tpl(input1);
      DMITIGR_ASSERT(t1.parameter_count() == 3);
      DMITIGR_ASSERT(t1.has_parameter("p1"));
      DMITIGR_ASSERT(t1.has_parameter("p2"));
      DMITIGR_ASSERT(t1.has_parameter("p3"));

      const std::string input2{"text2 {{ p2 }}, text4 {{ p4 }}"};
      const auto t2 = make_tpl(input2);
      DMITIGR_ASSERT(t2.parameter_count() == 2);
      DMITIGR_ASSERT(t2.has_parameter("p2"));
      DMITIGR_ASSERT(t2.has_parameter("p4"));

      t1.replace_parameter("p3", t2);
      DMITIGR_ASSERT(t1.parameter_count() == 3);
      DMITIGR_ASSERT(t1.has_parameter("p1"));
      DMITIGR_ASSERT(t1.has_parameter("p2"));
      DMITIGR_ASSERT(t1.has_parameter("p4"));
      DMITIGR_ASSERT(to_string(t1) == "Text1 {{ p1 }}, text3 text2 {{ p2 }}, text4 {{ p4 }}, text2 {{ p2 }}.");
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "unknown error" << std::endl;
    return 2;
  }
}
