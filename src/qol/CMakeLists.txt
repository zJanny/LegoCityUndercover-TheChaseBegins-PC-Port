add_library(chase_qol settings.cpp display_tab.cpp menu_model.cpp view_layout.cpp game_tab.cpp)
target_include_directories(chase_qol PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(chase_qol PUBLIC cxx_std_17)
target_compile_options(chase_qol PRIVATE -Wall -Wextra -Wpedantic)
