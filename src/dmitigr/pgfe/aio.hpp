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

#ifndef DMITIGR_PGFE_AIO_HPP
#define DMITIGR_PGFE_AIO_HPP

/// @NEW: remove me
// #define DMITIGR_LIBS_AIO_ASIO
#define DMITIGR_LIBS_AIO_BOOST_ASIO

#ifdef DMITIGR_LIBS_AIO_BOOST_ASIO
#ifndef DMITIGR_LIBS_AIO_ASIO
#define DMITIGR_LIBS_AIO_ASIO
#endif
#endif

#undef DMITIGR_LIBS_AIO_ASIO_NAMESPACE
#ifdef DMITIGR_LIBS_AIO_BOOST_ASIO
#define DMITIGR_LIBS_AIO_ASIO_NAMESPACE boost::asio
#elif DMITIGR_LIBS_AIO_ASIO
#define DMITIGR_LIBS_AIO_ASIO_NAMESPACE asio
#endif

#undef DMITIGR_PGFE_AIO
#if defined(DMITIGR_LIBS_AIO_ASIO) || defined(DMITIGR_LIBS_AIO_UV)
#define DMITIGR_PGFE_AIO
#endif

#ifdef DMITIGR_LIBS_AIO_BOOST_ASIO
#include <boost/asio.hpp>
#elif DMITIGR_LIBS_AIO_ASIO
#include <asio.hpp>
#endif

#include "../base/err.hpp"
#include "types_fwd.hpp"

#include <functional>

namespace dmitigr::pgfe {

#ifdef DMITIGR_PGFE_AIO
/// An alias of AIO handler.
using Aio_handler = std::function<void(const dmitigr::Err&, Connection&)>;
#endif

} // namespace dmitigr::pgfe

#endif  // DMITIGR_PGFE_AIO_HPP
