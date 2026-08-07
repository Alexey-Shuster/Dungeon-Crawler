from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.errors import ConanInvalidConfiguration


class DungeonsProjectConan(ConanFile):
    name = "dungeons_project"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "fPIC": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "boost/*:shared": False,
        "boost/*:cxxstd": "20",
        "boost/*:with_coroutine": True,
        "boost/*:with_signals2": True,
        "gtest/*:cxxstd": "20"
    }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            if str(self.settings.compiler.get_safe("cppstd")) not in ["20", "23", "gnu20", "gnu23"]:
                raise ConanInvalidConfiguration("This project requires a compiler with C++20/23 support!")

    def requirements(self):
        self.requires("boost/1.86.0")
        self.requires("gtest/1.17.0")
        self.requires("libcbor/0.13.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # creates files for CMake (including CMakeUserPresets.json)
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        # allows to build a project using the 'conan build .'
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
