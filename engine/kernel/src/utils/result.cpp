// #my_engine_source_header
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

    Error::Ptr Result<void>::getError() const
    {
        MY_DEBUG_CHECK(isError(), "Result<void> has no error");

        return m_error;
    }

    bool Result<>::isSuccess(Error::Ptr* error) const
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
        MY_DEBUG_CHECK(!m_error, "Ignoring Result<> that holds an error:{}", m_error->getMessage());
    }

} // namespace nau
