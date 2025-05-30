// #my_engine_source_file

#include "my/diag/assert.h"

#include "my/debug/debugger.h"
#include "my/diag/assert_handler.h"
#include "my/memory/singleton_memop.h"
#include "my/utils/scope_guard.h"

namespace my::diag
{
    namespace
    {

        AssertHandlerPtr& getCheckHandlerRef()
        {
            static AssertHandlerPtr s_handler;
            return (s_handler);
        }

    }  // namespace

    void setAssertHandler(AssertHandlerPtr newHandler, AssertHandlerPtr* prevHandler)
    {
        if (prevHandler)
        {
            *prevHandler = std::move(getCheckHandlerRef());
        }

        getCheckHandlerRef() = std::move(newHandler);
    }

    IAssertHandler* getCurrentAssertHandler()
    {
        return getCheckHandlerRef().get();
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

        if (auto& handler = diag::getCheckHandlerRef())
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

    class DefaultCheckHandler final : public diag::IAssertHandler
    {
    public:
        MY_DECLARE_SINGLETON_MEMOP(DefaultCheckHandler)

        diag::FailureActionFlag handleAssertFailure(const diag::FailureData& data) override
        {
            using namespace my::diag;

            std::string message;

            if (!data.message.empty())
            {
                message = std::format("Failed \"{}\". At [{}] {}({}). \nMessage: \"{}\"",
                                      data.condition,
                                      data.source.functionName,
                                      data.source.filePath,
                                      data.source.line.value_or(0),
                                      data.message.data());
            }
            else
            {
                message = std::format("Failed \"{}\". At [{}] {}({}).",
                                      data.condition,
                                      data.source.functionName,
                                      data.source.filePath.data(),
                                      data.source.line.value_or(0));
            }

            /*  if (hasLogger())
              {
                  getLogger().logMessage(LogLevel::Critical,
                                         {"Fatal"},
                                         data.source,
                                         my::utils::format(message.data()));
              }*/

            return data.kind == AssertionKind::Fatal ? FailureAction::DebugBreak | FailureAction::Abort : FailureAction::DebugBreak;
        }
    };

}  // namespace my::diag_detail

namespace my::diag
{
    AssertHandlerPtr createDefaultDeviceError()
    {
        return std::make_unique<diag_detail::DefaultCheckHandler>();
    }

}  // namespace my::diag
