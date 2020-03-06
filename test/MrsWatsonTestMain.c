//
// MrsWatsonTestMain.c - MrsWatson
// Copyright (c) 2016 Teragon Audio. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include "MrsWatsonTestMain.h"

#include "app/ProgramOption.h"
#include "base/File.h"
#include "base/PlatformInfo.h"
#include "unit/ApplicationRunner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern LinkedList getTestSuites(File mrsWatsonExePath, File resourcesPath);
extern TestSuite findTestSuite(LinkedList testSuites,
                               const CharString testSuiteName);
extern TestCase findTestCase(TestSuite testSuite, char *testName);
extern void printUnitTestSuites(void);
extern TestSuite runUnitTests(LinkedList testSuites, boolByte onlyPrintFailing);

#if UNIX
static const char *MRSWATSON_EXE_NAME = "mrswatson";
#elif WINDOWS
static const char *MRSWATSON_EXE_NAME = "mrswatson.exe";
#else
static const char *MRSWATSON_EXE_NAME = "mrswatson";
#endif

static boolByte _assertResourcePath(const File path,
                                    const ProgramOptions programOptions) {
  boolByte testRequired = false;

  if (programOptions->options[OPTION_TEST_NAME]->enabled) {
    if (strstr(programOptionsGetString(programOptions, OPTION_TEST_NAME)->data,
               "Integration:")) {
      testRequired = true;
    }
  } else if (programOptions->options[OPTION_TEST_SUITE]->enabled) {
    testRequired = charStringIsEqualToCString(
        programOptionsGetString(programOptions, OPTION_TEST_SUITE),
        "Integration", true);
  } else {
    testRequired = true;
  }

  if (testRequired && !fileExists(path)) {
    printf("ERROR: required resource path was not found\n");
    return false;
  }

  return true;
}

static ProgramOptions _newTestProgramOptions(void) {
  ProgramOptions programOptions = newProgramOptions(NUM_TEST_OPTIONS);
  srand((unsigned int)time(NULL));

  programOptionsAdd(programOptions, newProgramOptionWithName(
                                        OPTION_TEST_SUITE, "suite",
                                        "Choose a test suite to run. Run with "
                                        "'--list' option to see all suites.",
                                        true, kProgramOptionTypeString,
                                        kProgramOptionArgumentTypeRequired));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_NAME, "test",
          "Run a single test. Tests are named 'Suite:Name', for example:\n\
\t-t 'LinkedList:AppendItem'",
          true, kProgramOptionTypeString, kProgramOptionArgumentTypeRequired));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_PRINT_TESTS, "list-tests",
          "List all unit tests in the same format required by --test", true,
          kProgramOptionTypeEmpty, kProgramOptionArgumentTypeNone));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_MRSWATSON_PATH, "mrswatson-path",
          "Path to mrswatson executable. By default, mrswatson is assumed to be in the same \
directory as mrswatsontest. Only required for running integration tests.",
          true, kProgramOptionTypeString, kProgramOptionArgumentTypeRequired));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(OPTION_TEST_RESOURCES_PATH, "resources",
                               "Path to resources directory. Only required for "
                               "running integration tests.",
                               true, kProgramOptionTypeString,
                               kProgramOptionArgumentTypeRequired));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_PRINT_ONLY_FAILING, "quiet",
          "Print only failing tests. Note that if a test causes the suite to crash, the \
bad test's name will not be printed. In this case, re-run without this option, as \
the test names will be printed before the tests are executed.",
          true, kProgramOptionTypeEmpty, kProgramOptionArgumentTypeNone));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_COLOR, "color",
          "Use colored output (valid options: 'auto', 'force', 'none'", true,
          kProgramOptionTypeString, kProgramOptionArgumentTypeRequired));
  programOptionsSetCString(programOptions, OPTION_TEST_COLOR, "auto");

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_KEEP_FILES, "keep-files",
          "Keep files generated by integration tests (such as log files, audio output, \
etc.). Normally these files are automatically removed if a test succeeds.",
          true, kProgramOptionTypeEmpty, kProgramOptionArgumentTypeNone));

  programOptionsAdd(
      programOptions,
      newProgramOptionWithName(
          OPTION_TEST_HELP, "help", "Print full program help (this screen), or "
                                    "just the help for a single argument.",
          true, kProgramOptionTypeString, kProgramOptionArgumentTypeOptional));

  programOptionsAdd(programOptions,
                    newProgramOptionWithName(OPTION_TEST_VERBOSE, "verbose",
                                             "Show logging output from tests",
                                             true, kProgramOptionTypeEmpty,
                                             kProgramOptionArgumentTypeNone));

  return programOptions;
}

