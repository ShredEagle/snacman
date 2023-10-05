from conans import ConanFile


class SnacmanConan(ConanFile):
    """ Conan recipe for Snacman """

    name = "snacman"
    license = "MIT License"
    url = "https://github.com/Adnn/snacman"
    description = "A 3D Snacman."
    #topics = ("", "", ...)
    settings = ("os", "compiler", "build_type", "arch")
    options = {
        "build_tests": [True, False],
        "shared": [True, False],
        "visibility": ["default", "hidden"],
    }
    default_options = {
        "build_tests": False,
        "shared": False,
        "visibility": "hidden",
    }

    requires = (
        ("entity/e3b28a133b@adnn/develop"),
        ("graphics/89593a18fe@adnn/develop"),
        ("handy/3e495de542@adnn/develop"),
        ("math/dd29e310ac@adnn/develop"),
        ("MarkovJunior.cpp/01458bc65b@adnn/develop"),
        ("implot/0.14"), # MIT
        ("imgui/1.89.8"),

        ("spdlog/1.11.0@#1e0f4eb6338d05e4bd6fcc6bf4734172"),
        ("nlohmann_json/3.11.2"),
    )

    # Note: It seems conventionnal to add CMake build requirement
    # directly to the build profile.
    #build_requires = ()

    build_policy = "missing"
    generators = "CMakeDeps", "CMakeToolchain"


    python_requires="shred_conan_base/0.0.5@adnn/stable"
    python_requires_extend="shred_conan_base.ShredBaseConanFile"
