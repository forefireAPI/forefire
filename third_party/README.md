# Vendored third-party code

Dependencies that are checked in rather than fetched, so that a build and a CI
run need no network access beyond cloning the repository.

Nothing here is compiled on its own: `CMakeLists.txt` globs `src/*.cpp` only.
These are headers, included by whichever target needs them. Both `libforefireL`
and the test target have this directory on their include path, in both cases as
a `SYSTEM` include, so the project's `-Wall -Wextra` does not report code we do
not maintain.

| File or directory | Version | License |
| --- | --- | --- |
| `doctest/` | 2.5.3 (2026-07-06) | MIT |
| `stb_image_write.h` | 1.16 | public domain / MIT |

## doctest

The unit-test framework, used by `tests/unit/`. A single header, taken
unmodified from
<https://github.com/doctest/doctest/blob/v2.5.3/doctest/doctest.h>.

The 2.5 series gates its C++17 features behind `DOCTEST_CPLUSPLUS` checks, so
it still builds under the project's `CMAKE_CXX_STANDARD 11`.

To update it, replace the file with a newer tagged release and run
`ctest --test-dir build`. Nothing in `tests/unit/` uses anything beyond
`TEST_SUITE`, `TEST_CASE`, `CHECK`, `REQUIRE`, `CAPTURE` and `doctest::Approx`.

## stb_image_write

Writes the PNG output of the `plot` command. Taken unmodified from
<https://github.com/nothings/stb/blob/master/stb_image_write.h>, v1.16.

Header-only, and header-only in the awkward sense: the implementation is
compiled in wherever `STB_IMAGE_WRITE_IMPLEMENTATION` is defined before the
include, which `src/Command.cpp` does and nothing else may.

To update it, replace the file with a newer release and rebuild. ForeFire uses
`stbi_write_png` only.
