// #my_engine_source_header
#pragma once
#include "my/diag/error.h"
#include "my/kernel/kernel_config.h"


namespace my::diag
{
    /**
     */
    MY_KERNEL_EXPORT unsigned getAndResetLastErrorCode();

    /**
     */
    MY_KERNEL_EXPORT std::wstring getWinErrorMessageW(unsigned errorCode);

    /**
     */
    MY_KERNEL_EXPORT std::string getWinErrorMessageA(unsigned errorCode);

    /**
     */
    class MY_KERNEL_EXPORT WinCodeError : public my::DefaultError<>
    {
        MY_ERROR(my::diag::WinCodeError, my::DefaultError<>)
    
    public:
        WinCodeError(const diag::SourceInfo& sourceInfo, unsigned errorCode = diag::getAndResetLastErrorCode());

        WinCodeError(const diag::SourceInfo& sourceInfo, std::string message, unsigned errorCode = diag::getAndResetLastErrorCode());

        unsigned getErrorCode() const;

    private:
        const unsigned m_errorCode;
    };

}  // namespace my::diag

