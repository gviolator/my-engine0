// #my_engine_source_file

#include "my/script/realm.h"

namespace my::script
{
    InvocationScopeGuard::InvocationScopeGuard(Realm& realm, InvokeOptsFlag opts) :
        m_handle(realm.openInvocationScope(opts))
    {
    }

    InvocationScopeGuard::~InvocationScopeGuard() = default;
}  // namespace my::script