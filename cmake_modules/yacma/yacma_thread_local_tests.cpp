#include <vector>
#include <iostream>

int main()
{
    static thread_local std::vector<char> n;
    std::cout << n.size() << std::endl;
}
