// #my_engine_source_header

#include "my/diag/error.h"

namespace my::test
{
    struct MY_ABSTRACT_TYPE ITestError : Error
    {
        MY_ABSTRACT_ERROR(ITestError, Error)

        virtual unsigned getErrorCode() const = 0;
    };

    class TestError final : public DefaultError<ITestError>
    {
        using ErrorBase = DefaultError<ITestError>;

        MY_ERROR(TestError, ErrorBase)

    public:
        TestError(diag::SourceInfo sourceInfo, unsigned errorCode) :
            ErrorBase(sourceInfo, "errorCode"),
            m_errorCode(errorCode)
        {
        }

        unsigned getErrorCode() const override
        {
            return m_errorCode;
        }

    private:
        const unsigned m_errorCode;
    };

    TEST(TestError, MakeDefaultError)
    {
        const char* ErrorText = "test error";

        auto error = MakeError(ErrorText);
        ASSERT_EQ(std::string{ErrorText}, error->getMessage());
        ASSERT_TRUE(error->is<my::Error>());
    }

    TEST(TestError, MakeCustomError)
    {
        const std::string ErrorText = "test error";

        auto error2 = MakeErrorT(TestError)(100);
        ASSERT_EQ("errorCode", error2->getMessage());
        ASSERT_TRUE(error2->is<my::Error>());
        ASSERT_TRUE(error2->is<TestError>());
    }

    TEST(TestError, ErrorIsStdException)
    {
        const std::string ErrorText = "test error";
        auto error = MakeError(ErrorText);

        ASSERT_TRUE(error->is<std::exception>());

        auto& exception = error->as<const std::exception&>();
        ASSERT_EQ(ErrorText, exception.what());
    }

    TEST(TestError, FormattedMessage)
    {
        auto error = MakeError("Text[{}][{}]", 77, 22);
        ASSERT_EQ(error->getMessage(), std::string_view{"Text[77][22]"});
    }

}  // namespace my::test
