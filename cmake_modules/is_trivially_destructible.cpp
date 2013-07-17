#include <type_traits>

int main()
{
	sizeof(std::is_trivially_destructible<int>);
	return 0;
}
