// #my_engine_source_file

#include "my/serialization/runtime_value.h"

#include "my/diag/error.h"
#include "my/rtti/rtti_impl.h"
#include "my/serialization/serialization.h"
#include "my/utils/string_utils.h"

namespace my
{

    namespace
    {
        class RuntimeValueRefImpl final : public RuntimeValueRef
        {
            MY_REFCOUNTED_CLASS(my::RuntimeValueRefImpl, RuntimeValueRef)

        public:
            RuntimeValueRefImpl(RuntimeValuePtr& valueRef) :
                m_valueRef(valueRef),
                m_isMutable(true)
            {
            }

            RuntimeValueRefImpl(const RuntimeValuePtr& valueRef) :
                m_valueRef(const_cast<RuntimeValuePtr&>(valueRef)),
                m_isMutable(false)
            {
            }

            bool isMutable() const override
            {
                return m_isMutable;
            }

            void setValue(RuntimeValuePtr value) override
            {
                m_valueRef = std::move(value);
            }

            RuntimeValuePtr getValue() const override
            {
                return m_valueRef;
            }

        private:
            RuntimeValuePtr& m_valueRef;
            const bool m_isMutable;
        };

        std::optional<std::string> lexical_cast(const PrimitiveValue& value)
        {
            if (auto iValue = value.as<const IntegerValue*>())
            {
                return strings::lexicalCast(iValue->getInt64());
            }
            else if (auto fValue = value.as<const FloatValue*>())
            {
                return strings::lexicalCast(fValue->getSingle());
            }
            else if (auto bValue = value.as<const BooleanValue*>())
            {
                return strings::lexicalCast(bValue->getBool());
            }

            return std::nullopt;
        }

        bool lexical_cast(PrimitiveValue& value, const std::string& str)
        {
            if (auto iValue = value.as<IntegerValue*>())
            {
                const int64_t v = str.empty() ? 0 : strings::lexicalCast<int64_t>(str);
                iValue->setInt64(v);
            }
            else if (auto fValue = value.as<FloatValue*>())
            {
                const double v = str.empty() ? 0. : strings::lexicalCast<double>(str);
                fValue->setDouble(v);
            }
            else if (auto bValue = value.as<BooleanValue*>())
            {
                const bool v = str.empty() ? false : strings::lexicalCast<bool>(str);
                bValue->setBool(v);
            }
            else
            {
                return false;
            }

            return true;
        }

        Result<> assignStringValue(StringValue& dstStr, const PrimitiveValue& src, serialization::TypeCoercion typeCoercion)
        {
            if (auto const srcStr = src.as<const StringValue*>())
            {
                auto strBytes = srcStr->getString();
                return dstStr.setString(strBytes);
            }

            const std::optional<std::string> str = typeCoercion == serialization::TypeCoercion::Allow ? lexical_cast(src) : std::nullopt;

            if (str)
            {
                return dstStr.setString(*str);
            }

            return MakeError("String value can be assigned only from other string");
        }

        Result<> assignPrimitiveValue(PrimitiveValue& dst, const PrimitiveValue& src, serialization::TypeCoercion typeCoercion = serialization::TypeCoercion::Allow)
        {
            using namespace my::serialization;

            if (auto const dstStr = dst.as<StringValue*>())
            {
                return assignStringValue(*dstStr, src, typeCoercion);
            }

            const auto tryImplicitCast = [&src, &dst, typeCoercion]
            {
                if (typeCoercion != TypeCoercion::Allow)
                {
                    return false;
                }

                auto const srcStr = src.as<const StringValue*>();
                return srcStr && lexical_cast(dst, srcStr->getString());
            };

            if (auto const dstBool = dst.as<BooleanValue*>())
            {
                if (auto const srcBool = src.as<const BooleanValue*>())
                {
                    dstBool->setBool(srcBool->getBool());
                }
                else if (!tryImplicitCast())
                {
                    return MakeError("Fail to assign boolean value");
                }

                return {};
            }

            if (FloatValue* const dstFloat = dst.as<FloatValue*>())
            {
                if (auto const srcFloat = src.as<const FloatValue*>())
                {
                    dstFloat->setDouble(srcFloat->getDouble());
                }
                else if (auto const srcInt = src.as<const IntegerValue*>())
                {
                    if (dstFloat->getBitsCount() == sizeof(double))
                    {
                        dstFloat->setDouble(static_cast<double>(srcInt->get<int64_t>()));
                    }
                    else
                    {
                        dstFloat->setSingle(static_cast<float>(srcInt->get<int64_t>()));
                    }
                }
                else if (!tryImplicitCast())
                {
                    return MakeError("Can t assign value to float");
                }

                return {};
            }

            if (auto const dstInt = dst.as<IntegerValue*>())
            {
                if (auto const srcInt = src.as<const IntegerValue*>())
                {
                    if (dstInt->isSigned())
                    {
                        dstInt->setInt64(srcInt->getInt64());
                    }
                    else
                    {
                        dstInt->setUint64(srcInt->getUint64());
                    }
                }
                else if (auto const srcFloat = src.as<const FloatValue*>())
                {
                    const auto iValue = static_cast<int64_t>(std::floor(srcFloat->getSingle()));

                    if (dstInt->isSigned())
                    {
                        dstInt->setInt64(iValue);
                    }
                    else
                    {
                        dstInt->setUint64(iValue);
                    }
                }
                else if (!tryImplicitCast())
                {
                    return MakeError("Can t assign value to integer");
                }

                return {};
            }

            return MakeError("Do not known how to assign primitive runtime value");
        }

