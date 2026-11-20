find_package(Qt6 6.4 REQUIRED COMPONENTS Test)

qt_add_executable(RegieTests
    test_regie.cpp
)

target_link_libraries(RegieTests PRIVATE
    Qt6::Test
    Qt6::Widgets
    Qt6::Multimedia
)

target_sources(RegieTests PRIVATE
    ../regie.cpp
    ../regie.h
)

enable_testing()
add_test(NAME RegieTests COMMAND RegieTests)
