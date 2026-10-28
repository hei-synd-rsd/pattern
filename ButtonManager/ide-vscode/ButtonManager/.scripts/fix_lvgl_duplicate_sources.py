# Fix for "Multiple ways to build the same target" error caused by duplicate
# source filenames in the LVGL library (e.g. vg_lite_matrix.c exists in both
# src/debugging/vg_lite_tvg/ and src/libs/vg_lite_driver/VGLite/).
#
# This script runs as a PlatformIO pre-build extra script. It patches the LVGL
# build configuration files so that only one copy of each same-named source
# file is compiled, preventing SCons object-file collisions.
#
# Usage: add "pre:.scripts/fix_lvgl_duplicate_sources.py" to extra_scripts in
# platformio.ini

import os

Import("env")

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-zephyr")

# Marker used to detect if the patch has already been applied
_PATCH_MARKER = "# PATCHED_BY_FIX_LVG DUPLICATE_SOURCES"


def _patch_file(file_path, old_text, new_text):
    """Patch a text file if the marker is not already present."""
    if not os.path.isfile(file_path):
        return False

    with open(file_path, "r") as f:
        content = f.read()

    if _PATCH_MARKER in content:
        return False  # already patched

    if old_text not in content:
        return False  # nothing to patch

    content = content.replace(old_text, new_text, 1)

    with open(file_path, "w") as f:
        f.write(content)

    print("Fixed: %s" % file_path)
    return True


def force_cmake_reconfigure():
    """Force CMake to re-run on the next build so patched files take effect."""
    build_dir = env.subst("$BUILD_DIR")
    cmake_api_reply_dir = os.path.join(
        build_dir, ".cmake", "api", "v1", "reply"
    )
    if os.path.isdir(cmake_api_reply_dir):
        # Remove reply files so PlatformIO detects reconfigure is required
        for fname in os.listdir(cmake_api_reply_dir):
            fpath = os.path.join(cmake_api_reply_dir, fname)
            if os.path.isfile(fpath):
                os.remove(fpath)
        print("Forced CMake reconfiguration by clearing API reply directory")


def fix_lvgl_duplicate_sources():
    """Apply patches to eliminate duplicate LVGL source files."""
    patched = False

    # ------------------------------------------------------------------
    # 1. Patch os_desktop.cmake: deduplicate sources by basename after GLOB
    # ------------------------------------------------------------------
    os_desktop_cmake = os.path.join(
        FRAMEWORK_DIR,
        "_pio",
        "modules",
        "lib",
        "gui",
        "lvgl",
        "env_support",
        "cmake",
        "os_desktop.cmake",
    )

    old_glob = """file(GLOB_RECURSE SOURCES ${LVGL_ROOT_DIR}/src/*.c
                          ${LVGL_ROOT_DIR}/src/*.cpp
                          ${LVGL_ROOT_DIR}/src/*.S)"""

    new_glob = old_glob + """

%s
# Deduplicate source files by basename to avoid "Multiple ways to build the
# same target" errors when the same filename exists in multiple subdirectories
# (e.g. vg_lite_matrix.c in debugging/vg_lite_tvg/ and libs/vg_lite_driver/VGLite/)
set(_lvgl_unique_sources "")
foreach(_src ${SOURCES})
    get_filename_component(_basename ${_src} NAME)
    list(FIND _lvgl_unique_sources ${_basename} _found)
    if(_found EQUAL -1)
        list(APPEND _lvgl_unique_sources ${_src})
    endif()
endforeach()
set(SOURCES ${_lvgl_unique_sources})
""" % _PATCH_MARKER

    if _patch_file(os_desktop_cmake, old_glob, new_glob):
        patched = True

    # ------------------------------------------------------------------
    # 2. Patch Zephyr module CMakeLists.txt: remove duplicate vg_lite_matrix.c
    #    from the NXP VGLite driver section (keeps the debugging/TinyVG copy)
    # ------------------------------------------------------------------
    zephyr_lvgl_cmake = os.path.join(
        FRAMEWORK_DIR,
        "modules",
        "lvgl",
        "CMakeLists.txt",
    )

    old_line = (
        "    ${LVGL_DIR}/src/libs/vg_lite_driver/VGLite/vg_lite_matrix.c\n"
    )
    new_line = (
        "    %s # DISABLED: duplicate of src/debugging/vg_lite_tvg/vg_lite_matrix.c\n"
        % _PATCH_MARKER
    )

    if _patch_file(zephyr_lvgl_cmake, old_line, new_line):
        patched = True

    # ------------------------------------------------------------------
    # 3. Force CMake reconfiguration if any patch was applied
    # ------------------------------------------------------------------
    if patched:
        force_cmake_reconfigure()
        print("LVGL duplicate source fix applied. CMake will re-run on this build.")
    else:
        print("LVGL duplicate source fix: already applied or no patch needed.")


fix_lvgl_duplicate_sources()
