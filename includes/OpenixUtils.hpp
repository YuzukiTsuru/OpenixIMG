#ifndef OPENIX_UTILS_HPP
#define OPENIX_UTILS_HPP

#include <string>

namespace OpenixIMG {
    /**
     * @brief Utility class for common functions
     */
    class OpenixUtils {
    private:
        static bool verboseEnabled_; //!< Global verbose mode flag

    public:
        /**
         * @brief Set the global verbose mode
         * @param enabled Whether to enable verbose output
         */
        static void setVerboseEnabled(bool enabled);

        /**
         * @brief Check if verbose mode is enabled
         * @return True if verbose is enabled
         */
        static bool isVerboseEnabled();

        /**
         * @brief Log a message if verbose is enabled
         * @param message The message to log
         */
        static void log(const std::string &message);
    };
} // namespace OpenixIMG

#endif // OPENIX_UTILS_HPP
