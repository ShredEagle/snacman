#ifdef __clang__
#define SNAC_COMPILER_CLANG
#endif
#if defined(__GNUC__) && !defined(__clang__)
#define SNAC_COMPILER_GCC
#endif
#ifdef _MSC_VER
#define SNAC_COMPILER_MSVC
#endif

#define STRINGIFY_(a) #a
#define STRINGIFY(a) STRINGIFY_(a)
#define GLUE_(a,b) a##b
#define GLUE(a,b) GLUE_(a,b)

#define WARNING(a) GLUE(-W,a)

#if defined(SNAC_COMPILER_GCC)
    #define SNAC_GCC_PUSH_DIAG(diag) \
        _Pragma(STRINGIFY(GCC diagnostic push)) \
        _Pragma(STRINGIFY(GCC diagnostic ignored STRINGIFY(WARNING(diag))))
    #define SNAC_GCC_POP_DIAG() \
        _Pragma(STRINGIFY(GCC diagnostic pop))
#else
    #define SNAC_GCC_PUSH_DIAG(diag)
    #define SNAC_GCC_POP_DIAG()
#endif

#if defined(SNAC_COMPILER_CLANG)
    #define SNAC_CLANG_PUSH_DIAG(diag) \
        _Pragma(STRINGIFY(clang diagnostic push))\
        _Pragma(STRINGIFY(clang diagnostic ignored STRINGIFY(WARNING(diag))))
    #define SNAC_CLANG_POP_DIAG() \
        _Pragma(STRINGIFY(clang diagnostic pop))
#else
    #define SNAC_CLANG_PUSH_DIAG(diag)
    #define SNAC_CLANG_POP_DIAG()
#endif

