// Unit tests, currently focused on checkedc_utils, but other tests could be
// added here if it's useful.

#include "checkedc_utils.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// This is infeasible with all the varargs functions. Hopefully we're still
// testing the _bodies_ of checkedc_utils. TODO: Make the Checked C compiler
// consider _calls_ to vararg functions safe provided that they have a `format`
// attribute covering all the varargs and -Wformat validation succeeded. (Of
// course, the _implementations_ will still be considered unsafe.)
//#pragma CHECKED_SCOPE on

_Bool expect_sigill = 0;

void expected_sigill_handler(int sig) {
  // It looks like there aren't currently any cases in which there is stdio
  // output that we'd need to flush before doing this.
  const char msg[] = "Received expected SIGILL.\n";
  // These are async-signal-safe.
  write(1, msg, sizeof(msg) - 1);
  _exit(0);
}

void expect_sigill_setup() {
  expect_sigill = 1;
  struct sigaction sa;
  sa.sa_handler = &expected_sigill_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGILL, &sa, 0) < 0) {
    perror("sigaction for expected SIGILL");
    exit(1);
  }
}

// unit_tests_run_all.sh scans for calls to this macro to make the list of tests
// to run.
#define REQUESTED_TEST_IS(tn) (strcmp(test_name, #tn) == 0)

#define VERIFY_RESULT(exret, exbuf)                                            \
  do {                                                                         \
    if (ret == exret && strcmp(buf, exbuf) == 0) {                             \
      printf("Pass:     ret %d, buf %s\n", ret, buf);                          \
    } else {                                                                   \
      printf("EXPECTED: ret %d, buf %s\n", exret, exbuf);                      \
      printf("ACTUAL:   ret %d, buf %s\n", ret, buf);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main(int argc, _Nt_array_ptr<_Nt_array_ptr<char>> argv : count(argc)) {
  // Because many of these tests involve triggering a runtime failure, we run
  // only one specified test.

  if (argc != 2) {
    // Usage errors to stderr, test results (including failures) to stdout.
    fprintf(stderr, "Usage: ./unit_tests TEST_NAME\n");
    return 1;
  }

  _Dynamic_check(argc >= 2);
  _Nt_array_ptr<char> test_name = argv[1];
  char buf _Nt_checked[20] = "Part1";
  int ret;

  if (REQUESTED_TEST_IS(xstrbcpy_short)) {
    ret = xstrbcpy(buf, "Short message", sizeof buf - 1);
    VERIFY_RESULT(13, "Short message");

  } else if (REQUESTED_TEST_IS(xstrbcpy_full)) {
    ret = xstrbcpy(buf, "Rather long message", sizeof buf - 1);
    VERIFY_RESULT(19, "Rather long message");

  } else if (REQUESTED_TEST_IS(xstrbcpy_one_over)) {
    expect_sigill_setup();
    int ret = xstrbcpy(buf, "Overly wordy message", sizeof buf - 1);

  } else if (REQUESTED_TEST_IS(xstrbcat_short)) {
    ret = xstrbcat(buf, " Part2", sizeof buf - 1);
    VERIFY_RESULT(11, "Part1 Part2");

  } else if (REQUESTED_TEST_IS(xstrbcat_full)) {
    ret = xstrbcat(buf, " Barely enough", sizeof buf - 1);
    VERIFY_RESULT(19, "Part1 Barely enough");

  } else if (REQUESTED_TEST_IS(xstrbcat_one_over)) {
    expect_sigill_setup();
    ret = xstrbcat(buf, " A bit too much", sizeof buf - 1);

  } else if (REQUESTED_TEST_IS(xstrbcat_dest_full)) {
    ret = xstrbcat(buf, "", 5);
    VERIFY_RESULT(5, "Part1");

  } else if (REQUESTED_TEST_IS(xstrbcat_dest_already_overfull)) {
    expect_sigill_setup();
    ret = xstrbcat(buf, "", 4);

  } else if (REQUESTED_TEST_IS(xsbprintf_full)) {
    ret = xsbprintf(buf, sizeof buf - 1, "Some message %d", 314159);
    VERIFY_RESULT(19, "Some message 314159");

  } else if (REQUESTED_TEST_IS(xsbprintf_one_over)) {
    expect_sigill_setup();
    ret = xsbprintf(buf, sizeof buf - 1, "Some message %d", 3141592);

  } else {
    fprintf(stderr, "Unrecognized test name '%s'\n", test_name);
    return 1;
  }

  if (expect_sigill) {
    printf("Did not receive expected SIGILL\n");
    return 1;
  }
  return 0;
}