        Result<> assignCollection(Collection& dst, ReadonlyCollection& src, ValueAssignOptionFlag option)
        {
            if (!option.has(ValueAssignOption::MergeCollection))
            {
                dst.clear();
            }
            if (const size_t size = src.getSize(); size > 0)
            {
                dst.reserve(size);
                for (size_t i = 0; i < size; ++i)
                {
                    CheckResult(dst.append(src.getAt(i)));
                }
            }

            return {};
        }

        Result<> assignDictionary(ReadonlyDictionary& dst, ReadonlyDictionary& src, ValueAssignOptionFlag option)
        {
            if (Dictionary* const mutableDictionary = dst.as<Dictionary*>(); mutableDictionary && !option.has(ValueAssignOption::MergeCollection))
            {
                mutableDictionary->clear();
            }

            for (size_t i = 0, size = src.getSize(); i < size; ++i)
            {
                auto key = src.getKey(i);
                auto value = src.getValue(key);
                CheckResult(dst.setValue(key, value));
            }

            return {};
        }

        Result<> assignObject(RuntimeObject& obj, ReadonlyDictionary& srcDict)
        {
            for (size_t i = 0, size = obj.getSize(); i < size; ++i)
            {
                auto key = obj.getKey(i);
                if (auto srcValue = srcDict.getValue(key))
                {
                    CheckResult(obj.setFieldValue(key, srcValue));
                }
            }

            return {};
        }

    }  // namespace

    Result<> RuntimeValue::assign(RuntimeValuePtr dst, RuntimeValuePtr src, ValueAssignOptionFlag option)
    {
        MY_DEBUG_ASSERT(dst);
        MY_DEBUG_ASSERT(dst->isMutable());

        if (RuntimeValueRef* const valueRef = dst->as<RuntimeValueRef*>())
        {
            valueRef->setValue(std::move(src));
            return {};
        }

        MY_DEBUG_ASSERT(src);

        const auto clearDstIfOptionalSrcIsNull = [&dst]() -> Result<>
        {
            if (auto const dstOpt = dst->as<OptionalValue*>())
            {
                dstOpt->reset();
            }
            else if (auto const dstStr = dst->as<StringValue*>())
            {
                CheckResult(dstStr->setString(""));
            }
            else if (auto const dstCollection = dst->as<Collection*>())
            {
                dstCollection->clear();
            }
            else if (auto const dstDict = dst->as<Dictionary*>())
            {
                dstDict->clear();
            }

            return kResultSuccess;
        };

        if (RuntimeValueRef* const srcRefValue = src->as<RuntimeValueRef*>())
        {
            if (RuntimeValuePtr referencedValue = srcRefValue->getValue(); referencedValue)
            {
                return RuntimeValue::assign(dst, referencedValue);
            }

            return clearDstIfOptionalSrcIsNull();
        }

        if (OptionalValue* const srcOpt = src->as<OptionalValue*>())
        {
            if (srcOpt->hasValue())
            {
                return RuntimeValue::assign(dst, srcOpt->getValue());
            }

            return clearDstIfOptionalSrcIsNull();
        }

        if (auto const dstOpt = dst->as<OptionalValue*>())
        {
            return dstOpt->setValue(src);
        }

        if (auto const dstValue = dst->as<PrimitiveValue*>())
        {
            const PrimitiveValue* const srcValue = src->as<PrimitiveValue*>();
            if (!srcValue)
            {
                return MakeError("Expected primitive value");
            }
            return assignPrimitiveValue(*dstValue, *srcValue);
        }

        if (auto const dstCollection = dst->as<Collection*>())
        {
            ReadonlyCollection* const srcCollection = src->as<ReadonlyCollection*>();
            if (!srcCollection)
            {
                return MakeError("Expected Collection object");
            }

            return assignCollection(*dstCollection, *srcCollection, option);
        }

        if (auto* const dstCollection = dst->as<ReadonlyCollection*>(); dstCollection && src->is<ReadonlyCollection>())
        {
            ReadonlyCollection* const srcCollection = src->as<ReadonlyCollection*>();
            if (!srcCollection)
            {
                return MakeError("Expected Collection object");
            }

            const size_t srcSize = srcCollection->getSize();
            for (size_t i = 0, dstSize = dstCollection->getSize(); i < dstSize; ++i)
            {
                if (srcSize <= i)
                {
                    // check that dst is optional (and throw if not) ?
                    continue;
                }

                CheckResult(dstCollection->setAt(i, srcCollection->getAt(i)));
            }

            return {};
        }

        // must assign object prior dictionary:
        // each object is also dictionary, but logic is differ:  object has fixed set of the fields.
        if (auto* const dstObj = dst->as<RuntimeObject*>())
        {
            ReadonlyDictionary* const srcDict = src->as<ReadonlyDictionary*>();
            return srcDict ? assignObject(*dstObj, *srcDict) : MakeError("Expected Dictionary object");
        }

        if (auto* const dstDict = dst->as<ReadonlyDictionary*>())
        {
            ReadonlyDictionary* const srcDict = src->as<ReadonlyDictionary*>();
            return srcDict ? assignDictionary(*dstDict, *srcDict, option) : MakeError("Expected Dictionary object");
        }

        return MakeError("Do not known how to assign runtime value");
    }

    Ptr<RuntimeValueRef> RuntimeValueRef::create(RuntimeValuePtr& value, IAllocator* allocator)
    {
        return rtti::createInstanceWithAllocator<RuntimeValueRefImpl, RuntimeValueRef>(allocator, std::ref(value));
    }

    // Ptr<RuntimeValueRef> RuntimeValueRef::create(std::reference_wrapper<const RuntimeValuePtr> value, AllocatorPtr allocator)
    // {
    //     return rtti::createInstanceWithAllocator<RuntimeValueRefImpl, RuntimeValueRef>(std::move(allocator), value);
    // }

}  // namespace my
