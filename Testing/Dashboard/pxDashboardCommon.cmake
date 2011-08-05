# ITKTools Common Dashboard Script
#
# This script is shared among most praxix dashboard client machines.
# It contains basic dashboard driver code common to all clients.
#
# Checkout the directory containing this script to a path such as
# "/.../Dashboards/ctest-scripts/".  Create a file next to this
# script, say 'my_dashboard.cmake', with code of the following form:
#
#   # Client maintainer: someone@users.sourceforge.net
#   set(CTEST_SITE "machine.site")
#   set(CTEST_BUILD_NAME "Platform-Compiler")
#   set(CTEST_BUILD_CONFIGURATION Debug)
#   set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
#   include(${CTEST_SCRIPT_DIRECTORY}/pxDashboardCommon.cmake)
#
# Then run a scheduled task (cron job) with a command line such as
#
#   ctest -S /.../Dashboards/ctest-scripts/my_dashboard.cmake -V
#
# By default the source and build trees will be placed in the path
# "/.../Dashboards/MyTests/".
#
# The following variables may be set before including this script
# to configure it:
#
#   dashboard_model       = Nightly | Experimental | Continuous
#   dashboard_cache       = Initial CMakeCache.txt file content
#   dashboard_git_url     = Git url to clone
#   dashboard_git_branch  = branch to clone (default: master)
#   dashboard_do_coverage = True to enable coverage (ex: gcov)
#   dashboard_do_memcheck = True to enable memcheck (ex: valgrind)
#   dashboard_no_clean    = True to skip build tree wipeout
#   CTEST_GIT_COMMAND     = path to git command-line client
#   CTEST_BUILD_FLAGS     = build tool arguments (ex: -j2)
#   CTEST_DASHBOARD_ROOT  = Where to put source and build trees
#
# Note: this script is based on the vxl_common.cmake script and itk_common.cmake.
#

cmake_minimum_required( VERSION 2.8.3 )

set( CTEST_PROJECT_NAME ITKTools )

# Select the top dashboard directory.
if( NOT DEFINED CTEST_DASHBOARD_ROOT )
  get_filename_component( CTEST_DASHBOARD_ROOT
    "${CTEST_SCRIPT_DIRECTORY}/../MyTests" ABSOLUTE )
endif()

# Select the model (Nightly, Experimental, Continuous).
if( NOT DEFINED dashboard_model )
  set( dashboard_model Nightly )
endif()

# Default to a Release build.
if( NOT DEFINED CTEST_BUILD_CONFIGURATION )
  set( CTEST_BUILD_CONFIGURATION Release )
endif()

# Select git source to use.
if( NOT DEFINED dashboard_git_url )
  set( dashboard_git_url "git://github.com/ITKTools/ITKTools.git" )
endif()

# select branch to use
if( NOT DEFINED dashboard_git_branch )
  set( dashboard_git_branch master )
endif()

# Select GIT directory
if( NOT DEFINED CTEST_ITKTOOLS_DIRECTORY )
  set( CTEST_ITKTOOLS_DIRECTORY "${CTEST_DASHBOARD_ROOT}/ITKTools" )
endif()

# Select a source directory name.
if( NOT DEFINED CTEST_SOURCE_DIRECTORY )
  set( CTEST_SOURCE_DIRECTORY "${CTEST_ITKTOOLS_DIRECTORY}/src" )
endif()

# Select a build directory name.
# Note: We cannot put the bin directory on the same level as the
# src directory (as we recommend in the README.md), because the
# bin directory is created before git clone is called, and git clone
# demands an empty directory.
if( NOT DEFINED CTEST_BINARY_DIRECTORY )
  set( CTEST_BINARY_DIRECTORY ${CTEST_DASHBOARD_ROOT}/bin )
endif()
make_directory( ${CTEST_BINARY_DIRECTORY} )

# Look for a GIT command-line client.
if( NOT DEFINED CTEST_GIT_COMMAND )
  find_program( CTEST_GIT_COMMAND NAMES git git.cmd )
endif()
if( NOT DEFINED CTEST_GIT_COMMAND )
  message( FATAL_ERROR "No Git Found." )
endif()

# Look for a coverage command-line client.
if( NOT DEFINED CTEST_COVERAGE_COMMAND )
  find_program( CTEST_COVERAGE_COMMAND gcov )
endif()

# Look for a memory check command-line client.
if( NOT DEFINED CTEST_MEMORYCHECK_COMMAND )
  find_program( CTEST_MEMORYCHECK_COMMAND valgrind )
endif()

# Dangerous option: you might delete something by accident that you don't want to delete.
# Better given an error message.
#
# Delete source tree if it is incompatible with current Version control system (VCS).
#if(EXISTS ${CTEST_DASHBOARD_ROOT})
#  if(NOT EXISTS "${CTEST_DASHBOARD_ROOT}/.git")
#    set(vcs_refresh "because it is not managed by git.")
#  endif()
#  if(vcs_refresh AND "${CTEST_DASHBOARD_ROOT}" MATCHES "/(ITKTools|itktools|itkTools|ITKtools)[^/]*")
#    message("Deleting source tree\n  ${CTEST_DASHBOARD_ROOT}\n${vcs_refresh}")
#    file(REMOVE_RECURSE "${CTEST_DASHBOARD_ROOT}")
#  endif()
#endif()


