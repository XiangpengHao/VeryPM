add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test gtest_main glog::glog)
gtest_add_tests(TARGET smoke_test)

add_executable(epoch_test epoch_test.cpp)
target_link_libraries(epoch_test gtest_main glog::glog)
gtest_add_tests(TARGET epoch_test)

add_executable(garbage_test garbage_list_test.cpp)
target_link_libraries(garbage_test gtest_main glog::glog)
gtest_add_tests(TARGET garbage_test)
