// #my_engine_source_file
#include "my/rtti/ptr.h"
#include "my/rtti/rtti_impl.h"
#include "my/rtti/weak_ptr.h"
#include "my/utils/functor.h"
#include "my/utils/uni_ptr.h"

namespace my::test
{
    namespace
    {
        struct MY_ABSTRACT_TYPE MyRefCounted : IRefCounted
        {
            MY_INTERFACE(MyRefCounted, IRefCounted)
        };

        struct MY_ABSTRACT_TYPE ITestInterface1 : virtual IRttiObject
        {
            MY_INTERFACE(ITestInterface1, IRttiObject)
        };

        struct MY_ABSTRACT_TYPE ITestInterface2 : virtual IRttiObject
        {
            MY_INTERFACE(ITestInterface2, IRttiObject)
        };

        struct SimpleBase
        {
            virtual ~SimpleBase() = default;
        };

        class WithDestructor : public SimpleBase
        {
        public:
            WithDestructor() = default;

            template <typename F>
            WithDestructor(F f) :
                m_onDestruct(std::move(f))
            {
            }

            ~WithDestructor()
            {
                if (m_onDestruct)
                {
                    m_onDestruct();
                }
            }

        private:
            Functor<void()> m_onDestruct;
        };

        class TestService12 : public ITestInterface1,
                              public ITestInterface2
        {
            MY_RTTI_CLASS(TestService12, ITestInterface1, ITestInterface2)
        };

        class RcService : public MyRefCounted,
                          public WithDestructor
        {
            MY_REFCOUNTED_CLASS(RcService, MyRefCounted);

        public:
            using WithDestructor::WithDestructor;
        };
    }  // namespace

    /**
     */
    TEST(TestPtr, StdUniquePtrCast)
    {
        std::unique_ptr<ITestInterface1> itf1 = std::make_unique<TestService12>();

        static_assert(!std::is_constructible_v<std::unique_ptr<ITestInterface2>, decltype(itf1)>);

        auto itf2 = rtti::pointer_cast<ITestInterface2>(std::move(itf1));
        static_assert(std::is_same_v<std::unique_ptr<ITestInterface2>, decltype(itf2)>);

        ASSERT_TRUE(itf2);
        ASSERT_FALSE(itf1);

        static_assert(std::is_constructible_v<std::unique_ptr<IRttiObject>, decltype(itf2)>);

        auto itf3 = rtti::pointer_cast<IRttiObject>(std::move(itf2));
        static_assert(std::is_same_v<std::unique_ptr<IRttiObject>, decltype(itf3)>);

        ASSERT_TRUE(itf3);
        ASSERT_FALSE(itf2);
    }

    /**
     */
    TEST(TestPtr, UniqueToUniPtr)
    {
        bool isDestructed = false;
        std::unique_ptr uniquePtr = std::make_unique<WithDestructor>([&isDestructed]
        {
            isDestructed = true;
        });

        {
            UniPtr uniPtr = std::move(uniquePtr);
            ASSERT_FALSE(uniquePtr);
            ASSERT_TRUE(uniPtr);
        }

        ASSERT_TRUE(isDestructed);
    }

    /**
     */
    TEST(TestPtr, UniqueToUniPtrWithCast)
    {
        bool isDestructed = false;
        std::unique_ptr<WithDestructor> uniquePtr = std::make_unique<WithDestructor>([&isDestructed]
        {
            isDestructed = true;
        });

        UniPtr<WithDestructor> uniPtr = std::move(uniquePtr);

        uniPtr.reset();
        ASSERT_TRUE(isDestructed);
    }

    /**
     */
    TEST(TestPtr, UniToUniquePtr)
    {
        UniPtr<WithDestructor> uniPtr;
        bool isDestructed = false;

        {
            std::unique_ptr uniquePtr = std::make_unique<WithDestructor>([&isDestructed]
            {
                isDestructed = true;
            });

            uniPtr = std::move(uniquePtr);
        }

        {
            std::unique_ptr uniquePtr = uniPtr.release<std::unique_ptr<WithDestructor>>();
            ASSERT_FALSE(uniPtr);
            ASSERT_TRUE(uniquePtr);
        }

        ASSERT_TRUE(isDestructed);
    }

    /**
     */
    TEST(TestPtr, MyPtrToUniPtr)
    {
        bool isDestructed = false;
        Ptr service = rtti::createInstance<RcService>([&isDestructed]
        {
            isDestructed = true;
        });

        {
            UniPtr uniPtr = std::move(service);
            ASSERT_FALSE(service);
            ASSERT_TRUE(uniPtr);
        }

        ASSERT_TRUE(isDestructed);
    }

    TEST(TestPtr, MyPtrToUniPtrWithCast)
    {
        bool isDestructed = false;
        auto factory = [&isDestructed]() -> UniPtr<IRttiObject>
        {
            Ptr service = rtti::createInstance<RcService>([&isDestructed]
            {
                isDestructed = true;
            });

            return std::move(service);
        };

        UniPtr uniPtr = factory();
        ASSERT_TRUE(uniPtr);
        uniPtr.reset();
        ASSERT_TRUE(isDestructed);
    }

    /**
     */
    TEST(TestPtr, UniToMyPtr)
    {
        UniPtr<RcService> uniPtr;
        bool isDestructed = false;

        {
            auto service = rtti::createInstance<RcService>([&isDestructed]
            {
                isDestructed = true;
            });

            uniPtr = std::move(service);
            ASSERT_FALSE(service);
            ASSERT_TRUE(uniPtr);
        }

        Ptr ptr = uniPtr.release<Ptr<RcService>>();
        ptr.reset();
        ASSERT_TRUE(isDestructed);
    }

    TEST(TestPtr, UniToMyPtrWithCast)
    {
        bool isDestructed = false;
        UniPtr<IRttiObject> uniPtr = rtti::createInstance<RcService>([&isDestructed]
        {
            isDestructed = true;
        });

        ASSERT_TRUE(uniPtr);

        Ptr ptr = uniPtr.release<Ptr<RcService>>();
        ASSERT_FALSE(uniPtr);
        ASSERT_TRUE(ptr);

        ptr.reset();
        ASSERT_TRUE(isDestructed);
    }

}  // namespace my::test
