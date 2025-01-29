# WebSocket C++ server

`dmitigr::ws` - is a high-performant C++ library to work with WebSockets.

## Echo

```cpp
#include <dmitigr/ws/ws.hpp>
#include <chrono>

namespace ws = dmitigr::ws;

class Connection final : public ws::Connection {
  void handle_message(const std::string_view payload, const ws::Data_format format) override
  {
    send(payload, format); // respond by using non-blocking IO
  }

  void handle_close(int, std::string_view) override {}
  void handle_drain() override {}
};

class Listener final : public ws::Listener {
  using ws::Listener::Listener;
  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&, std::shared_ptr<ws::Http_io>) override
  {
    return std::make_shared<Connection>();
  }
};

int main()
{
  ws::Listener_options lo{"127.0.0.1", 9001};
  lo.set_idle_timeout(std::chrono::seconds{10})
    .set_max_incoming_payload_size(16 * 1024);
  Listener{lo}.listen();
}
```

## Echo (threaded)

```cpp
#include <dmitigr/ws.hpp>
#include <chrono>
#include <sstream>
#include <thread>

namespace ws = dmitigr::ws;

// The implementation of the thread per message approach.
// (Using a thread pool approach instead is trivial.)
class Connection final : public ws::Connection {
  void handle_message(const std::string_view payload, const ws::Data_format format) override
  {
    std::thread{[ws = shared_from_this(), payload, format]
    {
      // Simulate a work.
      std::this_thread::sleep_for(std::chrono::seconds(3));

      // Respond by using non-blocking IO.
      ws->event_loop_call_soon([ws, payload, format, tid = std::this_thread::get_id()]
      {
        if (ws->is_connected()) {
          std::ostringstream s;
          s << "Echo from thread " << tid << ": " << payload;
          ws->send(s.str(), format);
        }
      });
    }}.detach();
  }

  void handle_close(int, std::string_view) override {};
  void handle_drain() override {};
};

class Listener final : public ws::Listener {
  using ws::Listener::Listener;
  std::shared_ptr<ws::Connection> handle_handshake(const ws::Http_request&, std::shared_ptr<ws::Http_io>) override
  {
    return std::make_shared<Connection>();
  }
};

int main()
{
  ws::Listener_options lo{"127.0.0.1", 9001};
  lo.set_idle_timeout(std::chrono::seconds{10})
    .set_max_incoming_payload_size(16 * 1024);
  Listener{lo}.listen();
}
```

## Features

  - scalable and fast;
  - convenient to use in multi-threaded applications.
