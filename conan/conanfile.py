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
        ("entity/d5bbd4c725@adnn/develop"),
        ("graphics/3a9a713cde@adnn/develop"),
        ("handy/5c42e80ebf@adnn/develop"),
        ("math/a165ef4960@adnn/develop"),
        ("MarkovJunior.cpp/1c8bd4662f@adnn/develop"),

        ("spdlog/1.11.0"),
        ("nlohmann_json/3.11.2"),
    )

    # Note: It seems conventionnal to add CMake build requirement
    # directly to the build profile.
    #build_requires = ()

    build_policy = "missing"
    generators = "CMakeDeps", "CMakeToolchain"


    python_requires="shred_conan_base/0.0.5@adnn/stable"
    python_requires_extend="shred_conan_base.ShredBaseConanFile"
