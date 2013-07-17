#include <type_traits>

int main()
{
	sizeof(std::is_trivially_copyable<int>);
	return 0;
}
