// #my_engine_source_header

#include "my/test/helpers/runtime_guard.h"

#include "my/async/async_timer.h"
#include "my/async/executor.h"
#include "my/async/thread_pool_executor.h"
#include "my/runtime/disposable.h"
#include "my/runtime/internal/runtime_component.h"
#include "my/utils/scope_guard.h"

namespace my::test
{
  class RuntimeGuardImpl final : public RuntimeGuard
  {
  public:
    RuntimeGuardImpl()
    {
      using namespace my::async;

      ITimerManager::setDefaultInstance();
      m_defaultExecutor = createThreadPoolExecutor(4);
      Executor::setDefault(m_defaultExecutor);
    }

    ~RuntimeGuardImpl()
    {
      resetInternal();
    }

    void reset() override
    {
      resetInternal();
    }

  private:
    void resetInternal()
    {
      using namespace std::chrono_literals;
      using namespace my::async;

      scope_on_leave
      {
        Executor::setDefault(nullptr);
        ITimerManager::releaseInstance();
      };

      std::vector<IRttiObject*> components;
      if (ITimerManager::hasInstance())
      {
        components.push_back(&ITimerManager::getInstance());
      }

      components.push_back(m_defaultExecutor.get());

      for (auto* const component : components)
      {
        if (auto* const disposable = component->as<IDisposable*>())
        {
          disposable->dispose();
        }
      }

      const auto componentInUse = [](IRttiObject* component)
      {
        IRuntimeComponent* const runtimeComponent = component ? component->as<IRuntimeComponent*>() : nullptr;
        if (!runtimeComponent)
        {
          return false;
        }

        if (runtimeComponent->hasWorks())
        {
          return true;
        }

        const auto* const refCounted = runtimeComponent->as<const IRefCounted*>();
        return refCounted && refCounted->getRefsCount() > 1;
      };

      const auto anyComponentInUse = [&components, componentInUse]
      {
        return std::any_of(components.begin(), components.end(), componentInUse);
      };

      while (anyComponentInUse())
      {
        std::this_thread::sleep_for(50ms);
      }
    }

    async::Executor::Ptr m_defaultExecutor;
  };

  RuntimeGuard::Ptr RuntimeGuard::create()
  {
    return std::make_unique<RuntimeGuardImpl>();
  }
}  // namespace my::test
