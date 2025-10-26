/**
 * @file OpenixCFG.cpp
 * @brief Implementation of OpenixCFG class methods
 * @author YuzukiTsuru <gloomyghost@gloomyghost.com>
 */

#include <fstream>
#include <cctype>
#include <utility>
#include <sstream>

#include "OpenixUtils.hpp"
#include "OpenixCFG.hpp"

// Constant definitions
constexpr size_t MAX_ID_LEN = 32;
constexpr size_t MAX_LINE_LEN = 256;

// Variable class implementation
Variable::Variable(std::string name, const ValueType type)
    : name_(std::move(name)), type_(type), next_(nullptr) {
    // Initialize the variant based on type
    switch (type) {
        case ValueType::NUMBER:
            value_ = 0U;
            break;
        case ValueType::STRING:
        case ValueType::REFERENCE:
            value_ = std::string();
            break;
        case ValueType::LIST_ITEM:
            value_ = std::vector<std::shared_ptr<Variable> >();
            break;
    }
}

const std::string &Variable::getName() const {
    return name_;
}

ValueType Variable::getType() const {
    return type_;
}

void Variable::setNumber(uint32_t value) {
    type_ = ValueType::NUMBER;
    value_ = value;
}

uint32_t Variable::getNumber() const {
    if (type_ == ValueType::NUMBER) {
        return std::get<uint32_t>(value_);
    }
    return 0;
}

void Variable::setString(const std::string &value) {
    type_ = ValueType::STRING;
    value_ = value;
}

const std::string &Variable::getString() const {
    static const std::string emptyString;
    if (type_ == ValueType::STRING) {
        return std::get<std::string>(value_);
    }
    return emptyString;
}

void Variable::setReference(const std::string &value) {
    type_ = ValueType::REFERENCE;
    value_ = value;
}

const std::string &Variable::getReference() const {
    static const std::string emptyString;
    if (type_ == ValueType::REFERENCE) {
        return std::get<std::string>(value_);
    }
    return emptyString;
}

void Variable::addItem(const std::shared_ptr<Variable> &item) {
    if (type_ != ValueType::LIST_ITEM) {
        type_ = ValueType::LIST_ITEM;
        value_ = std::vector<std::shared_ptr<Variable> >();
    }
    std::get<std::vector<std::shared_ptr<Variable> > >(value_).push_back(item);
}

const std::vector<std::shared_ptr<Variable> > &Variable::getItems() const {
    static const std::vector<std::shared_ptr<Variable> > emptyVector;
    if (type_ == ValueType::LIST_ITEM) {
        return std::get<std::vector<std::shared_ptr<Variable> > >(value_);
    }
    return emptyVector;
}

void Variable::setNext(const std::shared_ptr<Variable> &next) {
    next_ = next;
}

std::shared_ptr<Variable> Variable::getNext() const {
    return next_;
}

// Group class implementation
Group::Group(std::string name)
    : name_(std::move(name)), next_(nullptr) {
}

const std::string &Group::getName() const {
    return name_;
}

void Group::addVariable(const std::shared_ptr<Variable> &var) {
    variables_.push_back(var);
}

const std::vector<std::shared_ptr<Variable> > &Group::getVariables() const {
    return variables_;
}

void Group::setNext(const std::shared_ptr<Group> &next) {
    next_ = next;
}

std::shared_ptr<Group> Group::getNext() const {
    return next_;
}

// CFGParser class implementation
OpenixCFG::OpenixCFG()
    : headGroup_(nullptr) {
}

OpenixCFG::~OpenixCFG() {
    freeAll();
}

bool OpenixCFG::loadFromFile(const fs::path &filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }
    return loadFromStream(file);
}

