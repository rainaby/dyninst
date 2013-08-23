# - Try to find DIASDK
# Once done this will define
#
#  DIASDK_FOUND - system has DIASDK
#  DIASDK_INCLUDE_DIRS - the DIASDK include directory
#  DIASDK_LIBRARIES - Link these to use DIASDK
#  DIASDK_DEFINITIONS - Compiler switches required for using DIASDK
#
if(WIN32)
if (DIASDK_INCLUDE_DIRS)
  set (DIASDK_FIND_QUIETLY TRUE)
endif (DIASDK_INCLUDE_DIRS)

if (MSVC11)
	GET_FILENAME_COMPONENT(VS_DIR [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\11.0\\Setup\\VS;ProductDir] REALPATH CACHE)
elseif (MSVC10)
    GET_FILENAME_COMPONENT(VS_DIR [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\10.0\\Setup\\VS;ProductDir] REALPATH CACHE)
elseif (MSVC90)
    GET_FILENAME_COMPONENT(VS_DIR [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\9.0\\Setup\\VS;ProductDir] REALPATH CACHE)
elseif (MSVC80)
    GET_FILENAME_COMPONENT(VS_DIR [HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\Setup\\VS;ProductDir] REALPATH CACHE)
endif()

find_path (DIASDK_INCLUDE_DIR
    NAMES
      cvconst.h
    HINTS
      ${DIASDK_INCLUDE_DIRS}
    PATHS
      ENV CPATH
      "${VS_DIR}/DIA SDK/include") # PATH and INCLUDE will also work

#find_library (DIASDK_LIBRARIES
#    NAMES
#      dbghlp
#    HINTS
#      ${DIASDK_LIBRARIES}
#    PATHS
#      ENV LIBRARY_PATH   # PATH and LIB will also work
#      ENV LD_LIBRARY_PATH)
include (FindPackageHandleStandardArgs)


# handle the QUIETLY and REQUIRED arguments and set DIASDK_FOUND to TRUE
# if all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIASDK DEFAULT_MSG
    DIASDK_INCLUDE_DIR)
endif()