# Support initial checkout if necessary;
if( NOT EXISTS "${CTEST_SOURCE_DIRECTORY}"
    AND NOT DEFINED CTEST_CHECKOUT_COMMAND
    AND CTEST_GIT_COMMAND )

  # Assume git version 1.6.5 or higher, which has git clone -b option.
  set( CTEST_CHECKOUT_COMMAND
     "\"${CTEST_GIT_COMMAND}\" clone -b ${dashboard_git_branch} \"${dashboard_git_url}\" \"${CTEST_ITKTOOLS_DIRECTORY}\"" )

  # CTest delayed initialization is broken, so we copy the
  # CTestConfig.cmake info here.
  # SK: Otherwise submission fails.
  set( CTEST_PROJECT_NAME "ITKTools" )
  set( CTEST_NIGHTLY_START_TIME "00:01:00 CET" )
  set( CTEST_DROP_METHOD "http" )
  set( CTEST_DROP_SITE "my.cdash.org" )
  set( CTEST_DROP_LOCATION "/submit.php?project=itktools" )
  set( CTEST_DROP_SITE_CDASH TRUE )

endif()

# Send the main script as a note, and this script
list( APPEND CTEST_NOTES_FILES
  "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
  "${CMAKE_CURRENT_LIST_FILE}" )

# Check for required variables.
foreach( req
  CTEST_CMAKE_GENERATOR
  CTEST_SITE
  CTEST_BUILD_NAME
  )
  if( NOT DEFINED ${req} )
    message( FATAL_ERROR "The containing script must set ${req}" )
  endif()
endforeach()

# Print summary information.
foreach( v
  CTEST_SITE
  CTEST_BUILD_NAME
  CTEST_DASHBOARD_ROOT
  CTEST_ITKTOOLS_DIRECTORY
  CTEST_SOURCE_DIRECTORY
  CTEST_BINARY_DIRECTORY
  CTEST_CMAKE_GENERATOR
  CTEST_BUILD_CONFIGURATION
  CTEST_GIT_COMMAND
  CTEST_CHECKOUT_COMMAND
  CTEST_COVERAGE_COMMAND
  CTEST_MEMORYCHECK_COMMAND
  CTEST_SCRIPT_DIRECTORY
  dashboard_git_url
  dashboard_git_branch
  dashboard_model
  )
  set( vars "${vars}  ${v}=[${${v}}]\n" )
endforeach()
message( "Dashboard script configuration:\n${vars}\n" )

# Avoid non-ascii characters in tool output.
set( ENV{LC_ALL} C )

# Helper macro to write the initial cache.
macro( write_cache )
  set( cache_build_type "" )
  set( cache_make_program "" )
  if( CTEST_CMAKE_GENERATOR MATCHES "Make" )
    set( cache_build_type CMAKE_BUILD_TYPE:STRING=${CTEST_BUILD_CONFIGURATION} )
    if( CMAKE_MAKE_PROGRAM )
      set( cache_make_program CMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM} )
    endif()
  endif()
  file( WRITE ${CTEST_BINARY_DIRECTORY}/CMakeCache.txt "
SITE:STRING=${CTEST_SITE}
BUILDNAME:STRING=${CTEST_BUILD_NAME}
${cache_build_type}
${cache_make_program}
${dashboard_cache}
")
endmacro()

# Start with a fresh build tree.
file( MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}" )
if( NOT "${CTEST_SOURCE_DIRECTORY}" STREQUAL "${CTEST_BINARY_DIRECTORY}"
    AND NOT dashboard_no_clean )
  message( "Clearing build tree..." )
  ctest_empty_binary_directory( ${CTEST_BINARY_DIRECTORY} )
endif()

# Support each testing model
if( dashboard_model STREQUAL Continuous )
  # Build once and then when updates are found.
  while( ${CTEST_ELAPSED_TIME} LESS 43200 )
    set( START_TIME ${CTEST_ELAPSED_TIME} )
    ctest_start( Continuous )

    # always build if the tree is missing
    set( FRESH_BUILD OFF )
    if( NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" )
      message( "Starting fresh build..." )
      write_cache()
      set( FRESH_BUILD ON )
    endif()

    # Check for changes
    ctest_update( SOURCE ${CTEST_ITKTOOLS_DIRECTORY} RETURN_VALUE res )
    message( "Found ${res} changed files" )
    # SK: Only do initial checkout at the first iteration.
    # After that, the CHECKOUT_COMMAND has to be removed, otherwise
    # "svn update" will never see any changes.
    set( CTEST_CHECKOUT_COMMAND )

    if( (res GREATER 0) OR (FRESH_BUILD) )
      # run cmake twice; this seems to be necessary, otherwise the
      # KNN lib is not built
      #ctest_configure()
      # # only for elastix; for ITKTools once should be sufficient:
      ctest_configure()
      ctest_read_custom_files( ${CTEST_BINARY_DIRECTORY} )
      ctest_build()
      ctest_test( ${CTEST_TEST_ARGS} )
      ctest_submit()
    endif()

    # Delay until at least 10 minutes past START_TIME
    ctest_sleep( ${START_TIME} 600 ${CTEST_ELAPSED_TIME} )
  endwhile()
else()
  write_cache()
  ctest_start( ${dashboard_model} )
  ctest_update( SOURCE ${CTEST_ITKTOOLS_DIRECTORY} )
  # run cmake twice; this seems to be necessary, otherwise the
  # KNN lib is not built
  #ctest_configure()
  # # only for elastix; for ITKTools once should be sufficient:
  ctest_configure()
  ctest_read_custom_files( ${CTEST_BINARY_DIRECTORY} )
  ctest_build()
  ctest_test( ${CTEST_TEST_ARGS} )
  if( dashboard_do_coverage )
    ctest_coverage()
  endif()
  if( dashboard_do_memcheck )
    ctest_memcheck()
  endif()
  # Submit results, retry every 5 minutes for a maximum of two hours
  ctest_submit( RETRY_COUNT 24 RETRY_DELAY 300 )
endif()

