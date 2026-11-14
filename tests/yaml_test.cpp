// yaml_test.cpp - tem::yaml::Reader: typed values, community entries and the errors that stop a run.
// Added 2024-11-09.

#include <sys/wait.h>
#include <unistd.h>

#include <cmath>
#include <iostream>
#include <string>

#include "tem_yaml.h"

static int failures = 0;

static void check(bool ok, const char* what) {
  if (!ok) {
    std::cout << "FAIL: " << what << std::endl;
    ++failures;
  }
}

// True if reading the key stops the run (the reader exits with status 255), as for a bad value.
static bool stops(const std::string& yaml, void (*read)(tem::yaml::Reader&)) {
  std::cout.flush();
  std::cerr.flush();
  const pid_t pid = fork();
  if (pid == 0) {
    if (!freopen("/dev/null", "w", stderr)) _exit(1);
    tem::yaml::Reader r;
    r.parse(yaml, "test");
    read(r);
    _exit(0);
  }
  int status = 0;
  waitpid(pid, &status, 0);
  return WIFEXITED(status) && WEXITSTATUS(status) == 255;
}

int main() {
  tem::yaml::Reader r;
  r.parse(
      "run:\n  years: 12\n  ratio: 0.25\n  whole: 2.0\n  big: 3e2\n  on: true\n  off: no\n  name: tem.nc4\n"
      "files: [a.nc, b.nc4]\n"
      "community:\n  - type: 3\n    description: Boreal\n    leafcb: 4.5\n    phen: 1\n"
      "  - type: 7\n    leafcb: 6.5\n",
      "test");

  check(r.getInt("years") == 12, "int");
  check(std::fabs(r.getDouble("ratio") - 0.25) < 1e-15, "double");
  check(r.getInt("whole") == 2, "an integer written as 2.0");
  check(r.getLong("big") == 300, "an integer written as 3e2");
  check(r.getBool("on") && !r.getBool("off"), "booleans");
  check(r.getString("name") == "tem.nc4", "string");
  check(r.getStringList("files").size() == 2 && r.getStringList("files")[1] == "b.nc4", "list");
  check(r.getStringList("name").size() == 1, "a scalar as a one-entry list");
  check(r.has("years") && !r.has("missing"), "has");
  check(r.getOptional("missing").empty(), "optional");

  std::string description;
  check(r.nextCommunity(&description) == 3 && description == "Boreal", "first community");
  check(std::fabs(r.getDouble("leafcb") - 4.5) < 1e-15 && r.getInt("phen") == 1, "values of the open entry");
  r.endEntry();
  check(r.nextCommunity() == 7 && std::fabs(r.getDouble("leafcb") - 6.5) < 1e-15, "second community");

  check(stops("a: 0.5\n", [](tem::yaml::Reader& x) { x.getInt("a"); }), "0.5 is not an integer");
  check(stops("a: abc\n", [](tem::yaml::Reader& x) { x.getDouble("a"); }), "text is not a number");
  check(stops("a: 1\n", [](tem::yaml::Reader& x) { x.getInt("b"); }), "a missing key stops the run");
  check(stops("a: maybe\n", [](tem::yaml::Reader& x) { x.getBool("a"); }), "a bad boolean");
  check(stops("community:\n  - type: Boreal\n", [](tem::yaml::Reader& x) { x.nextCommunity(); }), "a community type that is text");
  check(stops("community:\n  - type: 1\n", [](tem::yaml::Reader& x) { x.nextCommunity(); x.nextCommunity(); }), "reading past the last community");
  check(stops("a: 1\na: 2\n", [](tem::yaml::Reader&) {}), "a duplicate key");

  std::cout << (failures ? "FAILED" : "PASS") << std::endl;
  return failures ? 1 : 0;
}
