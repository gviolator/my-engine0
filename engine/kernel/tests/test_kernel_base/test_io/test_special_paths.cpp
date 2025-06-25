// #my_engine_source_file
#ifdef _WIN32
#include "my/io/special_paths.h"

namespace fs = std::filesystem;

namespace my::test
{
    TEST(TestSpecialPaths, NotEmptyWithPrefixFileName)
    {
        const fs::path tempFileName = io::getNativeTempFilePath("TEST");
        ASSERT_FALSE(tempFileName.empty());
    }

    TEST(TestSpecialPaths, CorrectPrefixFileName)
    {
        const fs::path tempFileName = io::getNativeTempFilePath("MyTemp");
        //ASSERT_FALSE(tempFileName.find(u8"TMP") == std::string::npos);
        ASSERT_FALSE(tempFileName.empty());
    }

    TEST(TestSpecialPaths, KnownFolder_UserDocuments)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::UserDocuments);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::UserDocuments);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Home)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::UserHome);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::UserHome);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_LocalAppData)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::LocalAppData);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::LocalAppData);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Temp)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::Temp);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::Temp);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_ExecutableLocation)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::ExecutableLocation);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::ExecutableLocation);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

    TEST(TestSpecialPaths, KnownFolder_Current)
    {
        auto path0 = io::getKnownFolderPath(io::KnownFolder::Current);
        auto path1 = io::getKnownFolderPath(io::KnownFolder::Current);
        ASSERT_TRUE(!path0.empty());
        ASSERT_EQ(path0, path1);
        ASSERT_TRUE(std::filesystem::is_directory(path0));
    }

}  // namespace my::test
#endif
