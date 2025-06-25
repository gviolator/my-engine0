// #my_engine_source_file
#ifdef _WIN32
#include "my/platform/windows/diag/win_error.h"

namespace my::test
{
    /**
        Test that getWinErrorMessage returns something (without crash).
     */
    TEST(TestWinError, GetErrorMessage)
    {
        const unsigned AnyErrorCode = 1008;

        const auto message = diag::getWinErrorMessageA(AnyErrorCode);

        ASSERT_FALSE(message.empty());
    }
}  // namespace my::test
#endif
