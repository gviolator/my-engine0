// #my_engine_source_file
#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include "my/dispatch/class_descriptor.h"
#include "my/dispatch/dispatch.h"
#include "my/io/fs_path.h"
#include "my/io/stream.h"
#include "my/kernel/kernel_config.h"
#include "my/memory/buffer.h"
#include "my/utils/preprocessor.h"
#include "my/utils/result.h"

namespace my::script_detail
{
    struct InvocationGuardHandle
    {
        virtual ~InvocationGuardHandle() = default;
    };

    using InvocationGuardHandlePtr = std::unique_ptr<InvocationGuardHandle>;
}  // namespace my::script_detail

namespace my::script
{
    enum class InvokeOpts
    {
        KeepResult = FlagValue(1),
        MultiResult = FlagValue(2)
    };

    MY_DEFINE_TYPED_FLAG(InvokeOpts)

    class MY_KERNEL_EXPORT InvocationScopeGuard
    {
    public:
        // OnStack by default
        InvocationScopeGuard(struct IRealm&, InvokeOptsFlag opts = {});
        InvocationScopeGuard(const InvocationScopeGuard&) = delete;
        InvocationScopeGuard(InvocationScopeGuard&&) = delete;
        InvocationScopeGuard() = delete;
        ~InvocationScopeGuard();

        InvocationScopeGuard& operator=(const InvocationScopeGuard&) = delete;

    private:
        const script_detail::InvocationGuardHandlePtr m_handle;
    };

    using InvokeResult = Result<Ptr<>>;

    /**
     */
    struct MY_ABSTRACT_TYPE IRealm : IRttiObject
    {
        MY_INTERFACE(my::script::IRealm, IRttiObject)

        virtual InvokeResult Execute(std::span<const std::byte> program, const char* chunkName) = 0;

        virtual InvokeResult ExecuteFile(const io::FsPath& path) = 0;

        virtual Result<> Compile(io::IStream& programSource, io::IStream& compilationOutput) = 0;

        virtual void RegisterClass(ClassDescriptorPtr classDescriptor) = 0;

        // virtual InvokeResult invokeGlobal(const char* callableName, DispatchArguments args, InvokeOptsFlag opts) = 0;

        virtual Result<std::unique_ptr<IDispatch>> CreateClassInstance(std::string_view className) = 0;

        virtual void EnableDebug(bool enableDebug) = 0;

    protected:
        virtual script_detail::InvocationGuardHandlePtr OpenInvocationScope(InvokeOptsFlag) = 0;

        friend class InvocationScopeGuard;
    };

    using RealmPtr = std::unique_ptr<IRealm>;

    MY_KERNEL_EXPORT
    Result<Buffer> CompileToBuffer(IRealm&, io::IStream& stream);

    inline InvokeResult Execute(IRealm& realm, const std::string_view program, const char* chunkName)
    {
        const std::span buffer{reinterpret_cast<const std::byte*>(program.data()), program.size()};
        return realm.Execute(buffer, chunkName);
    }

}  // namespace my::script

#define script_invoke_scope(realm, ...)                                 \
    const my::script::InvocationScopeGuard ANONYMOUS_VAR(invokeGuard__) \
    {                                                                   \
        realm, ##__VA_ARGS__                                            \
    }
