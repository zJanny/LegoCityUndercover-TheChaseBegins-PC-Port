add_executable(qol-settings-test settings_test.cpp)
target_link_libraries(qol-settings-test PRIVATE chase_qol)
target_compile_options(qol-settings-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME qol-settings COMMAND qol-settings-test)

add_executable(qol-view-layout-test view_layout_test.cpp)
target_link_libraries(qol-view-layout-test PRIVATE chase_qol)
target_compile_options(qol-view-layout-test PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME qol-view-layout COMMAND qol-view-layout-test)
