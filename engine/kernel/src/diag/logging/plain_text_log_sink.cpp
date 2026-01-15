// #my_engine_source_file

#include "my/diag/log_sinks.h"
#include "my/memory/runtime_stack.h"
#include "my/threading/lock_guard.h"
#include "my/threading/spin_lock.h"

namespace my::diag
{
    class PlainTextLogSink : public LogSink
    {
        MY_REFCOUNTED_CLASS(my::diag::PlainTextLogSink, LogSink)
    public:
        PlainTextLogSink(io::StreamPtr&& outStream) :
            m_outStream{std::move(outStream)}

        {
        }

    private:
        void log(const LogMessage&, std::string_view formattedMessage) override
        {
            if (formattedMessage.empty())
            {
                return;
            }

            const std::lock_guard lock(m_mutex);
            m_outStream->write(reinterpret_cast<const std::byte*>(formattedMessage.data()), formattedMessage.size()).ignore();
        }

        const io::StreamPtr m_outStream;
        threading::SpinLock m_mutex;
    };

    LogSinkPtr createPlainTextSink(io::StreamPtr stream)
    {
        return rtti::createInstance<PlainTextLogSink>(std::move(stream));
    }
}  // namespace my::diag