void _printTestSummary(int testsRun, int testsPassed, int testsFailed,
                       int testsSkipped) {
  CharString numberBuffer = newCharStringWithCapacity(kCharStringLengthShort);

  printToLog(getLogColor(kTestLogEventReset), NULL, "Ran ");
  sprintf(numberBuffer->data, "%d", testsRun);
  printToLog(getLogColor(kTestLogEventSection), NULL, numberBuffer->data);
  printToLog(getLogColor(kTestLogEventReset), NULL, " tests: ");
  sprintf(numberBuffer->data, "%d", testsPassed);
  printToLog(getLogColor(kTestLogEventPass), NULL, numberBuffer->data);
  printToLog(getLogColor(kTestLogEventReset), NULL, " passed, ");

  sprintf(numberBuffer->data, "%d", testsFailed);

  if (testsFailed > 0) {
    printToLog(getLogColor(kTestLogEventFail), NULL, numberBuffer->data);
  } else {
    printToLog(getLogColor(kTestLogEventReset), NULL, numberBuffer->data);
  }

  printToLog(getLogColor(kTestLogEventReset), NULL, " failed, ");

  sprintf(numberBuffer->data, "%d", testsSkipped);

  if (testsSkipped > 0) {
    printToLog(getLogColor(kTestLogEventSkip), NULL, numberBuffer->data);
  } else {
    printToLog(getLogColor(kTestLogEventReset), NULL, numberBuffer->data);
  }

  printToLog(getLogColor(kTestLogEventReset), NULL, " skipped");
  flushLog(NULL);

  freeCharString(numberBuffer);
}

File _findMrsWatsonExe(CharString mrsWatsonExeArg) {
  CharString currentExecutableFilename = NULL;
  CharString mrsWatsonExeName = NULL;
  File currentExecutablePath = NULL;
  File currentExecutableDir = NULL;
  File mrsWatsonExe = NULL;

  if (mrsWatsonExeArg != NULL && !charStringIsEmpty(mrsWatsonExeArg)) {
    mrsWatsonExe = newFileWithPath(mrsWatsonExeArg);
  } else {
    currentExecutableFilename = fileGetExecutablePath();
    currentExecutablePath = newFileWithPath(currentExecutableFilename);

    if (currentExecutablePath != NULL) {
      currentExecutableDir = fileGetParent(currentExecutablePath);

      if (currentExecutableDir != NULL) {
        mrsWatsonExeName = newCharStringWithCString(MRSWATSON_EXE_NAME);

        if (platformInfoIsRuntime64Bit()) {
          charStringAppendCString(mrsWatsonExeName, "64");
        }

        mrsWatsonExe =
            newFileWithParent(currentExecutableDir, mrsWatsonExeName);
      }
    }
  }

  freeCharString(currentExecutableFilename);
  freeCharString(mrsWatsonExeName);
  freeFile(currentExecutablePath);
  freeFile(currentExecutableDir);
  return mrsWatsonExe;
}

