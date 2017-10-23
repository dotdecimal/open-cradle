# This file extracts information about a Subversion working copy and writes
# it to a C++ header file.
#
# The following should be defined when invoking it.
# SOURCE_DIR: The directory containing the working copy.
# SVN_INFO_FILE: The path to the header file that will contain the info.
# PREFIX: The prefix to use for identifiers in the header file.

# The FindSubversion.cmake module is part of the standard distribution.
include(FindSubversion)

# Extract the working copy information into MY_XXX variables.
Subversion_WC_INFO(${SOURCE_DIR} MY)

set(temp_file "${SVN_INFO_FILE}.tmp")

# Write a temporary copy of the header file.
# (Apparently there's no easy way to split strings over multiple lines.)
file(WRITE ${temp_file}
     "#define ${PREFIX}_SVN_REVISION ${MY_WC_REVISION}\n#define ${PREFIX}_SVN_URL \"${MY_WC_URL}\"\n")

# Copy the file to the final header only if the info changes.
# This reduces needless rebuilds of dependent source files.
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${temp_file} ${SVN_INFO_FILE})
