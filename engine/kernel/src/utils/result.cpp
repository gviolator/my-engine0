// #my_engine_source_file
#include "my/utils/result.h"


namespace my
{
    Result<void>::Result(Result<>&& other)
        : m_error(std::move(other.m_error))
    {}

    Result<void>::operator bool () const
    {
        return !isError();
    }

    bool Result<void>::isError() const
    {
        return static_cast<bool>(m_error);
    }

    ErrorPtr Result<void>::getError() const
    {
        MY_DEBUG_ASSERT(isError(), "Result<void> has no error");

        return m_error;
    }

    bool Result<>::isSuccess(ErrorPtr* error) const
    {
        if (m_error && error)
        {
            *error = m_error;
            return false;
        }

        return true;
    }

    void Result<void>::ignore() const noexcept
    {
        MY_DEBUG_ASSERT(!m_error, "Ignoring Result<> that holds an error:{}", m_error->getMessage());
    }

} // namespace nau
