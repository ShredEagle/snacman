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
        ("entity/38ecfecf59@adnn/develop"),
        ("graphics/a494da01c2@adnn/develop"),
        ("handy/15a1bb8eaa@adnn/develop"),
        ("math/93cb736b19@adnn/develop"),
        ("MarkovJunior.cpp/c6fb4e32d1@adnn/develop"),

        # Waiting for my PR on conan-center for assimp to get merged in
        # see: https://github.com/conan-io/conan-center-index/pull/20185
        # I manually exported the recipe and uploaded it.
        ("assimp/5.3.1@adnn/develop"),
        ("cli11/2.4.2"),
        ("spdlog/1.13.0"),
        ("nlohmann_json/3.11.2"),

        # Intel Instrumentation and Tracing Technology API
        ("ittapi/3.24.4"),

        # The overrides (who will think about reviewing them?)
        ("zlib/1.3"),
    )

    # Note: It seems conventionnal to add CMake build requirement
    # directly to the build profile.
    #build_requires = ()

    build_policy = "missing"
    generators = "CMakeDeps", "CMakeToolchain"


    python_requires="shred_conan_base/0.0.5@adnn/stable"
    python_requires_extend="shred_conan_base.ShredBaseConanFile"
