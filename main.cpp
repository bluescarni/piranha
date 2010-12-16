#include <iostream>
#include <thread>
#include <vector>

#include "src/runtime_info.hpp"
#include "src/thread_management.hpp"

int main()
{
	std::cout << std::thread::hardware_concurrency() << '\n';
	std::cout << piranha::runtime_info::hardware_concurrency() << '\n';
// 	piranha::thread_management::bind_to_proc(7);
	std::cout << piranha::thread_management::bound_proc().first << '\n';
	std::cout << piranha::thread_management::bound_proc().second << '\n';
}