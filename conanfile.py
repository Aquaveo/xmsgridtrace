"""
XMSGridtrace Conanfile and Support
"""
import os
from conans import ConanFile, CMake, tools
from conans.errors import ConanException


class XmsgridtraceConan(ConanFile):
    """XMSGridtrace Conanfile"""
    name = "xmsgridtrace"
    version = None
    license = "XMSNG Software License"
    url = "https://github.com/Aquaveo/xmsgridtrace"
    description = "Grid library for XMS products"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "xms": [True, False],
        "pybind": [True, False],
        "testing": [True, False],
    }
    default_options = "xms=False", "pybind=False", "testing=False"
    generators = "cmake"
    build_requires = "cxxtest/4.4@aquaveo/stable"
    exports = "CMakeLists.txt", "LICENSE", "test_files/*"
    exports_sources = "xmsgridtrace/*", "test_files/*"

    def configure(self):
        # Set version dynamically using XMS_VERSION env variable.
        self.version = self.env.get('XMS_VERSION', '99.99.99')

        # Raise ConanExceptions for Unsupported Versions
        s_os = self.settings.os
        s_compiler = self.settings.compiler
        s_compiler_version = self.settings.compiler.version

        self.options['xmscore'].xms = self.options.xms
        self.options['xmscore'].pybind = self.options.pybind
        self.options['xmscore'].testing = self.options.testing

        self.options['xmsinterp'].xms = self.options.xms
        self.options['xmsinterp'].pybind = self.options.pybind
        self.options['xmsinterp'].testing = self.options.testing

        self.options['xmsgrid'].xms = self.options.xms
        self.options['xmsgrid'].pybind = self.options.pybind
        self.options['xmsgrid'].testing = self.options.testing

        self.options['xmsextractor'].xms = self.options.xms
        self.options['xmsextractor'].pybind = self.options.pybind
        self.options['xmsextractor'].testing = self.options.testing

        if s_compiler == "apple-clang" and s_os == 'Linux':
            raise ConanException("Clang on Linux is not supported.")

        if s_compiler == "apple-clang" \
                and s_os == 'Macos' \
                and float(s_compiler_version.value) < 9.0:
            raise ConanException("Clang > 9.0 is required for Mac.")

    def requirements(self):
        """Requirements"""
        # If building for XMS, use the older, custom boost
        if self.options.xms and self.settings.compiler.version == "12":
            self.requires("boost/1.60.0@aquaveo/testing")
        else:
            self.requires("boost/1.66.0@conan/stable")
        # Pybind if not Visual studio 2013
        if not (self.settings.compiler == 'Visual Studio' \
                and self.settings.compiler.version == "12") \
                and self.options.pybind:
            self.requires("pybind11/2.2.2@aquaveo/stable")

        # Use the dev version of XMSCore, XMSInterp, XMSGrid, XMSExtractor
        self.requires("xmscore/1.0.40@aquaveo/stable")
        self.requires("xmsinterp/1.0.18@aquaveo/stable")
        self.requires("xmsgrid/1.0.10@aquaveo/stable")
        self.requires("xmsextractor/1.0.0@aquaveo/stable")

    def build(self):
        cmake = CMake(self)

        if self.settings.compiler == 'Visual Studio' \
           and self.settings.compiler.version == "12":
            cmake.definitions["XMS_BUILD"] = self.options.xms

        # CXXTest doesn't play nice with PyBind. Also, it would be nice to not
        # have tests in release code. Thus, if we want to run tests, we will
        # build a test version (without python), run the tests, and then (on
        # sucess) rebuild the library without tests.
        cmake.definitions["XMS_VERSION"] = '{}'.format(self.version)
        cmake.definitions["IS_PYTHON_BUILD"] = self.options.pybind
        cmake.definitions["BUILD_TESTING"] = self.options.testing
        cmake.definitions["XMS_TEST_PATH"] = "test_files"
        cmake.definitions["PYTHON_TARGET_VERSION"] = self.env.get("PYTHON_TARGET_VERSION", "3.6")
        cmake.configure(source_folder=".")
        cmake.build()

        if self.options.testing:
            print("***********(0.0)*************")
            try:
                cmake.test()
            except ConanException:
                raise
            finally:
                if os.path.isfile("TEST-cxxtest.xml"):
                    with open("TEST-cxxtest.xml", "r") as f:
                        for line in f.readlines():
                            no_newline = line.strip('\n')
                            print(no_newline)
                print("***********(0.0)*************")
        elif self.options.pybind:
            with tools.pythonpath(self):
                if not self.settings.os == "Macos":
                  self.run('pip install --user numpy')
                else:
                  self.run('pip install numpy')
                self.run('python -m unittest discover -v -p *_pyt.py -s ../xmsgridtrace/python', cwd="./lib")

    def package(self):
        self.copy("*.h", dst="include/xmsgridtrace", src="xmsgridtrace")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.pyd", dst="lib", keep_path=False)
        self.copy("*_py.*.so", dst="site-packages", keep_path=False)
        self.copy("*_py.so", dst="site-packages", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)
        self.copy("license", dst="licenses", ignore_case=True, keep_path=False)

    def package_info(self):
        self.env_info.PYTHONPATH.append(os.path.join(self.package_folder, "site-packages"))
        if self.settings.build_type == 'Debug':
            self.cpp_info.libs = ["xmsgridtracelib_d"]
        else:
            self.cpp_info.libs = ["xmsgridtracelib"]
