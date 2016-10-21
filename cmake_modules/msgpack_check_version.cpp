#include <msgpack/version_master.h>

#if MSGPACK_VERSION_MAJOR < 2

#error Minimum msgpack-c version supported is 2.

#endif

int main()
{
    return 0;
}
