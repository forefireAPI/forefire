#ifndef FOREFIRE_FFSTATE_H
#define FOREFIRE_FFSTATE_H
#include <string>

namespace libforefire {
// Versioned fire-component checkpoints; terrain remains an external dependency.
class FFState {
public:
    static void save(const std::string& filename, const std::string& scenario = "");
    static void load(const std::string& filename, bool attach = false, const std::string& provider = "");
    static void prepareCoupling(const std::string& filename);
    static void attachEnvironment(const std::string& filename, class FireDomain& domain);
};
}
#endif
