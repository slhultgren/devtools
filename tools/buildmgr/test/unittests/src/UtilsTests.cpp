/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CBuildUnitTestEnv.h"

#include "XMLTreeSlim.h"
#include "RteValueAdjuster.h"

using namespace std;

class CbuildUtilsTests :public ::testing::Test {
protected:
  static void SetUpTestSuite();
  static void TearDownTestSuite();
};

void CbuildUtilsTests::SetUpTestSuite() {
}

void CbuildUtilsTests::TearDownTestSuite() {
  error_code ec;
  fs::current_path(CBuildUnitTestEnv::workingDir, ec);
}

TEST_F(CbuildUtilsTests, GetFileType)
{
  RteFile::Category cat;
  vector<pair<string, RteFile::Category>> input =
  { {"Test.c", RteFile::Category::SOURCE_C }, {"Test.C",RteFile::Category::SOURCE_C},
  {"Test.cpp", RteFile::Category::SOURCE_CPP}, {"Test.c++", RteFile::Category::SOURCE_CPP},
  {"Test.C++", RteFile::Category::SOURCE_CPP}, {"Test.cxx",RteFile::Category::SOURCE_CPP},
  {"Test.asm", RteFile::Category::SOURCE_ASM}, {"Test.s", RteFile::Category::SOURCE_ASM },
  {"Test.S",RteFile::Category::SOURCE_ASM }, {"Test.txt",RteFile::Category::OTHER} };

  for (auto elem = 0; elem <= RteFile::Category::OTHER &&
    elem != RteFile::Category::SOURCE; ++elem) {
     cat = CbuildUtils::GetFileType(static_cast<RteFile::Category>(elem), "");
     EXPECT_EQ(cat, static_cast<RteFile::Category>(elem));
  }

  for_each(input.begin(), input.end(), [&](pair<string, RteFile::Category> in) {
    cat = CbuildUtils::GetFileType(RteFile::Category::SOURCE, in.first);
    EXPECT_EQ(cat, in.second);
  });
}

TEST_F(CbuildUtilsTests, RemoveSlash) {
  string input = "/Arm";
  input = CbuildUtils::RemoveSlash(input);
  EXPECT_EQ("Arm", input);

  input = "\testinput";
  input = CbuildUtils::RemoveSlash(input);
  EXPECT_EQ("\testinput", input);
}

TEST_F(CbuildUtilsTests, ReplaceColon) {
  string input = "::Arm:";
  input = CbuildUtils::ReplaceColon(input);
  EXPECT_EQ("__Arm_", input);

  input = "test:input";
  input = CbuildUtils::ReplaceColon(input);
  EXPECT_EQ("test_input", input);
}

TEST_F(CbuildUtilsTests, GetItemByTagAndAttribute) {
  bool result;
  list<RteItem*> list;
  string compiler = "AC6", use = "armasm", name = "Test";
  RteItem compilerItem(nullptr);
  compilerItem.SetTag("cflags");
  compilerItem.SetAttribute("compiler", "AC6");

  RteItem asflagItem(nullptr);
  asflagItem.SetTag("asflags");
  asflagItem.SetAttribute("use", "armasm");

  RteItem outputItem(nullptr);
  outputItem.SetTag("output");
  outputItem.SetAttribute("name", "Test");

  list.push_back(&compilerItem);
  list.push_back(&asflagItem);
  list.push_back(&outputItem);

  /* Valid input */
  const RteItem*    cflags       = CbuildUtils::GetItemByTagAndAttribute(list, "cflags", "compiler", "AC6");
  const RteItem*    asUseflags   = CbuildUtils::GetItemByTagAndAttribute(list, "asflags", "use", "armasm");
  const RteItem*    asNameflags  = CbuildUtils::GetItemByTagAndAttribute(list, "output", "name", "Test");

  /* Invalid input*/
  const RteItem*    unknownflags = CbuildUtils::GetItemByTagAndAttribute(list, "Invalid", "compiler", "AC6");
  const RteItem*    InvalidTag   = CbuildUtils::GetItemByTagAndAttribute(list, "Invalid", "name", "Blinky");

  result = (cflags && asUseflags && asNameflags && !unknownflags && !InvalidTag);
  EXPECT_TRUE(result);
}

TEST_F(CbuildUtilsTests, UpdatePathWithSpaces) {
  string path, expected;

  path     = "/mnt/C/test dir/new folder/";
  expected = "/mnt/C/test\\ dir/new\\ folder/";
  path     = CbuildUtils::UpdatePathWithSpaces(path);
  EXPECT_EQ(expected, path);

  path     = "/C/test dir/double  space/";
  expected = "/C/test\\ dir/double\\ \\ space/";
  path     = CbuildUtils::UpdatePathWithSpaces(path);
  EXPECT_EQ(expected, path);
}

TEST_F(CbuildUtilsTests, StrPathConv) {
  string path, expected;

  path     = "/C/testdir\\new folder";
  expected = "/C/testdir/new folder";
  path     = CbuildUtils::StrPathConv(path);
  EXPECT_EQ(expected, path);

  path     = "/C/test\\dir\\Temp";
  expected = "/C/test/dir/Temp";
  path     = CbuildUtils::StrPathConv(path);
  EXPECT_EQ(expected, path);
}

TEST_F(CbuildUtilsTests, StrPathAbsolute) {
  string path, base, expected;
  error_code ec;
  string cwd = fs::current_path(ec).generic_string() + "/";
  fs::create_directories(cwd + "/UtilsTest/relative/path", ec);

  path     = "./UtilsTest/relative/path";
  base     = cwd;
  expected = "\"" + cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  path     = ".\\UtilsTest\\relative\\path";
  base     = cwd;
  expected = "\"" +cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  path     = "--relpath_flag=./UtilsTest/relative/path";
  base     = cwd;
  expected = "--relpath_flag=\"" + cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  path     = "--relpath_flag=.\\UtilsTest\\relative\\path";
  base     = cwd;
  expected = "--relpath_flag=\"" + cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  // Move CWD
  fs::current_path(cwd + "/UtilsTest", ec);

  path     = "../UtilsTest/relative/path";
  base     = fs::current_path(ec).generic_string() + "/";
  expected = "\"" + cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  path     = "..\\UtilsTest\\relative\\path";
  base     = fs::current_path(ec).generic_string() + "/";
  expected = "\"" + cwd + "UtilsTest/relative/path\"";
  path     = CbuildUtils::StrPathAbsolute(path, base);
  EXPECT_EQ(expected, path);

  // Restore CWD
  fs::current_path(cwd, ec);
}

TEST_F(CbuildUtilsTests, ExecCommand) {
  auto result = CbuildUtils::ExecCommand("invalid command");
  EXPECT_EQ(false, (0 == result.second) ? true : false) << result.first;

  string testdir = "mkdir_test_dir";
  fs::current_path(testout_folder);
  if (fs::exists(testdir)) RemoveDir(testdir);

  result = CbuildUtils::ExecCommand("mkdir " + testdir);
  EXPECT_TRUE(fs::exists(testdir));
  EXPECT_EQ(true, (0 == result.second) ? true : false) << result.first;

  fs::current_path(CBuildUnitTestEnv::workingDir);
  RemoveDir(testdir);
}
