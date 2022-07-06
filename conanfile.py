from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout


class PiranhaConan(ConanFile):
    name = "piranha"
    version = "0.11"

    # Optional metadata
    license = "LGPL-3.0"
    author =  "Francesco Biscani <bluescarni@gmail.com>"
    url = "https://github.com/bluescarni/piranha"
    description = " The Piranha computer algebra system."
    topics = ("computer-algebra", "math-bignum")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "pyranha/*", "include/*", "src/*", "cmake_modules/*"
    requires = "boost/1.79.0", "libbacktrace/cci.20210118", "mppp/0.26", "bzip2/1.0.8", "gmp/6.2.1", "mpfr/4.1.0", "msgpack-c/4.0.0"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
