// #my_engine_source_file
#include "my/memory/runtime_stack.h"
#include "my/memory/singleton_memop.h"
#include "my/runtime/internal/runtime_object_registry.h"
#include "my/threading/lock_guard.h"
#include "my/utils/scope_guard.h"

namespace my {
namespace {

template <typename T, typename... U>
void fastRemoveObjectAt(std::vector<T, U...>& container, size_t index)
{
    MY_DEBUG_FATAL(index < container.size());
    const size_t lastIndex = container.size() - 1;
    if (index != lastIndex)
    {
        container[index] = std::move(container[lastIndex]);
    }

    container.resize(lastIndex);
}

}  // namespace

class RuntimeObjectRegistryImpl final : public RuntimeObjectRegistry
{
    MY_DECLARE_SINGLETON_MEMOP(RuntimeObjectRegistryImpl)
public:
    RuntimeObjectRegistryImpl() = default;

    ~RuntimeObjectRegistryImpl();

    [[nodiscard]] ObjectId addObject(Ptr<>);

    [[nodiscard]] ObjectId addObject(IRttiObject&);

    void removeObject(ObjectId);

    bool isAutoRemovable(ObjectId);

private:
    class ObjectEntry
    {
    public:
        ObjectEntry() :
            m_objectId(0),
            m_ptr(nullptr),
            m_isWeak(false)
        {
        }

        ObjectEntry(ObjectId objectId, IRttiObject& object) :
            m_objectId(objectId),
            m_ptr(&object),
            m_isWeak(false)
        {
        }

        ObjectEntry(ObjectId objectId, Ptr<>& object) :
            m_objectId(objectId),
            m_ptr(getWeakRef(object)),
            m_isWeak(true)
        {
        }

        ObjectEntry(const ObjectEntry&) = delete;

        ObjectEntry(ObjectEntry&& other) :
            m_objectId(std::exchange(other.m_objectId, 0)),
            m_ptr(std::exchange(other.m_ptr, nullptr)),
            m_isWeak(std::exchange(other.m_isWeak, false))
        {
        }

        ~ObjectEntry()
        {
            reset();
        }

        ObjectEntry& operator=(const ObjectEntry&) = delete;

        ObjectEntry& operator=(ObjectEntry&& other)
        {
            reset();

            m_objectId = std::exchange(other.m_objectId, 0);
            m_ptr = std::exchange(other.m_ptr, nullptr);
            m_isWeak = std::exchange(other.m_isWeak, false);

            return *this;
        }

        explicit operator bool() const
        {
            return m_ptr != nullptr;
        }

        ObjectId getObjectId() const
        {
            return m_objectId;
        }

        bool isWeakRef() const
        {
            return m_isWeak;
        }

        bool isExpired() const
        {
            return !m_ptr || (m_isWeak && reinterpret_cast<const IWeakRef*>(m_ptr)->isDead());
        }

        /**
            Return actual instance.
            For weak refs first lock is performed (to check that object actually live).
            For non weak refs will return instance as is.
            Always add ref for ref for refcounted object
        */
        IRttiObject* lock() const
        {
            if (!m_ptr)
            {
                return nullptr;
            }

            if (m_isWeak)
            {
                return reinterpret_cast<IWeakRef*>(m_ptr)->acquire();
            }

            IRttiObject* const instance = reinterpret_cast<IRttiObject*>(m_ptr);
            if (IRefCounted* const refCounted = instance->as<IRefCounted*>())
            {  // always add reference if object is ref counted (even it was registered through IRttiObject& constructor)
               // this is makes logic below where objects are used more simple (just always releaseRef())
                refCounted->addRef();
            }

            return instance;
        }

        bool isMyNonWeakRefObject(const IRttiObject* object) const
        {
            return !m_isWeak && m_ptr == object;
        }

    private:
        static void* getWeakRef(Ptr<>& object)
        {
            MY_DEBUG_ASSERT(object);
            return object->getWeakRef();
        }

        void reset()
        {
            if (m_ptr && m_isWeak)
            {
                reinterpret_cast<IWeakRef*>(m_ptr)->releaseRef();
            }

            m_ptr = nullptr;
            m_isWeak = false;
        }

        ObjectId m_objectId;
        void* m_ptr;
        bool m_isWeak;
    };

    void removeExpiredEntries();

    void visitObjects(void (*callback)(std::span<IRttiObject*>, void*), const rtti::TypeInfo, void*) override;

