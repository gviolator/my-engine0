#if 0
#include <EASTL/vector.h>

#include <concepts>

#include "nau/math/math.h"
#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/utils/type_utility.h"

namespace my::math
{

    float getVec3Value(const vec3& v, size_t index)
    {
        return v.getElem(index);
    }

    void setVec3Value(vec3& v, size_t index, float elem)
    {
        v.setElem(index, elem);
    }

}  // namespace my::math

namespace nau
{
    struct FooObject1
    {
        int field1 = 1;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(field1))
    };
    /*
        template <typename T>
        class VectorCollection : public RuntimeCollection
        {
            using ThisType = VectorCollection<T>;
            MY_REFCOUNTED_CLASS(ThisType, RuntimeCollection)

        public:
            VectorCollection(T& collection) :
                m_collection(m_collection)
            {
            }

            bool isMutable() const override
            {
                return true;
            }

            size_t getSize() const override
            {
                return m_collection.size();
            }

            RuntimeValuePtr getAt(size_t index) const override
            {
                MY_DEBUG_CHECK(index < this->getSize());

                return makeValueRef(m_collection[index]);  // this->makeChildValue(makeValueRef(el));
            }

            Result<> setAt(size_t index, RuntimeValuePtr value) override
            {
                MY_DEBUG_CHECK(value);
                MY_DEBUG_CHECK(index < this->getSize());

                decltype(auto) el = m_collection.emplace_back();
                return RuntimeValue::assign(makeValueRef(el), std::move(value));
            }

            void clear()
            {
            }

            Result<> append(RuntimeValuePtr value) override
            {
                decltype(auto) element = m_collection.emplace_back();
                return RuntimeValue::assign(makeValueRef(element), value);
            }

        private:
            T& m_collection;
        };

    */
}  // namespace nau

#if 0
namespace my::math
{
    template <typename T>
    class VecRuntimeValue : public RuntimeReadonlyCollection,
                            public RuntimeReadonlyDictionary
    {
        MY_REFCOUNTED_CLASS(my::math::VecRuntimeValue<T>, RuntimeReadonlyCollection, RuntimeReadonlyDictionary)

    public:
        using Type = std::decay_t<T>;
        static constexpr bool IsReference = std::is_reference_v<T>;
        static constexpr bool IsMutable = !std::is_const_v<T>;

        VecRuntimeValue(T vec)
            : m_vec(vec)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        size_t getSize() const override
        {
            return getKeysArray().size();
        }

        RuntimeValuePtr getAt(size_t index) override
        {
            MY_DEBUG_CHECK(index < getSize());
            return makeValueCopy(m_vec.getElem(index));
        }

        Result<> setAt(size_t index, const RuntimeValuePtr& value) override
        {
            MY_DEBUG_CHECK(index < getSize());
            auto castResult = runtimeValueCast<float>(value);
            CheckResult(castResult)

            m_vec.setElem(index, *castResult);
            return ResultSuccess;
        }

        std::string_view getKey(size_t index) const override
        {
            MY_DEBUG_CHECK(index < getSize());

            return getKeysArray()[index];
        }

        RuntimeValuePtr getValue(std::string_view key) override
        {
            if(const auto index = getElementIndex(key))
            {
                const float elem = m_vec.getElem(*index);
                return makeValueCopy(elem);
            }

            return nullptr;
        }

        Result<> setValue(std::string_view key, const RuntimeValuePtr& value) override
        {
            if(const auto index = getElementIndex(key))
            {
                const float elem = *runtimeValueCast<float>(value);
                m_vec.setElem(static_cast<int>(*index), elem);

                return ResultSuccess;
            }

            return MakeError("Unknown vec elem ({})", key);
        }

        bool containsKey(std::string_view key) const override
        {
            const auto keys = getKeysArray();

            return std::any_of(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });
        }

    private:
        static consteval inline auto getKeysArray()
        {
            return std::array<const char*, 4>{"x", "y", "z", "w"};
        }

        static inline std::optional<size_t> getElementIndex(std::string_view key)
        {
            const auto keys = getKeysArray();
            auto index = std::find_if(keys.begin(), keys.end(), [key](const char* fieldName)
            {
                return strings::icaseEqual(key, fieldName);
            });

            if (index == keys.end())
            {
                MY_FAILURE("Invalid field ({})", key);
                return std::nullopt;
            }

            return static_cast<size_t>(index - keys.begin());
        }

        Type m_vec;
    };

}  // namespace my::math

#endif

namespace my::test
{

    struct MyObject
    {
        float x;
        float y;
        std::optional<unsigned> z;
        std::vector<unsigned> m_values;

        NAU_CLASS_FIELDS(
            CLASS_FIELD(x),
            CLASS_FIELD(y),
            CLASS_NAMED_FIELD(m_values, "values"),
            CLASS_FIELD(z))
    };

    TEST(RuntimeValueCustoms, Test1)
    {
        const char* j = R"-(
            {
                "x": 77.0,
                "y": null,
                "z": 9
            }
        )-";

        math::vec3 v;
        auto vv = Vectormath::SSE::makeValueRef(v);
        constexpr bool IsVec = math::LikeVec4<math::Quat>;

        auto v2 = serialization::JsonUtils::parse<math::vec3>(j);

        auto v4 = serialization::JsonUtils::parse<math::vec4>(j);

        auto v5 = serialization::JsonUtils::parse<math::Quat>(j);

        std::cout << "Done\n";
    }

}  // namespace my::test
#endif
