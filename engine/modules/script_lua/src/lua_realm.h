// #my_engine_source_file
#pragma once

#include "my/rtti/rtti_impl.h"
#include "my/script/realm.h"


namespace my::script
{
    class LuaRealm final : public Realm
    {
        MY_RTTI_CLASS(my::script::LuaRealm, Realm)
    public:
        LuaRealm();
        ~LuaRealm();

    private:
        class InvocationGuardImpl;

        static int NativeResolveImportPath(lua_State* l) noexcept;
        static int NativeLoadModule(lua_State* l) noexcept;

        /**
            @brief keeps lua's call result on stack.
        */
        Result<> executeFileInternal(const io::FsPath& filePath);

        InvokeResult execute(std::span<const std::byte> program, const char* chunkName) override;

        InvokeResult executeFile(const io::FsPath& path) override;

        Result<> compile(io::IStream& programSource, io::IStream& compilationOutput) override;

        void registerClass(ClassDescriptorPtr classDescriptor) override;

        Result<std::unique_ptr<IDispatch>> createClassInstance(std::string_view className) override;

        script_detail::InvocationGuardHandlePtr openInvocationScope(InvokeOptsFlag opts) override;

        template<typename Call>
        InvokeResult executeImpl(Call call);

        lua_State* m_lua = nullptr;
        const InvocationGuardImpl* m_currentInvocationGuard = nullptr;
    };
}  // namespace my::script
