// #my_engine_source_file

#include "my/diag/log_sinks.h"
#include "my/memory/runtime_stack.h"
#include "my/threading/lock_guard.h"
#include "my/threading/spin_lock.h"


namespace my::diag
{
    namespace
    {
        constexpr const char* Color_Reset = "\033[0m";
        constexpr const char* Color_Error = "\033[31m";    // red
        constexpr const char* Color_Verbose = "\033[32m";  // green
        constexpr const char* Color_Warning = "\033[33m";  // yellow

        // #define RESET
        // #define BLACK   "\033[30m"      /* Black */
        // #define RED     "\033[31m"      /* Red */
        // #define GREEN   "\033[32m"      /* Green */
        // #define YELLOW  "\033[33m"      /* Yellow */
        // #define BLUE    "\033[34m"      /* Blue */
        // #define MAGENTA "\033[35m"      /* Magenta */
        // #define CYAN    "\033[36m"      /* Cyan */
        // #define WHITE   "\033[37m"      /* White */
        // #define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
        // #define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
        // #define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
        // #define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
        // #define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
        // #define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
        // #define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
        // #define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
    }  // namespace

    class ConsoleLogSink : public LogSink
    {
        MY_REFCOUNTED_CLASS(my::diag::ConsoleLogSink, LogSink)
    public:
        ConsoleLogSink(io::StreamPtr&& outStream, io::StreamPtr&& errorStream) :
            m_outStream{std::move(outStream)},
            m_errorStream{std::move(errorStream)}

        {
        }

    private:
        void log(const LogMessage& sourceMessage, std::string_view formattedMessage) override
        {
            if (formattedMessage.empty())
            {
                return;
            }

            rtstack_scope;
            const LogLevel level = sourceMessage.level;

            std::basic_string<char, std::char_traits<char>, RtStackStdAllocator<char>> buffer;
            if (level == LogLevel::Error || level == LogLevel::Critical)
            {
                buffer.append(Color_Error);
            }
            else if (level == LogLevel::Warning)
            {
                buffer.append(Color_Warning);
            }
            else if (level == LogLevel::Verbose || level == LogLevel::Debug)
            {
                buffer.append(Color_Verbose);
            }

            buffer.append(formattedMessage);
            if (!buffer.ends_with('\n'))
            {
                buffer.append("\n");
            }

            buffer.append(Color_Reset);

            // stream (if specified) must implements mutual access by it self. Lock only cout/cerr.
            if (level == LogLevel::Critical || level == LogLevel::Error)
            {
                if (!m_errorStream)
                {
                    const std::lock_guard lock(m_mutex);
                    std::cerr << buffer;
                }
                else
                {
                    m_errorStream->write(reinterpret_cast<const std::byte*>(buffer.data()), buffer.size()).ignore();
                }
            }
            else
            {
                if (!m_outStream)
                {
                    const std::lock_guard lock(m_mutex);
                    std::cout << buffer;
                }
                else
                {
                    m_outStream->write(reinterpret_cast<const std::byte*>(buffer.data()), buffer.size()).ignore();
                }
            }
        }

        const io::StreamPtr m_outStream;
        const io::StreamPtr m_errorStream;
        threading::SpinLock m_mutex;
    };

    LogSinkPtr createConsoleSink(io::StreamPtr outStream, io::StreamPtr errorStream)
    {
        return rtti::createInstance<ConsoleLogSink>(std::move(outStream), std::move(errorStream));
    }
}  // namespace my::diag
