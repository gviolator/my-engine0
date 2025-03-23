// #my_engine_source_header
#pragma once
#include "my/diag/check_handler.h"

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

    diag::CheckHandlerPtr prevHandler;
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
