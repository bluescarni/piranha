#include <boost/iostreams/device/mapped_file.hpp>

int main()
{
    boost::iostreams::mapped_file f("", boost::iostreams::mapped_file::readonly);
}
