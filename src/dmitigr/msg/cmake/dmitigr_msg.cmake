# -*- cmake -*-
#
# Copyright 2023 Dmitry Igrishin

# ------------------------------------------------------------------------------
# Info
# ------------------------------------------------------------------------------

dmitigr_libs_set_library_info(msg 0 0 0 "Messaging library")

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------

set(dmitigr_msg_headers
  conveyor.hpp
  database.hpp
  errc.hpp
  errctg.hpp
  exceptions.hpp
  mail_processor.hpp
  message.hpp
  processor.hpp
  sqlite_conveyor.hpp
  types_fwd.hpp
  )

# ------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------

set(dmitigr_libs_msg_deps base os rajson sqlixx)

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

if(DMITIGR_LIBS_TESTS)
  set(dmitigr_msg_tests convproc)
  set(dmitigr_msg_tests_target_link_libraries)
endif()
