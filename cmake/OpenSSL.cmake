if(MINGW)
    set(ENV{CONFIGURE_CHECKER_WARN} "1")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OPENSSL_TARGET_PLATFORM "mingw64" CACHE STRING "OpenSSL Configure target")
    else()
        set(OPENSSL_TARGET_PLATFORM "mingw32" CACHE STRING "OpenSSL Configure target")
    endif()
    set(OPENSSL_CONFIGURE_OPTIONS
        "no-asm;--openssldir=/usr/local/ssl"
        CACHE STRING "Extra OpenSSL Configure options"
    )

    include(ProcessorCount)
    ProcessorCount(N)
    if(N EQUAL 0)
        set(N 1)
    endif()

    set(OPENSSL_BUILD_OPTIONS
        "RM=cmd /c del /F /Q"
        "-j"
        "${N}"
        CACHE STRING "Extra OpenSSL build options"
    )

    set(OPENSSL_USE_CCACHE OFF CACHE BOOL "Disable ccache for OpenSSL")
endif()

CPMAddPackage(
   NAME openssl-cmake
   GIT_REPOSITORY "https://github.com/jimmy-park/openssl-cmake"
   GIT_TAG "4.1.0"
)

