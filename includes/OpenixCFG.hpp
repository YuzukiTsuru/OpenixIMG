/**
 * @file OpenixCFG.hpp
 * @brief implementation of DragonEx image config file parser
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#ifndef CFGPARSER_HPP
#define CFGPARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <iostream>
#include <variant>

namespace fs = std::filesystem;

/**
 * @enum ValueType
 * @brief Enumeration defining the possible types of values a Variable can hold
 */
enum class ValueType {
    NUMBER, /**< Numeric value type */
    STRING, /**< String value type */
    LIST_ITEM, /**< List item type containing multiple sub-items */
    REFERENCE,
};

// Forward declarations
class Variable;
class Group;

/**
 * @class Variable
 * @brief Represents a configuration variable with a name, type, and value
 *
 * The Variable class can store values of different types (number, string, or list)
 * and provides accessor methods to get and set these values.
 */
class Variable {
public:
    /**
     * @brief Constructor for Variable
     *
     * @param name The name of the variable
     * @param type The type of the variable (NUMBER, STRING, or LIST_ITEM)
     */
    Variable(std::string name, ValueType type);

    ~Variable() = default;

    /**
     * @brief Get the name of the variable
     *
     * @return The name of the variable
     */
    [[nodiscard]] const std::string &getName() const;

    /**
     * @brief Get the type of the variable
     *
     * @return The type of the variable (NUMBER, STRING, or LIST_ITEM)
     */
    [[nodiscard]] ValueType getType() const;

    /**
     * @brief Set the numeric value of the variable
     *
     * @param value The numeric value to set
     */
    void setNumber(uint32_t value);

    /**
     * @brief Get the numeric value of the variable
     *
     * @return The numeric value if the variable type is NUMBER, otherwise 0
     */
    [[nodiscard]] uint32_t getNumber() const;

    /**
     * @brief Set the string value of the variable
     *
     * @param value The string value to set
     */
    void setString(const std::string &value);

    /**
     * @brief Get the string value of the variable
     *
     * @return The string value if the variable type is STRING, otherwise an empty string
     */
    [[nodiscard]] const std::string &getString() const;

    /**
     * @brief Set the reference value of the variable
     *
     * @param value The reference value to set
     */
    void setReference(const std::string &value);

    /**
     * @brief Set the reference value of the variable
     *
     * @return The string value if the variable type is STRING, otherwise an empty string
     */
    [[nodiscard]] const std::string &getReference() const;

    /**
     * @brief Add a sub-item to a list type variable
     *
     * @param item The variable to add as a sub-item
     */
    void addItem(const std::shared_ptr<Variable> &item);

    /**
     * @brief Get all sub-items of a list type variable
     *
     * @return A vector of sub-items if the variable type is LIST_ITEM, otherwise an empty vector
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Variable> > &getItems() const;

    /**
     * @brief Set the next variable in a linked list
     *
     * @param next The next variable in the list
     */
    void setNext(const std::shared_ptr<Variable> &next);

    /**
     * @brief Get the next variable in a linked list
     *
     * @return The next variable in the list, or nullptr if none
     */
    [[nodiscard]] std::shared_ptr<Variable> getNext() const;

private:
    std::string name_; /**< The name of the variable */
    ValueType type_; /**< The type of the variable */

    /**
     * @brief The value of the variable, stored as a variant
     *
     * This variant can hold a uint32_t (for numbers), a string, or a vector of shared pointers to Variables (for lists).
     */
    std::variant<uint32_t, std::string, std::vector<std::shared_ptr<Variable> > > value_;

    std::shared_ptr<Variable> next_; /**< Pointer to the next variable in a linked list */
};

/**
 * @class Group
 * @brief Represents a group of configuration variables
 *
 * The Group class contains a collection of variables and provides methods to manage them.
 */
class Group {
public:
    /**
     * @brief Constructor for Group
     *
     * @param name The name of the group
     */
    explicit Group(std::string name);

    ~Group() = default;

    /**
     * @brief Get the name of the group
     *
     * @return The name of the group
     */
    [[nodiscard]] const std::string &getName() const;

    /**
     * @brief Add a variable to the group
     *
     * @param var The variable to add
     */
    void addVariable(const std::shared_ptr<Variable> &var);

    /**
     * @brief Get all variables in the group
     *
     * @return A vector of all variables in the group
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Variable> > &getVariables() const;

    /**
     * @brief Set the next group in a linked list
     *
     * @param next The next group in the list
     */
    void setNext(const std::shared_ptr<Group> &next);

    /**
     * @brief Get the next group in a linked list
     *
     * @return The next group in the list, or nullptr if none
     */
    [[nodiscard]] std::shared_ptr<Group> getNext() const;

private:
    std::string name_; /**< The name of the group */
    std::vector<std::shared_ptr<Variable> > variables_; /**< The variables in the group */
    std::shared_ptr<Group> next_; /**< Pointer to the next group in a linked list */
};

/**
 * @class OpenixCFG
 * @brief Main parser class for DragonEx image configuration files
 *
 * The CFGParser class provides methods to load, parse, and access configuration data
 * from DragonEx image configuration files.
 */
class OpenixCFG {
public:
    /**
     * @brief Constructor for CFGParser
     */
    OpenixCFG();

