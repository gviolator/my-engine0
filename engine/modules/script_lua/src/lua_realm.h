// #my_engine_source_file
#pragma once

#include "my/rtti/rtti_impl.h"
#include "my/script/realm.h"
#include "my/containers/intrusive_list.h"
namespace my::script {

/**
*/
class LuaRealm final : public IRealm, public IntrusiveListNode<LuaRealm>
{
    MY_RTTI_CLASS(my::script::LuaRealm, IRealm)
public:
    LuaRealm();
    ~LuaRealm();

    lua_State* GetLua() const
    {
        return m_lua;
    }

private:
    class InvocationGuardImpl;

    static int NativeResolveImportPath(lua_State* l) noexcept;
    static int NativeLoadModule(lua_State* l) noexcept;

    /**
        @brief keeps lua's call result on stack.
    */
    Result<> ExecuteFileInternal(const io::FsPath& filePath);

    InvokeResult Execute(std::span<const std::byte> program, const char* chunkName) override;

    InvokeResult ExecuteFile(const io::FsPath& path) override;

    Result<> Compile(io::IStream& programSource, io::IStream& compilationOutput) override;

    void RegisterClass(ClassDescriptorPtr classDescriptor) override;

    Result<std::unique_ptr<IDispatch>> CreateClassInstance(std::string_view className) override;

    void EnableDebug(bool enableDebug) override;

    script_detail::InvocationGuardHandlePtr OpenInvocationScope(InvokeOptsFlag opts) override;

    template <typename Call>
    InvokeResult ExecuteImpl(Call call);

    lua_State* m_lua = nullptr;
    const InvocationGuardImpl* m_currentInvocationGuard = nullptr;
};
}  // namespace my::script
