// #my_engine_source_file
#include "my/test/helpers/check_guard.h"

#include "my/diag/check_handler.h"

namespace my::test
{
  namespace
  {
    class CheckGuardHandler : public diag::IAssertHandler
    {
    public:
      CheckGuardHandler(CheckGuardBase& guard) :
          m_guard(guard)
      {
      }

      diag::FailureActionFlag handleAssertFailure(const diag::FailureData& data) override
      {
        ++m_guard.failureCounter;
        ++m_guard.fatalFailureCounter;

        return diag::FailureAction::None;
      }

    private:
      CheckGuardBase& m_guard;
    };

    class ThrowCheckGuardHandler final : public CheckGuardHandler
    {
    public:
      using CheckGuardHandler::CheckGuardHandler;

      diag::FailureActionFlag handleAssertFailure(const diag::FailureData& data) override
      {
        [[maybe_unused]] const auto res = CheckGuardHandler::handleAssertFailure(data);

        throw std::runtime_error(std::string{data.condition}.c_str());
        return diag::FailureAction::None;
      }

    };    
  }  // namespace

  CheckGuardBase::~CheckGuardBase()
  {
    diag::setAssertHandler(std::move(prevHandler));
  }

  CheckGuard::CheckGuard()
  {
    diag::setAssertHandler(std::make_unique<CheckGuardHandler>(*this), &prevHandler);
  }

  CheckGuard::~CheckGuard() = default;

  CheckGuardThrowOnFailure::CheckGuardThrowOnFailure()
  {
    diag::setAssertHandler(std::make_unique<ThrowCheckGuardHandler>(*this), &prevHandler);
  }

  CheckGuardThrowOnFailure::~CheckGuardThrowOnFailure() = default;

}  // namespace my::test
