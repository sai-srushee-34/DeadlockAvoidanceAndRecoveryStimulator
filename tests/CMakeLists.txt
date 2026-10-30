# ==============================================================================
# Deadlock Prevention & Recovery System - Test Targets
# ==============================================================================

# Unit Tests
add_executable(deadlock_tests
    unit/test_core.cpp
)
target_link_libraries(deadlock_tests PRIVATE
    deadlock_core
)
add_test(NAME CoreUnitTests COMMAND deadlock_tests)

add_executable(prevention_tests
    unit/test_prevention.cpp
)
target_link_libraries(prevention_tests PRIVATE
    deadlock_core
)
add_test(NAME PreventionUnitTests COMMAND prevention_tests)

add_executable(detection_tests
    unit/test_detection.cpp
)
target_link_libraries(detection_tests PRIVATE
    deadlock_core
)
add_test(NAME DetectionUnitTests COMMAND detection_tests)

add_executable(recovery_tests
    unit/test_recovery.cpp
)
target_link_libraries(recovery_tests PRIVATE
    deadlock_core
)
add_test(NAME RecoveryUnitTests COMMAND recovery_tests)

# Integration Tests
add_executable(api_tests
    integration/test_api.cpp
)
target_link_libraries(api_tests PRIVATE
    deadlock_core
)
add_test(NAME ApiIntegrationTests COMMAND api_tests)
