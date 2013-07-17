#include <type_traits>

int main()
{
	sizeof(std::has_trivial_destructor<int>);
	return 0;
}
