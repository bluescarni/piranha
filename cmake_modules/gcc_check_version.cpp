#if __GNUC__  < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
	#error Unsupported GCC version.
#endif

int main()
{
	return 0;
}
