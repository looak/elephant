# Fetched sources live in .build/deps, one copy per OS and tag, shared by every build tree so a new or cleaned tree
# doesn't download them again. What's compiled from them stays in each tree, its flags are its own. Not shared between
# operating systems: a git clone made on one is refused by git on the other when the same folder is seen from WSL.
include(FetchContent)

set(ELEPHANT_DEPS_DIR "${CMAKE_SOURCE_DIR}/.build/deps/${CMAKE_SYSTEM_NAME}")

# FetchContent_Declare with the source folder chosen, the rest of the arguments are passed on.
function(elephant_declare_dep name repository tag)
    string(TOUPPER "${name}" upper)
    set(source "${ELEPHANT_DEPS_DIR}/${name}-${tag}")
    # already there: FetchContent uses it as it is, without going to the network. A copy from an interrupted download is
    # cleared with ./build.sh --clean --deps.
    if(EXISTS "${source}/.git")
        set(FETCHCONTENT_SOURCE_DIR_${upper} "${source}" CACHE INTERNAL "")
    endif()
    FetchContent_Declare(${name}
        GIT_REPOSITORY ${repository}
        GIT_TAG        ${tag}
        SOURCE_DIR     ${source}
        ${ARGN})
endfunction()
