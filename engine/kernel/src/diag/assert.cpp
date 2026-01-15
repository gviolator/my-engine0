// #my_engine_source_file

#include "my/diag/assert.h"

// #include "my/debug/debugger.h"
#include "my/diag/assert_handler.h"
#include "my/diag/logging.h"
#include "my/memory/singleton_memop.h"
#include "my/utils/scope_guard.h"

namespace my::diag
{
    namespace
    {
        class DefaultAssertHandler final : public diag::IAssertHandler
        {
        public:
            MY_SINGLETON_MEMOPS(DefaultAssertHandler)

            diag::FailureActionFlag handleAssertFailure(const diag::FailureData& data) override
            {
                using namespace my::diag;

                std::string message =
                    data.message.empty() ? std::format("ASSERT FAIL({})", data.condition) : std::format("ASSERT FAIL({})\n{}", data.condition, data.message);

                const LogLevel level = data.kind == AssertionKind::Fatal ? LogLevel::Critical : LogLevel::Error;
                getDefaultLogger().log(level, data.source, nullptr, std::move(message));

                return data.kind == AssertionKind::Fatal ? FailureAction::DebugBreak | FailureAction::Abort : FailureAction::DebugBreak;
            }
        };

        AssertHandlerPtr& getAssertHandler()
        {
            static AssertHandlerPtr s_handler = std::make_unique<DefaultAssertHandler>();
            return (s_handler);
        }

    }  // namespace

    void setAssertHandler(AssertHandlerPtr newHandler, AssertHandlerPtr* prevHandler)
    {
        if (prevHandler)
        {
            *prevHandler = std::move(getAssertHandler());
        }

        getAssertHandler() = std::move(newHandler);
    }

    IAssertHandler* getCurrentAssertHandler()
    {
        return getAssertHandler().get();
    }

}  // namespace my::diag

namespace my::diag_detail
{
    diag::FailureActionFlag raiseFailure(diag::AssertionKind kind, diag::SourceInfo source, std::string_view condition, std::string_view message)
    {
        using namespace my::diag;

        static thread_local unsigned threadRaiseFailureCounter = 0;

        if (threadRaiseFailureCounter > 0)
        {
            MY_PLATFORM_BREAK;
            MY_PLATFORM_ABORT;
        }

        ++threadRaiseFailureCounter;

        scope_on_leave
        {
            --threadRaiseFailureCounter;
        };

        if (auto& handler = diag::getAssertHandler())
        {
            const FailureData failureData{
                kind,
                source,
                condition,
                message};

            return handler->handleAssertFailure(failureData);
        }

        return kind == AssertionKind::Default ? FailureAction::DebugBreak : (FailureAction::DebugBreak | FailureAction::Abort);
    }

}  // namespace my::diag_detail