int main(int argc, char *argv[]) {
  ProgramOptions programOptions;
  int totalTestsRun = 0;
  int totalTestsPassed = 0;
  int totalTestsFailed = 0;
  int totalTestsSkipped = 0;
  CharString testSuiteToRun = NULL;
  CharString testSuiteName = NULL;
  CharString totalTimeString = NULL;
  CharString executablePath = NULL;
  CharString useColor = NULL;
  TestCase testCase = NULL;
  TestSuite testSuite = NULL;
  LinkedList testSuites = NULL;
  TestSuite unitTestResults = NULL;
  TaskTimer timer;
  char *testArgument;
  char *colon;
  char *testCaseName;

  timer = newTaskTimer(NULL, NULL);
  taskTimerStart(timer);

  programOptions = _newTestProgramOptions();

  if (!programOptionsParseArgs(programOptions, argc, argv)) {
    printf("Or run with --help (option) to see help for a single option\n");
    freeProgramOptions(programOptions);
    freeTaskTimer(timer);
    return -1;
  }

  if (programOptions->options[OPTION_TEST_HELP]->enabled) {
    printf("Run with '--help full' to see extended help for all options.\n");

    if (charStringIsEmpty(
            programOptionsGetString(programOptions, OPTION_TEST_HELP))) {
      printf("All options, where <argument> is required and [argument] is "
             "optional\n");
      programOptionsPrintHelp(programOptions, false, DEFAULT_INDENT_SIZE);
    } else {
      programOptionsPrintHelp(programOptions, true, DEFAULT_INDENT_SIZE);
    }

    freeProgramOptions(programOptions);
    freeTaskTimer(timer);
    return -1;
  } else if (programOptions->options[OPTION_TEST_PRINT_TESTS]->enabled) {
    printUnitTestSuites();
    freeProgramOptions(programOptions);
    freeTaskTimer(timer);
    return -1;
  }

  useColor = programOptionsGetString(programOptions, OPTION_TEST_COLOR);
  useColoredOutput(useColor);
  if (programOptions->options[OPTION_TEST_VERBOSE]->enabled) {
    initEventLogger();
    setLogLevel(LOG_DEBUG);
    setLoggingColorEnabledWithString(useColor);
  }

  testSuiteToRun = programOptionsGetString(programOptions, OPTION_TEST_SUITE);
  File mrsWatsonExePath = _findMrsWatsonExe(
      programOptionsGetString(programOptions, OPTION_TEST_MRSWATSON_PATH));
  File resourcesPath = newFileWithPath(
      programOptionsGetString(programOptions, OPTION_TEST_RESOURCES_PATH));
  if (!_assertResourcePath(mrsWatsonExePath, programOptions)) {
    freeProgramOptions(programOptions);
    freeFile(mrsWatsonExePath);
    freeFile(resourcesPath);
    freeTaskTimer(timer);
    return -1;
  }
  if (!_assertResourcePath(resourcesPath, programOptions)) {
    freeProgramOptions(programOptions);
    freeFile(mrsWatsonExePath);
    freeFile(resourcesPath);
    freeTaskTimer(timer);
    return -1;
  }

  if (programOptions->options[OPTION_TEST_NAME]->enabled) {
    testArgument =
        programOptionsGetString(programOptions, OPTION_TEST_NAME)->data;
    colon = strchr(testArgument, ':');

    if (colon == NULL) {
      printf("ERROR: Invalid test name\n");
      programOptionPrintHelp(programOptions->options[OPTION_TEST_NAME], true,
                             DEFAULT_INDENT_SIZE, 0);
      freeProgramOptions(programOptions);
      freeFile(mrsWatsonExePath);
      freeFile(resourcesPath);
      freeTaskTimer(timer);
      return -1;
    }

    testCaseName = strdup(colon + 1);
    *colon = '\0';

    testSuiteName = programOptionsGetString(programOptions, OPTION_TEST_NAME);
    testSuites = getTestSuites(mrsWatsonExePath, resourcesPath);
    testSuite = findTestSuite(testSuites, testSuiteName);

    if (testSuite == NULL) {
      printf("ERROR: Could not find test suite '%s'\n", testSuiteName->data);
      freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
      freeProgramOptions(programOptions);
      freeFile(mrsWatsonExePath);
      freeFile(resourcesPath);
      freeTaskTimer(timer);
      return -1;
    }

    testCase = findTestCase(testSuite, testCaseName);

    if (testCase == NULL) {
      printf("ERROR: Could not find test case '%s'\n", testCaseName);
      freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
      freeProgramOptions(programOptions);
      freeFile(mrsWatsonExePath);
      freeFile(resourcesPath);
      freeTaskTimer(timer);
      return -1;
    } else {
      printf("=== Running test %s:%s ===\n", testSuite->name, testCase->name);
      runTestCase(testCase, testSuite);
      freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
    }
  } else if (programOptions->options[OPTION_TEST_SUITE]->enabled) {
    testSuites = getTestSuites(mrsWatsonExePath, resourcesPath);
    testSuite = findTestSuite(testSuites, testSuiteToRun);

    if (testSuite == NULL) {
      printf("ERROR: Invalid test suite '%s'\n", testSuiteToRun->data);
      printf("Run with '--list' suite to show possible test suites\n");
      freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
      freeProgramOptions(programOptions);
      freeFile(mrsWatsonExePath);
      freeFile(resourcesPath);
      freeTaskTimer(timer);
      return -1;
    } else {
      printf("=== Running test suite %s ===\n", testSuite->name);
      testSuite->onlyPrintFailing =
          programOptions->options[OPTION_TEST_PRINT_ONLY_FAILING]->enabled;
      runTestSuite(testSuite, NULL);
      totalTestsRun = testSuite->numSuccess + testSuite->numFail;
      totalTestsPassed = testSuite->numSuccess;
      totalTestsFailed = testSuite->numFail;
      totalTestsSkipped = testSuite->numSkips;
      freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
    }
  } else {
    testSuites = getTestSuites(mrsWatsonExePath, resourcesPath);

    printf("=== Running tests ===\n");
    unitTestResults = runUnitTests(
        testSuites,
        programOptions->options[OPTION_TEST_PRINT_ONLY_FAILING]->enabled);

    totalTestsRun += unitTestResults->numSuccess + unitTestResults->numFail;
    totalTestsPassed += unitTestResults->numSuccess;
    totalTestsFailed += unitTestResults->numFail;
    totalTestsSkipped += unitTestResults->numSkips;

    freeLinkedListAndItems(testSuites, (LinkedListFreeItemFunc)freeTestSuite);
    freeTestSuite(unitTestResults);
  }

  taskTimerStop(timer);

  if (totalTestsRun > 0) {
    printf("\n=== Finished ===\n");
    _printTestSummary(totalTestsRun, totalTestsPassed, totalTestsFailed,
                      totalTestsSkipped);
    totalTimeString = taskTimerHumanReadbleString(timer);
    printf("Total time: %s\n", totalTimeString->data);
  }

  freeProgramOptions(programOptions);
  freeCharString(executablePath);
  freeCharString(totalTimeString);
  freeFile(mrsWatsonExePath);
  freeFile(resourcesPath);
  freeTaskTimer(timer);
  return totalTestsFailed;
}
