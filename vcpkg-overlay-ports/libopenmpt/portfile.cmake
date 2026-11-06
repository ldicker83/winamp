vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO OpenMPT/openmpt
    REF "libopenmpt-${VERSION}"
    SHA512 b6770adc29a4898596a761c998c3de8d3f85973adb947ed362e397e821e5cc82ec0d6cd34d17024e7060baeb796274d2e543bfa7bd2a750f94e8e0507929f0d4
    HEAD_REF master
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        "-DVERSION=${VERSION}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    vcpkg_replace_string(${CURRENT_PACKAGES_DIR}/include/libopenmpt/libopenmpt_config.h "defined(LIBOPENMPT_USE_DLL)" "0")
else()
    vcpkg_replace_string(${CURRENT_PACKAGES_DIR}/include/libopenmpt/libopenmpt_config.h "defined(LIBOPENMPT_USE_DLL)" "1")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
