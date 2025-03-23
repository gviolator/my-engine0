// #my_engine_source_header
#include "my/test/helpers/check_guard.h"

#include "my/diag/check_handler.h"

namespace my::test
{
  namespace
  {
    class CheckGuardHandler : public diag::ICheckHandler
    {
    public:
      CheckGuardHandler(CheckGuardBase& guard) :
          m_guard(guard)
      {
      }

      diag::FailureActionFlag handleCheckFailure(const diag::FailureData& data) override
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

      diag::FailureActionFlag handleCheckFailure(const diag::FailureData& data) override
      {
        [[maybe_unused]] const auto res = CheckGuardHandler::handleCheckFailure(data);

        throw std::runtime_error(std::string{data.condition}.c_str());
        return diag::FailureAction::None;
      }

    };    
  }  // namespace

  CheckGuardBase::~CheckGuardBase()
  {
    diag::setCheckHandler(std::move(prevHandler));
  }

  CheckGuard::CheckGuard()
  {
    diag::setCheckHandler(std::make_unique<CheckGuardHandler>(*this), &prevHandler);
  }

  CheckGuard::~CheckGuard() = default;

  CheckGuardThrowOnFailure::CheckGuardThrowOnFailure()
  {
    diag::setCheckHandler(std::make_unique<ThrowCheckGuardHandler>(*this), &prevHandler);
  }

  CheckGuardThrowOnFailure::~CheckGuardThrowOnFailure() = default;

}  // namespace my::test
