#include <type_traits>

int main()
{
	sizeof(std::has_trivial_copy_constructor<int>);
	return 0;
}