    std::recursive_mutex m_mutex;
    ObjectId m_objectId = 0;
    std::vector<ObjectEntry> m_objects;
};

RuntimeObjectRegistryImpl::~RuntimeObjectRegistryImpl()
{
    removeExpiredEntries();
    MY_DEBUG_ASSERT(m_objects.empty(), "Still alive ({}) objects", m_objects.size());
}

void RuntimeObjectRegistryImpl::visitObjects(VisitObjectsCallback callback, const rtti::TypeInfo type, void* callbackData)
{
    lock_(m_mutex);

    if (m_objects.empty())
    {
        return;
    }

    rtstack_scope;

    StackContainer<std::vector, IRttiObject*> instances;
    instances.reserve(m_objects.size());

    scope_on_leave
    {
        for (IRttiObject* const obj : instances)
        {
            if (IRefCounted* const refCounted = obj->as<IRefCounted*>())
            {
                refCounted->releaseRef();
            }
        }
    };

    // collect required objects, remove expired entries
    for (size_t i = 0; i < m_objects.size();)
    {
        // lock will return nullptr only for expired weak referenced objects
        IRttiObject* const instance = m_objects[i].lock();
        if (!instance)
        {
            fastRemoveObjectAt(m_objects, i);
            continue;
        }

        if (!type || instance->is(type))
        {
            instances.push_back(std::move(instance));
        }
        else if (IRefCounted* const refCounted = instance->as<IRefCounted*>())
        {  // object will not be used and must be released immediately
            refCounted->releaseRef();
        }

        ++i;
    }

    if (!instances.empty())
    {
        callback({instances.begin(), instances.end()}, callbackData);
    }
}

RuntimeObjectRegistry::ObjectId RuntimeObjectRegistryImpl::addObject(Ptr<> ptr)
{
    lock_(m_mutex);

    MY_DEBUG_ASSERT(ptr);

    const auto id = ++m_objectId;
    m_objects.emplace_back(id, ptr);

    return id;
}

RuntimeObjectRegistry::ObjectId RuntimeObjectRegistryImpl::addObject(IRttiObject& object)
{
    lock_(m_mutex);

    const auto id = ++m_objectId;
    m_objects.emplace_back(id, std::ref(object));

    return id;
}

void RuntimeObjectRegistryImpl::removeExpiredEntries()
{
    std::erase_if(m_objects, [](const ObjectEntry& entry)
    {
        return entry.isExpired();
    });
}

void RuntimeObjectRegistryImpl::removeObject(ObjectId id)
{
    lock_(m_mutex);

    for (size_t i = 0, size = m_objects.size(); i < size; ++i)
    {
        if (m_objects[i].getObjectId() == id)
        {
            fastRemoveObjectAt(m_objects, i);
            break;
        }
    }
}

bool RuntimeObjectRegistryImpl::isAutoRemovable(ObjectId objectId)
{
    lock_(m_mutex);

    auto iter = std::find_if(m_objects.begin(), m_objects.end(), [objectId](const ObjectEntry& entry)
    {
        return entry.getObjectId() == objectId;
    });

    return iter != m_objects.end() && iter->isWeakRef();
}

namespace {
auto& getCurrentRuntimeObjectRegistryRef()
{
    static std::unique_ptr<RuntimeObjectRegistryImpl> s_runtimeObjectRegistry;
    return (s_runtimeObjectRegistry);
}
}  // namespace

RuntimeObjectRegistration::RuntimeObjectRegistration() :
    m_objectId(0)
{
}

RuntimeObjectRegistration::RuntimeObjectRegistration(Ptr<> object) :
    RuntimeObjectRegistration()
{
    if (auto& registry = getCurrentRuntimeObjectRegistryRef())
    {
        m_objectId = registry->addObject(std::move(object));
    }
}

RuntimeObjectRegistration::RuntimeObjectRegistration(IRttiObject& object) :
    RuntimeObjectRegistration()
{
    if (auto& registry = getCurrentRuntimeObjectRegistryRef())
    {
        m_objectId = registry->addObject(object);
    }
}

RuntimeObjectRegistration::RuntimeObjectRegistration(RuntimeObjectRegistration&& other) :
    m_objectId(std::exchange(other.m_objectId, 0))
{
}

RuntimeObjectRegistration::~RuntimeObjectRegistration()
{
    reset();
}

RuntimeObjectRegistration& RuntimeObjectRegistration::operator=(RuntimeObjectRegistration&& other)
{
    reset();
    m_objectId = std::exchange(other.m_objectId, 0);
    return *this;
}

RuntimeObjectRegistration& RuntimeObjectRegistration::operator=(std::nullptr_t)
{
    reset();
    return *this;
}

RuntimeObjectRegistration::operator bool() const
{
    return m_objectId != 0;
}

void RuntimeObjectRegistration::setAutoRemove()
{
    if (m_objectId == 0)
    {
        return;
    }

    auto& registry = getCurrentRuntimeObjectRegistryRef();
    if (!registry)
    {
        return;
    }

    const bool isAutoRemovable = registry->isAutoRemovable(m_objectId);
    MY_DEBUG_FATAL(isAutoRemovable, "Object can not be used as autoremovable");

    if (isAutoRemovable)
    {
        m_objectId = 0;
    }
}

void RuntimeObjectRegistration::reset()
{
    if (m_objectId != 0)
    {
        const auto objectId = std::exchange(m_objectId, 0);

        auto& registry = getCurrentRuntimeObjectRegistryRef();
        MY_DEBUG_ASSERT(registry);
        if (registry)
        {
            registry->removeObject(objectId);
        }
    }
}

RuntimeObjectRegistry& getRuntimeObjectRegistry()
{
    auto& registry = getCurrentRuntimeObjectRegistryRef();
    MY_DEBUG_FATAL(registry);

    return *registry;
}

bool hasRuntimeObjectRegistry()
{
    return static_cast<bool>(getCurrentRuntimeObjectRegistryRef());
}

void setDefaultRuntimeObjectRegistryInstance()
{
    auto& registry = getCurrentRuntimeObjectRegistryRef();
    MY_DEBUG_ASSERT(!registry, "Default instance already set");
    registry = std::make_unique<RuntimeObjectRegistryImpl>();
}

void resetRuntimeObjectRegistryInstance()
{
    getCurrentRuntimeObjectRegistryRef().reset();
}

}  // namespace my
