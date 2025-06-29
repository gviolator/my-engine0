// #my_engine_source_file
#pragma once
#include "my/diag/assert_handler.h"

namespace my::test
{
  struct CheckGuardBase
  {
    bool noFailures() const
    {
      return failureCounter == 0 && fatalFailureCounter == 0;
    }

    size_t failureCounter = 0;
    size_t fatalFailureCounter = 0;

  protected:
    ~CheckGuardBase();

    diag::AssertHandlerPtr prevHandler;
  };

  /**
  */
  struct CheckGuard : CheckGuardBase
  {
    CheckGuard();
    ~CheckGuard();
  };

  /**
  */
  struct CheckGuardThrowOnFailure : CheckGuardBase
  {
    CheckGuardThrowOnFailure();
    ~CheckGuardThrowOnFailure();
  };
}  // namespace my::test
