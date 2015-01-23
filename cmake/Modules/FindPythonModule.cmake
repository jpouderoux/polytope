# Find if a Python module is installed
# Found at http://www.cmake.org/pipermail/cmake/2011-January/041666.html
# To use do: find_python_module(PyQt4 REQUIRED)
# Dave found this on github at https://github.com/ivansafrin/Polycode/blob/master/CMake/FindPythonModule.cmake
# JMO modified to check PYTHON_VERSION and do the right thing for python 2.x.
include(${CMAKE_CURRENT_LIST_DIR}/FindPackageHandleStandardArgs.cmake)
function(find_python_module module)
    string(TOUPPER ${module} module_upper)
    if(NOT PY_${module_upper})
        if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
            set(${module}_FIND_REQUIRED TRUE)
        endif()
        # A module's location is usually a directory, but for binary modules
        # it's a .so file.
        if (${PYTHON_VERSION} VERSION_LESS "3.0")
            set (CMDSTRING "import ${module}; print ${module}.__file__.replace('__init__.pyc', '')")
        else()
            set (CMDSTRING "import re, ${module}; print re.compile('/__init__.py.*').sub('',${module}.__file__)")
        endif()
        execute_process(COMMAND ${PYTHON_EXE} "-c" "${CMDSTRING}"
                        RESULT_VARIABLE _${module}_status 
                        OUTPUT_VARIABLE _${module}_location
                        ERROR_QUIET 
                        OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (${_${module}_status} EQUAL 0)
            set(PY_${module_upper} ${_${module}_location} CACHE STRING 
                    "Location of Python module ${module}")
        else()
            set(PY_${module_upper} "0")
        endif()
    endif(NOT PY_${module_upper})
    find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)