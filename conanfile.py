from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy

class MrManager(ConanFile):
    name = "mr-manager"
    version = "1.0.2"
    license = "MIT"

    description = "Wait-Free object manager with per-type memory pools"

    author = "Michael Tsukanov (mt6@4j-company.ru)"
    url = "https://github.com/4j-company/mr-manager"

    settings = "os", "compiler", "build_type", "arch"

    exports_sources = "CMakeLists.txt", "include/*"

    package_type = "header-library"
    implements = ["auto_header_only"]

    def validate(self):
        check_min_cppstd(self, "17")

    def requirements(self):
        self.requires("folly/2024.08.12.00")

    def build_requirements(self):
        self.test_requires("gtest/1.14.0")

    def layout(self):
        basic_layout(self)

    def package(self):
        copy(self, "include/*", self.source_folder, self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.requires = ["folly::folly"]

