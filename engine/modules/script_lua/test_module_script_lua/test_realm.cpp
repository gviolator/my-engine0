// #my_engine_source_file
#include <chrono>

#include "in_mem_filesystem.h"
#include "my/app/application.h"
#include "my/app/application_delegate.h"
#include "my/dispatch/closure_value.h"
#include "my/io/virtual_file_system.h"
#include "my/memory/buffer.h"
#include "my/memory/mem_base.h"
#include "my/memory/runtime_stack.h"
#include "my/rtti/rtti_impl.h"
#include "my/script/realm.h"
#include "my/script/script_manager.h"
#include "my/serialization/runtime_value_builder.h"
#include "my/service/service_provider.h"

using namespace my::my_literals;
using namespace my::script;
using namespace std::chrono_literals;

namespace my::test
{

    class AppDelegate final : public ApplicationInitDelegate
    {
        MY_REFCOUNTED_CLASS(AppDelegate, ApplicationInitDelegate)
    public:
        Result<> configureApplication() override
        {

            return ResultSuccess;
        }

        Result<> registerApplicationServices() override
        {
            // Register application services here if needed
            return ResultSuccess;
        }
    };

    /**
     * @brief Base class for script realm tests.
     * This class provides common functionality for testing script realms.
     */
    class TestRealmBase
    {
    protected:
        void startupApplication()
        {
            m_app = createApplication(rtti::createInstance<AppDelegate>());
            m_app->startupOnCurrentThread().ignore();

            getServiceProvider().get<io::IVirtualFileSystem>().mount("/prog", m_contentFs).ignore();
            getServiceProvider().get<ScriptManager>().addScriptSearchPath("/prog");
            getServiceProvider().get<ScriptManager>().addScriptFileExtension(".lua");

            m_realm = createRealm();
        }

        void shutdownApplication()
        {
            if (m_app)
            {
                m_app->stop();
                while (m_app->step())
                {
                    std::this_thread::sleep_for(10ms);
                }
                m_app.reset();
            }
            m_realm.reset();
        }

        void addContent(const io::FsPath& path, const std::string_view content)
        {
            m_contentFs->addContent(path, fromStringView(content).toReadOnly());
        }

        static script::RealmPtr createRealm()
        {
            const ClassDescriptorPtr klass = getServiceProvider().findClass<script::Realm>();
            if (!klass)
            {
                return nullptr;
            }

            UniPtr<IRttiObject> ptr = *klass->getConstructor()->invoke(nullptr, {});
            return ptr.release<script::RealmPtr>();
        }

        ApplicationPtr m_app;
        script::RealmPtr m_realm;
        Ptr<io::InMemoryFileSystem> m_contentFs = rtti::createInstance<io::InMemoryFileSystem>();

        
    private:
        const RuntimeStackGuard m_stackGuard{2_Mb};  // Initialize runtime stack guard with 2 MB
    };

    /**
     * @brief Test class for script realm return values.
     * This class tests the return values from scripts executed in the script realm.
     */
    class TestRealmReturnValues : public TestRealmBase,
                                  public testing::TestWithParam<script::InvokeOptsFlag>
    {
    protected:
        void SetUp() override
        {
            startupApplication();
        }

        void TearDown() override
        {
            shutdownApplication();
        }

        template <typename T, typename U>
        void checkPrimitiveValue(script::InvokeResult&& result, const U& expectedValue)
        {
            if (!result)
            {
                FAIL() << result.getError()->getMessage();
            }

            ASSERT_TRUE((*result)->is<T>());
            const Result<U> valueResult = runtimeValueCast<U>(*result);
            if (!valueResult)
            {
                FAIL() << "Bad value cast:" << valueResult.getError()->getDiagMessage();
            }

            ASSERT_EQ(expectedValue, *valueResult);
        }
    };

    /**
     * @brief Test class for script realm functionality.
     */
    class TestRealm : public TestRealmBase,
                      public testing::Test
    {
    protected:
        void SetUp() override
        {
            startupApplication();
        }

        void TearDown() override
        {
            shutdownApplication();
        }
    };

    /**
     * @brief Test for creating a script realm.
     * This test verifies that a script realm can be created successfully.
     */
    TEST_F(TestRealm, CreateRealm)
    {
        ASSERT_TRUE(m_realm);
    }

    TEST_F(TestRealm, RequireBasic)
    {
        constexpr std::string_view module1 = R"--(
                local exports = {};

                local moduleValue = 'one'
                function exports.setValue(val)
                    moduleValue = val;
                end

                function exports.showValue()
                    print(moduleValue)
                end

                function exports.getValue()
                    return moduleValue;
                end

                return exports
            )--";

        addContent("module1.lua", module1);

        //script::InvokeOptsFlag opts = script::InvokeOpts::KeepResult;
        script_invoke_scope(*m_realm);

        InvokeResult result = execute(*m_realm, "return require('module1')", nullptr);
        ASSERT_TRUE(result);
        ASSERT_TRUE((*result)->is<Dictionary>());