bool OpenixCFG::loadFromStream(std::istream &stream) {
    // Free previous resources first
    freeAll();

    std::string line;
    std::shared_ptr<Group> currentGroup = nullptr;

    while (std::getline(stream, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        skipWhitespace(line);

        // Skip comment lines
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Process group definitions
        if (line[0] == '[') {
            std::shared_ptr<Group> newGroup = parseGroup(line);
            if (!headGroup_) {
                headGroup_ = newGroup;
            } else {
                // Add to the end of the linked list
                std::shared_ptr<Group> temp = headGroup_;
                while (temp->getNext()) {
                    temp = temp->getNext();
                }
                temp->setNext(newGroup);
            }
            currentGroup = newGroup;
            groupMap_[newGroup->getName()] = newGroup;
        }
        // Process list items
        else if (line[0] == '{') {
            if (!currentGroup) {
                OpenixIMG::OpenixUtils::log("Found list item but no current group!");
                continue;
            }
            std::shared_ptr<Variable> var = parseListItem(line);
            currentGroup->addVariable(var);
        }
        // Process key-value pairs
        else if (std::isalpha(line[0])) {
            if (!currentGroup) {
                OpenixIMG::OpenixUtils::log("Found variable but no current group!");
                continue;
            }
            std::shared_ptr<Variable> var = parseKeyValue(line);
            currentGroup->addVariable(var);
            variableMap_[var->getName()] = var;
        }
        // Unknown line format
        else {
            throw std::runtime_error("Unknown line format: " + line);
        }
    }

    return headGroup_ != nullptr;
}

std::shared_ptr<Group> OpenixCFG::findGroup(const std::string &name) const {
    if (const auto it = groupMap_.find(name); it != groupMap_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Variable> OpenixCFG::findVariable(const std::string &name) const {
    if (const auto it = variableMap_.find(name); it != variableMap_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Variable> OpenixCFG::findVariable(const std::string &name, const std::string &groupName) const {
    const std::shared_ptr<Group> group = findGroup(groupName);
    if (!group) {
        return nullptr;
    }

    for (const auto &var: group->getVariables()) {
        if (var->getName() == name) {
            return var;
        }
    }

    return nullptr;
}

std::optional<uint32_t> OpenixCFG::getNumber(const std::string &name) const {
    if (const std::shared_ptr<Variable> var = findVariable(name); var && var->getType() == ValueType::NUMBER) {
        return var->getNumber();
    }
    return std::nullopt;
}

std::optional<uint32_t> OpenixCFG::getNumber(const std::string &name, const std::string &groupName) const {
    if (const std::shared_ptr<Variable> var = findVariable(name, groupName);
        var && var->getType() == ValueType::NUMBER) {
        return var->getNumber();
    }
    return std::nullopt;
}

std::optional<std::string> OpenixCFG::getString(const std::string &name) const {
    if (const std::shared_ptr<Variable> var = findVariable(name); var && var->getType() == ValueType::STRING) {
        return var->getString();
    }
    return std::nullopt;
}

std::optional<std::string> OpenixCFG::getString(const std::string &name, const std::string &groupName) const {
    if (const std::shared_ptr<Variable> var = findVariable(name, groupName);
        var && var->getType() == ValueType::STRING) {
        return var->getString();
    }
    return std::nullopt;
}

size_t OpenixCFG::countVariables(const std::string &groupName) const {
    const std::shared_ptr<Group> group = findGroup(groupName);
    if (!group) {
        return 0;
    }
    return group->getVariables().size();
}

void OpenixCFG::addGroup(const std::shared_ptr<Group> &group) {
    if (!headGroup_) {
        headGroup_ = group;
    } else {
        // Add to the end of the linked list
        std::shared_ptr<Group> temp = headGroup_;
        while (temp->getNext()) {
            temp = temp->getNext();
        }
        temp->setNext(group);
    }
    groupMap_[group->getName()] = group;
}

void OpenixCFG::freeAll() {
    headGroup_ = nullptr;
    groupMap_.clear();
    variableMap_.clear();
}

void OpenixCFG::dump() const {
    std::cout << dumpToString() << std::endl;
}

std::string OpenixCFG::dumpToString() const {
    std::stringstream ss;

    if (!headGroup_) {
        ss << "No configuration loaded." << std::endl;
        return ss.str();
    }

    std::shared_ptr<Group> group = headGroup_;
    while (group) {
        ss << "[" << group->getName() << "]" << std::endl;

        for (const auto &var: group->getVariables()) {
            if (var->getType() == ValueType::NUMBER) {
                if (group->getName() == "IMAGE_CFG") {
                    ss << var->getName() << " = 0x" << std::hex << var->getNumber() << std::dec << std::endl;
                } else {
                    ss << var->getName() << " = " << var->getNumber() << std::endl;
                }
            } else if (var->getType() == ValueType::STRING) {
                ss << var->getName() << " = \"" << var->getString() << "\"" << std::endl;
            } else if (var->getType() == ValueType::REFERENCE) {
                ss << var->getName() << " = " << var->getReference() << std::endl;
            } else if (var->getType() == ValueType::LIST_ITEM) {
                ss << var->getName() << (var->getName().empty() ? "" : "=") << "{ ";
                for (const auto &item: var->getItems()) {
                    if (item->getType() == ValueType::STRING) {
                        ss << item->getName() << " = \"" << item->getString() << "\", ";
                    } else if (item->getType() == ValueType::REFERENCE) {
                        ss << item->getName() << " = " << item->getReference() << ", ";
                    } else if (item->getType() == ValueType::NUMBER) {
                        if (group->getName() == "IMAGE_CFG") {
                            ss << item->getName() << " = 0x" << std::hex << item->getNumber() << std::dec << std::endl;
                        } else {
                            ss << item->getName() << " = " << item->getNumber() << std::endl;
                        }
                    }
                }
                ss << "}," << std::endl;
            }
        }

        ss << std::endl;
        group = group->getNext();
    }

    return ss.str();
}

// Parser helper functions implementation
void OpenixCFG::skipWhitespace(std::string &line) {
    size_t pos = 0;
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
        pos++;
    }

    // Skip comment lines
    if (pos < line.size() && line[pos] == ';') {
        line.clear();
        return;
    }

    if (pos > 0) {
        line = line.substr(pos);
    }
}

std::string OpenixCFG::parseIdentifier(std::string &line) {
    std::string result;
    size_t pos = 0;

    skipWhitespace(line);

    while (pos < line.size() && (std::isalnum(static_cast<unsigned char>(line[pos])) || line[pos] == '_'
                                 || line[0] == '.')) {
        result += line[pos];
        pos++;
    }

    if (pos < line.size()) {
        line = line.substr(pos);
    } else {
        line.clear();
    }

    return result;
}

std::string OpenixCFG::parseString(std::string &line) {
    std::string result;
    size_t pos = 0;

    skipWhitespace(line);

    if (line.empty() || (line[pos] != '"' && line[pos] != '\'')) {
        return result;
    }

    const char delim = line[pos];
    pos++;

    while (pos < line.size() && line[pos] != delim) {
        if (line[pos] == '\\' && pos + 1 < line.size()) {
            pos++;
        }
        result += line[pos];
        pos++;
    }

    if (pos < line.size() && line[pos] == delim) {
        pos++;
    }

    if (pos < line.size()) {
        line = line.substr(pos);
    } else {
        line.clear();
    }

    return result;
}

std::shared_ptr<Variable> OpenixCFG::parseExpression(std::string &line) const {
    std::string result;
    bool isString = false;
    long number = 0;

    skipWhitespace(line);

    if (line.empty()) {
        return std::make_shared<Variable>("", ValueType::STRING);
    }

    // Check if it's a number
    if (std::isdigit(static_cast<unsigned char>(line[0])) || line[0] == '-') {
        size_t endPos = 0;
        try {
            number = std::stol(line, &endPos, 0);
            if (endPos > 0) {
                line = line.substr(endPos);
                auto var = std::make_shared<Variable>("", ValueType::NUMBER);
                var->setNumber(number);
                return var;
            }
        } catch (...) {
            // Not a valid number, try to process as string
        }
    }

    // Try to parse string or variable reference
    while (!line.empty()) {
        skipWhitespace(line);

        if (line.empty()) break;

        if (line[0] == '"' || line[0] == '\'') {
            // Parse string
            result += parseString(line);
            isString = true;
        } else if (std::isalpha(static_cast<unsigned char>(line[0])) || line[0] == '_' || line[0] == '.') {
            // Parse variable reference or identifier, including path-like strings
            auto ident = parseIdentifier(line);

            if (const auto var = findVariable(ident); var && var->getType() == ValueType::STRING) {
                result += var->getString();
            } else if (var && var->getType() == ValueType::NUMBER) {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "0x%x", var->getNumber());
                result += buffer;
            } else {
                result += ident;
            }
            isString = true;
        } else {
            break;
        }

        skipWhitespace(line);
        if (line.size() >= 2 && line[0] == '.' && line[1] == '.') {
            line = line.substr(2);
        } else {
            break;
        }
    }

    // Check if is a group name reference
    if (isString && !result.empty() && result.find('\"') == std::string::npos && findGroup(result)) {
        // If true set as reference
        auto refVar = std::make_shared<Variable>("", ValueType::REFERENCE);
        refVar->setReference(result);
        return refVar;
    }

    auto var = std::make_shared<Variable>("", isString ? ValueType::STRING : ValueType::NUMBER);
    if (isString) {
        var->setString(result);
    } else {
        var->setNumber(number);
    }

    return var;
}

std::shared_ptr<Group> OpenixCFG::parseGroup(const std::string &line) {
    // Find the start position of the group name (skip '[')
    auto startPos = line.find('[');
    if (startPos == std::string::npos) {
        return nullptr;
    }

    startPos++;

    // Skip whitespace characters
    while (startPos < line.size() && std::isspace(static_cast<unsigned char>(line[startPos]))) {
        startPos++;
    }

    if (startPos >= line.size()) {
        return nullptr;
    }

    // Find the end position of the group name
    const auto endPos = line.find(']', startPos);
    if (endPos == std::string::npos) {
        return nullptr;
    }

    // Extract the group name
    auto groupName = line.substr(startPos, endPos - startPos);

    size_t firstNonSpace = 0;
    size_t lastNonSpace = groupName.length() - 1;

    // Find the first non-whitespace character
    while (firstNonSpace <= lastNonSpace && std::isspace(groupName[firstNonSpace])) {
        ++firstNonSpace;
    }

    // Find the last non-whitespace character
    while (lastNonSpace >= firstNonSpace && std::isspace(groupName[lastNonSpace])) {
        --lastNonSpace;
    }

    if (firstNonSpace <= lastNonSpace) {
        // Extract the non-whitespace part
        groupName = groupName.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
    } else {
        // If there are no non-whitespace characters, clear the string
        groupName.clear();
    }

    if (groupName.empty()) {
        return nullptr;
    }

    return std::make_shared<Group>(groupName);
}

std::shared_ptr<Variable> OpenixCFG::parseKeyValue(std::string &line) const {
    skipWhitespace(line);

    // Parse variable name
    std::string name = parseIdentifier(line);
    if (name.empty()) {
        return nullptr;
    }

    skipWhitespace(line);

    // Check for equals sign
    if (line.empty() || line[0] != '=') {
        return nullptr;
    }

    // Skip equals sign
    line = line.substr(1);

    // Parse expression
    const auto expr = parseExpression(line);
    if (!expr) {
        return nullptr;
    }

    // Create variable and set value
    auto var = std::make_shared<Variable>(name, expr->getType());

    switch (expr->getType()) {
        case ValueType::NUMBER:
            var->setNumber(expr->getNumber());
            break;
        case ValueType::REFERENCE:
            var->setReference(expr->getReference());
            break;
        case ValueType::STRING:
            var->setString(expr->getString());
            break;
        case ValueType::LIST_ITEM:
            // List items should not be returned directly by parseExpression
            break;
    }

    return var;
}

std::shared_ptr<Variable> OpenixCFG::parseListItem(std::string &line) const {
    skipWhitespace(line);

    // Check if it starts with '{'
    if (line.empty() || line[0] != '{') {
        return nullptr;
    }

    // Skip '{'
    line = line.substr(1);

    // Create list item variable
    const auto listItem = std::make_shared<Variable>("", ValueType::LIST_ITEM);

    bool foundComma = false;

    do {
        skipWhitespace(line);

        // Check if we've reached the end of the list
        if (line.empty() || line[0] == '}') {
            if (!line.empty()) {
                line = line.substr(1);
            }
            break;
        }

        // Parse sub-item
        if (const auto subItem = parseKeyValue(line)) {
            listItem->addItem(subItem);
        }

        skipWhitespace(line);

        // Check for comma
        foundComma = false;
        if (!line.empty() && line[0] == ',') {
            line = line.substr(1);
            foundComma = true;
        }

        // Check if we've reached the end of the list
        if (!line.empty() && line[0] == '}') {
            line = line.substr(1);
            break;
        }
    } while (foundComma);

    return listItem;
}

std::string OpenixCFG::resolveVariableReference(const std::string &varName) const {
    if (const auto var = findVariable(varName); var && var->getType() == ValueType::STRING) {
        return var->getString();
    }
    return varName; // If variable not found or not a string, return original name
}
