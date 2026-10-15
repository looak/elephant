#include <core/build_info.hpp>

#include <elephant_gambit_build.h>
#include <elephant_gambit_config.h>

#include <format>

namespace build_info {

const std::string& commit()
{
    static const std::string value = ELEPHANT_GAMBIT_GIT_COMMIT;
    return value;
}

std::string shortCommit()
{
    if (commit().empty())
        return "nogit";
    return commit().substr(0, 7) + (ELEPHANT_GAMBIT_GIT_DIRTY ? "-dirty" : "");
}

std::vector<std::pair<std::string, std::string>> entries()
{
#if defined(__clang__)
    const std::string compiler = std::format("clang {}.{}.{}", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(_MSC_VER)
    const std::string compiler = std::format("msvc {}", _MSC_FULL_VER);
#elif defined(__GNUC__)
    const std::string compiler = std::format("gcc {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
    const std::string compiler = "unknown";
#endif
#if defined(ASSERTIONS_ENABLED)
    const char* assertions = "true";
#else
    const char* assertions = "false";
#endif
#if defined(EXCEPTIONS_ENABLED)
    const char* exceptions = "true";
#else
    const char* exceptions = "false";
#endif

    return {
        { "version", ELEPHANT_GAMBIT_VERSION_FULL },
        { "commit", commit() },
        { "dirty", ELEPHANT_GAMBIT_GIT_DIRTY ? "true" : "false" },
        { "branch", ELEPHANT_GAMBIT_GIT_BRANCH },
        { "compiler", compiler },
        { "config", ELEPHANT_GAMBIT_BUILD_CONFIG },
        { "assertions", assertions },
        { "exceptions", exceptions },
        { "bits", std::to_string(sizeof(void*) * 8) },
    };
}

}  // namespace build_info