        Dictionary& module = (*result)->as<Dictionary&>();
        ASSERT_TRUE(module.containsKey("getValue"));
        ASSERT_TRUE(module.containsKey("setValue"));
        ASSERT_TRUE(module.containsKey("showValue"));



        
        // Call the module's function
        // module.callMethod("setValue", {makeValueCopy("two")});
        // InvokeResult valueResult = module.callMethod("getValue", {});
        // ASSERT_TRUE(valueResult);
        // ASSERT_TRUE((*valueResult)->is<StringValue>());
        // ASSERT_EQ((*valueResult)->as<StringValue&>().getString(), "two");
    }
  

    /**
     * @brief Test for returning primitive values from a script.
     * This test verifies that primitive values can be returned from a script executed in the script realm.
     */
    TEST_P(TestRealmReturnValues, ReturnPrimitive)
    {
        const script::InvokeOptsFlag opts = GetParam();
        script_invoke_scope(*m_realm, opts);
        {
            InvokeResult result = execute(*m_realm, "return 'text'", nullptr);
            ASSERT_NO_FATAL_FAILURE(checkPrimitiveValue<StringValue>(std::move(result), std::string{"text"}));
        }

        {
            InvokeResult result = execute(*m_realm, "return 77", nullptr);
            ASSERT_NO_FATAL_FAILURE(checkPrimitiveValue<IntegerValue>(std::move(result), 77));
        }

        {
            InvokeResult result = execute(*m_realm, "return 99.0", nullptr);
            ASSERT_NO_FATAL_FAILURE(checkPrimitiveValue<FloatValue>(std::move(result), 99.0));
        }

        {
            InvokeResult result = execute(*m_realm, "return true", nullptr);
            ASSERT_NO_FATAL_FAILURE(checkPrimitiveValue<BooleanValue>(std::move(result), true));
        }

        {
            InvokeResult result = execute(*m_realm, "return false", nullptr);
            ASSERT_NO_FATAL_FAILURE(checkPrimitiveValue<BooleanValue>(std::move(result), false));
        }
    }

    /**
     * @brief Test for returning and calling a closure in a script.
     * This test verifies that a closure can be returned from a script and called with parameters.
     */
    TEST_P(TestRealmReturnValues, ReturnAndCallClosure)
    {
        constexpr std::string_view program =
            R"--(
                local valueAdd = 10;
                return function (a, b)
                    return a + b + valueAdd
                end
            )--";

        const script::InvokeOptsFlag opts = GetParam();
        script_invoke_scope(*m_realm, opts);

        constexpr int Arg1 = 50;
        constexpr int Arg2 = 7;
        constexpr int ExpectedResult = Arg1 + Arg2 + 10;

        InvokeResult closure = execute(*m_realm, program, nullptr);
        ASSERT_TRUE(closure);
        ASSERT_TRUE((*closure)->is<ClosureValue>());

        {
            script_invoke_scope(*m_realm, opts);
            ClosureValue& callable = (*closure)->as<ClosureValue&>();
            InvokeResult callResult = callable.invoke({makeValueCopy(Arg1), makeValueCopy(Arg2)});
            ASSERT_TRUE(callResult);
            Ptr<> result = *callResult;
            ASSERT_TRUE(result);
            ASSERT_TRUE(result->is<IntegerValue>());
            ASSERT_EQ(result->as<const IntegerValue&>().getInt64(), ExpectedResult);
        }
    }

    TEST_F(TestRealm, Temp)
    {
#if 0
        {
            auto& manager = getServiceProvider().get<script::ScriptManager>();
            manager.addScriptFileExtension(".lua");
            manager.addScriptSearchPath("/prog");
        }

        constexpr std::string_view program = R"--(
            local module = require('module1')
            local moduleSame = require('module1')

            moduleSame.setValue("two")
            module.showValue()
            print('exec completed')

        )--";

        // mylog("Top ({})",

        auto result = realm->execute({reinterpret_cast<const std::byte*>(program.data()), program.size()}, {}, "test");
        if (!result)
        {
            std::cerr << result.getError()->getDiagMessage() << std::endl;
        }

        constexpr std::string_view program2 = "return require('module1')";

        auto result2 = realm->execute({reinterpret_cast<const std::byte*>(program2.data()), program2.size()}, script::InvokeOpts::KeepResult, "test2");
        if (!result2)
        {
            std::cerr << result2.getError()->getDiagMessage() << std::endl;
        }

        ASSERT_TRUE(result2);

        // // Example of invoking a global function in the realm
        // const script::InvocationScopeGuard iGuard{*realm};

        // auto ires = realm->invokeGlobal("interop_appMainStep", {makeValueCopy(0.1f)}, {});
        // ASSERT_TRUE(ires && *ires) << "Failed to invoke interop_appMainStep";

        // std::cout << "App step: (" << *runtimeValueCast<bool>(*ires) << ")\n";
#endif
    }

    INSTANTIATE_TEST_SUITE_P(Default, TestRealmReturnValues, testing::Values(script::InvokeOptsFlag{script::InvokeOpts::KeepResult}, script::InvokeOptsFlag{script::InvokeOpts{}}));

}  // namespace my::test