// #my_engine_source_file

#include "my/script/realm.h"

namespace my::script
{
    InvocationScopeGuard::InvocationScopeGuard(IRealm& realm, InvokeOptsFlag opts) :
        m_handle(realm.OpenInvocationScope(opts))
    {
    }

    InvocationScopeGuard::~InvocationScopeGuard() = default;
}  // namespace my::script