    ~OpenixCFG();

    /**
     * @brief Load configuration from a file
     *
     * @param filepath The path to the configuration file
     * @return true if the file was successfully loaded and parsed, false otherwise
     */
    bool loadFromFile(const fs::path &filepath);

    /**
     * @brief Load configuration from an input stream
     *
     * @param stream The input stream containing the configuration data
     * @return true if the stream was successfully parsed, false otherwise
     */
    bool loadFromStream(std::istream &stream);

    /**
     * @brief Find a group by name
     *
     * @param name The name of the group to find
     * @return A shared pointer to the group if found, nullptr otherwise
     */
    std::shared_ptr<Group> findGroup(const std::string &name) const;

    /**
     * @brief Find a variable by name (searches all groups)
     *
     * @param name The name of the variable to find
     * @return A shared pointer to the variable if found, nullptr otherwise
     */
    std::shared_ptr<Variable> findVariable(const std::string &name) const;

    /**
     * @brief Find a variable by name within a specific group
     *
     * @param name The name of the variable to find
     * @param groupName The name of the group to search in
     * @return A shared pointer to the variable if found, nullptr otherwise
     */
    std::shared_ptr<Variable> findVariable(const std::string &name, const std::string &groupName) const;

    /**
     * @brief Get a numeric value by variable name (searches all groups)
     *
     * @param name The name of the variable to get
     * @return An optional containing the numeric value if found and is a number, std::nullopt otherwise
     */
    std::optional<uint32_t> getNumber(const std::string &name) const;

    /**
     * @brief Get a numeric value by variable name within a specific group
     *
     * @param name The name of the variable to get
     * @param groupName The name of the group to search in
     * @return An optional containing the numeric value if found and is a number, std::nullopt otherwise
     */
    std::optional<uint32_t> getNumber(const std::string &name, const std::string &groupName) const;

    /**
     * @brief Get a string value by variable name (searches all groups)
     *
     * @param name The name of the variable to get
     * @return An optional containing the string value if found and is a string, std::nullopt otherwise
     */
    std::optional<std::string> getString(const std::string &name) const;

    /**
     * @brief Get a string value by variable name within a specific group
     *
     * @param name The name of the variable to get
     * @param groupName The name of the group to search in
     * @return An optional containing the string value if found and is a string, std::nullopt otherwise
     */
    std::optional<std::string> getString(const std::string &name, const std::string &groupName) const;

    /**
     * @brief Count the number of variables in a specific group
     *
     * @param groupName The name of the group to count variables in
     * @return The number of variables in the group, or 0 if the group is not found
     */
    size_t countVariables(const std::string &groupName) const;

    /**
     * @brief Add a group to the configuration
     * @param group The group to add
     */
    void addGroup(const std::shared_ptr<Group> &group);

    /**
     * @brief Free all resources used by the parser
     */
    void freeAll();

    /**
     * @brief Print the entire configuration (for debugging purposes)
     */
    void dump() const;

    /**
     * @brief Dump the entire configuration to a string
     * @return A string containing the entire configuration
     */
    std::string dumpToString() const;

private:
    std::shared_ptr<Group> headGroup_; /**< The first group in the linked list of groups */
    std::unordered_map<std::string, std::shared_ptr<Group> > groupMap_;
    /**< Map of group names to groups for quick lookup */
    std::unordered_map<std::string, std::shared_ptr<Variable> > variableMap_;
    /**< Map of variable names to variables for quick lookup */

    /**
     * @brief Parse a group definition from a line of text
     *
     * @param line The line of text containing the group definition
     * @return A shared pointer to the parsed group, or nullptr if parsing failed
     */
    static std::shared_ptr<Group> parseGroup(const std::string &line);

    /**
     * @brief Parse a key-value pair from a line of text
     *
     * @param line The line of text containing the key-value pair
     * @return A shared pointer to the parsed variable, or nullptr if parsing failed
     */
    std::shared_ptr<Variable> parseKeyValue(std::string &line) const;

    /**
     * @brief Parse a list item from a line of text
     *
     * @param line The line of text containing the list item
     * @return A shared pointer to the parsed list variable, or nullptr if parsing failed
     */
    std::shared_ptr<Variable> parseListItem(std::string &line) const;

    /**
     * @brief Parse an expression (number, string, or variable reference) from a line of text
     *
     * @param line The line of text containing the expression
     * @return A shared pointer to a variable containing the parsed value
     */
    std::shared_ptr<Variable> parseExpression(std::string &line) const;

    /**
     * @brief Parse an identifier from a line of text
     *
     * @param line The line of text containing the identifier
     * @return The parsed identifier
     */
    static std::string parseIdentifier(std::string &line);

    /**
     * @brief Parse a string from a line of text
     *
     * @param line The line of text containing the string
     * @return The parsed string
     */
    static std::string parseString(std::string &line);

    /**
     * @brief Skip whitespace and comments in a line of text
     *
     * @param line The line of text to process
     */
    static void skipWhitespace(std::string &line);

    /**
     * @brief Resolve a variable reference to its string value
     *
     * @param varName The name of the variable to resolve
     * @return The string value of the variable if found and is a string, otherwise the original name
     */
    std::string resolveVariableReference(const std::string &varName) const;
};

#endif // CFGPARSER_HPP
