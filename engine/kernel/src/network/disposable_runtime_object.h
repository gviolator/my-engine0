// #my_engine_source_file
#pragma once
#include "my/rtti/rtti_object.h"

#include <atomic>

namespace my {

class DisposableRuntimeObject
{
protected:
    bool isDisposed() const;
    void doDispose(bool waitFor, IRefCounted& refCountedSelf, void (*callback)(IRefCounted&) noexcept);

private:
    std::atomic<bool> m_isDisposed = false;
};

}  // namespace my
