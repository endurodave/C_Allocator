# PlatformIO extra script: put the FreeRTOS library's headers on the path of
# every build, C_Allocator included.
#
# This example links C_Allocator from this repository (lib_deps symlink). The
# library's source folder is the repository root, which holds this example's
# .pio/libdeps, FreeRTOS included, so PlatformIO's dependency finder thinks
# C_Allocator provides FreeRTOS.h and doesn't give it the FreeRTOS library's
# include path. Not needed with lib_deps = endurodave/C_Allocator: the
# registry package has no .pio folder.
import glob
import os

Import("env")

libdeps = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))
for header in glob.glob(os.path.join(libdeps, "*", "src", "FreeRTOS.h")):
    env.Append(CPPPATH=[os.path.dirname(header)])
