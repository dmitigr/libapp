# Date and time library in C++

`dmitigr::dt` - is an easy-to-use library to work with date and time in C++.

## Example

```cpp
#include <cassert>
#include <dmitigr/dt/dt.hpp>

int main()
{
  namespace dt = dmitigr::dt;
  auto ts = dt::Timestamp::make();
  ts->set_date(2019, dt::Month::feb, 20);
  assert(ts->day_of_week() == dt::Day_of_week::wed);
}
```

## Features

Parsing of HTTP-date defined in [RFC7231][rfc7231_7111].

[rfc7231_7111]: https://tools.ietf.org/html/rfc7231#section-7.1.1.1
