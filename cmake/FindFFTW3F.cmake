# - Find FFTW3F
# Find the native FFTW3 includes and library, single precision
#
#  FFTW3F_INCLUDE_DIR    - where to find fftw3.h
#  FFTW3F_LIBRARIES   - List of libraries when using FFTW.
#  FFTW3F_FOUND       - True if FFTW found.

if (FFTW3F_INCLUDE_DIR AND FFTW3F_LIBRARIES)
  # Already in cache, be silent
  set (FFTW3F_FIND_QUIETLY TRUE)
endif (FFTW3F_INCLUDE_DIR AND FFTW3F_LIBRARIES)

find_path (FFTW3F_INCLUDE_DIR fftw3.h 
	CACHE STRING "Path to headers for single precision FFTW3")

find_library (FFTW3F_LIBRARIES 
		NAMES fftw3f
		PATHS "${FFTW3F_INCLUDE_DIR}/../lib"
		CACHE STRING "Single precision FFTW3 libraries")

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (FFTW3F DEFAULT_MSG FFTW3F_LIBRARIES FFTW3F_INCLUDE_DIR)

mark_as_advanced (FFTW3F_LIBRARIES FFTW3F_INCLUDE_DIR)
