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
        ("entity/1e5610241c@adnn/develop"),
        ("graphics/5b0149ff9a@adnn/develop"),
        ("handy/e2b164a804@adnn/develop"),
        ("MarkovJunior.cpp/f7022ac731@adnn/develop"),
        ("math/329f6705a1@adnn/develop"),
        ("reflexion/57198786a6@adnn/develop"),

        ("implot/0.16"),  # MIT
        ("imgui/1.89.8"),

        # Waiting for my PR on conan-center for assimp to get merged in
        # see: https://github.com/conan-io/conan-center-index/pull/20185
        # I manually exported the recipe and uploaded it.
        ("cli11/2.4.2"),
        ("assimp/5.4.2"),
        ("spdlog/1.13.0"),
        # ("spdlog/1.13.0@#1e0f4eb6338d05e4bd6fcc6bf4734172"),
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
