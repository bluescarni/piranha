#if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 1)
	#error Unsupported Clang version.
#endif

int main()
{
	return 0;
}
