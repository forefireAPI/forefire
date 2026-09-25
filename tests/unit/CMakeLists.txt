# ----------------------------------
# C++ unit tests
# ----------------------------------
# These link the core library and exercise single models in isolation, which
# is the half of the code tests/runff cannot reach: it runs one case through
# one propagation model, so a regression in any of the other models is
# invisible to it.

add_executable(forefire_unit_tests
    main.cpp
    model_sandbox.cpp
    test_domain_ownership.cpp
    test_flux_models.cpp
    test_http_server.cpp
    test_model_registry.cpp
    test_propagation_models.cpp)

target_link_libraries(forefire_unit_tests PRIVATE forefireL)
target_include_directories(forefire_unit_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}")
# SYSTEM, so the warning flags below stop at code this project maintains.
target_include_directories(forefire_unit_tests SYSTEM PRIVATE
    "${PROJECT_SOURCE_DIR}/third_party")

if(_ff_warning_flags)
    target_compile_options(forefire_unit_tests PRIVATE ${_ff_warning_flags})
endif()

# doctest groups its cases into suites; registering one CTest entry per suite
# keeps `ctest` output useful without needing doctest's CMake integration.
foreach(_suite "model registry" "propagation models" "flux models" "http server"
                "domain ownership")
    string(REPLACE " " "_" _suite_id "${_suite}")
    add_test(NAME "unit.${_suite_id}"
             COMMAND forefire_unit_tests --test-suite=${_suite} --no-skipped-summary)
endforeach()
