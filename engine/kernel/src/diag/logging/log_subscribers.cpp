#if 0
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <nau/app/application.h>
#include <nau/async/task_collection.h>

#include <iostream>

// #include "nau/diag/assertion.h"
#include "file_helper.h"
#include "nau/app/background_work_service.h"
#include "nau/diag/log_subscribers.h"
#include "nau/diag/logging.h"
#include "nau/service/service_provider.h"

template <>
struct fmt::formatter<my::diag::LogLevel, char> : fmt::formatter<const char*, char>
{
    auto format(my::diag::LogLevel level, fmt::buffered_context<char>& ctx) const
    {
        switch (level)
        {
            case my::diag::LogLevel::Debug:
                return formatter<const char*, char>::format("Debug", ctx);
                break;
            case my::diag::LogLevel::Info:
                return formatter<const char*, char>::format("Info", ctx);
                break;
            case my::diag::LogLevel::Warning:
                return formatter<const char*, char>::format("Warning", ctx);
                break;
            case my::diag::LogLevel::Error:
                return formatter<const char*, char>::format("Error", ctx);
                break;
            case my::diag::LogLevel::Critical:
                return formatter<const char*, char>::format("Critical", ctx);
                break;
            case my::diag::LogLevel::Verbose:
                return formatter<const char*, char>::format("Verbose", ctx);
                break;
        };
        return formatter<const char*, char>::format("unknown log level", ctx);
    }
};

namespace my::diag
{
    namespace
    {
        std::string tagsToString(const std::vector<std::string>& tags)
        {
            // TODO use format
            std::string output;
            if (tags.size() > 0)
            {
                output = tags[0];
            }
            for (size_t i = 1; i < tags.size(); i++)
            {
                output = my::utils::format(u8"{}, {}", output, tags[i].c_str()).c_str();
            }
            return output;
        }

        std::string timeToString(time_t time)
        {
            // TODO use format
            std::string timeString{64};
            strftime((char*)timeString.data(), 64, "%F %H:%M:%S", std::localtime(&time));
            return timeString;
        }
    }  // namespace

    struct DefaultMessageFormatter
    {
        static std::string format(const LoggerMessage& message)
        {
            return (const char*)my::string::format(u8"[{}][{}][{}][{}]: {}\n",
                                                    message.index,
                                                    timeToString(message.time),
                                                    message.level,
                                                    tagsToString(message.tags),
                                                    message.data.data())
                .c_str();
        }
    };

    class ConioLogSubscriber final : public ILogSubscriber
    {
    public:
        void processMessage(const LoggerMessage& message) override
        {
            std::cout
                << DefaultMessageFormatter::format(message).c_str()
                << std::endl;
        }
    };

    class FileLogSubscriber final : public ILogSubscriber
    {
        spdlog::details::file_helper m_file;
        my::threading::SpinLock m_mutex;
        async::TaskCollection m_fileWriteTasks;
        std::vector<LoggerMessage> m_pendingMessages;

        std::string fileNameFormat(std::string_view filename)
        {
            std::string dateString{64};
            time_t time = std::time({});
            strftime((char*)dateString.data(), 64, "%F.%H-%M-%S", std::localtime(&time));

            return std::string((const char*)my::string::format(u8"{}.{}.log", filename, dateString.data()).c_str());
        }

    public:
        FileLogSubscriber(std::string_view filename, const spdlog::file_event_handlers& event_handlers = {}) :
            m_file(fileNameFormat(filename), event_handlers)
        {
        }

        void processMessage(const LoggerMessage& message) override
        {
            lock_(m_mutex);

            // TODO: at this moment processMessage can be called while services system is gone.
            if (!hasServiceProvider() || !getServiceProvider().has<io::FileSystem>() || !getApplication().hasExecutor())
            {
                m_pendingMessages.emplace_back(message);
                return;
            }

            // NAU-1861 quick fix
            if (!m_file.isOpen())
            {
                m_file.open();
            }

            if (m_pendingMessages.size() != 0)
            {
                m_pendingMessages.emplace_back(message);

                auto writeToFile = [](spdlog::details::file_helper* file, std::vector<LoggerMessage> messages) -> my::async::Task<>
                {
                    BackgroundWorkService* const workService = hasServiceProvider() ? getServiceProvider().find<BackgroundWorkService>() : nullptr;
                    if (workService)
                    {
                        co_await workService->getExecutor();
                    }
                    for (auto& message : messages)
                    {
                        file->write(DefaultMessageFormatter::format(message));
                    }
                    file->flush();
                };

                m_fileWriteTasks.push(writeToFile(&m_file, std::move(m_pendingMessages)));
            }
            else
            {
                auto writeToFile = [](spdlog::details::file_helper* file, LoggerMessage message) -> async::Task<>
                {
                    BackgroundWorkService* const workService = hasServiceProvider() ? getServiceProvider().find<BackgroundWorkService>() : nullptr;
                    if (workService)
                    {
                        co_await workService->getExecutor();
                    }
                    file->write(DefaultMessageFormatter::format(message));
                    file->flush();
                };

                m_fileWriteTasks.push(writeToFile(&m_file, message));
            }
        }
        ~FileLogSubscriber() override
        {
            async::Task<> finalizer = [](FileLogSubscriber& self) -> async::Task<>
            {
                if (auto executor = async::Executor::getDefault())
                {
                    co_await executor;
                    // if there is no default executor, then all tasks are expected to have completed as well (this should be guaranteed by the runtime)
                    co_await self.m_fileWriteTasks.awaitCompletion();
                }

                self.m_file.flush();
                self.m_file.close();
            }(*this);

            finalizer.setContinueOnCapturedExecutor(false);
            async::wait(finalizer);
        }
    };

    ILogSubscriber::Ptr createConioOutputLogSubscriber()
    {
        return std::make_unique<ConioLogSubscriber>();
    }

    ILogSubscriber::Ptr createFileOutputLogSubscriber(std::string_view filename)
    {
        return std::make_unique<FileLogSubscriber>(filename);
    }
}  // namespace my::diag

#endif