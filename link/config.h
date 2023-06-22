#ifndef __LINKS_CONFIG_H__
#define __LINKS_CONFIG_H__

#include <exception>
#include <memory>
#include <string>
#include <boost/lexical_cast.hpp>
#include "log.h"

namespace links {

class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description = "") 
        : m_name(name) 
        , m_description(description) {
    }
    virtual ~ConfigVarBase() {}

    const std::string getName() { return m_name; }
    const std::string getDescription() { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
private:
    std::string m_name;
    std::string m_description;
};

template<class T>
class ConfigVar : public ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBase(name, description)
        , m_val(default_value) {
    }

    std::string toString() override {
        try {
            return boost::lexical_cast<std::string>(m_val);
        } catch (std::exception& e) {
            LINK_LOG_ERROR(LINK_GET_ROOT()) << "ConfigVar::toString exception" 
                << e.what() << " convert: " << typeid(m_val).name() << "to string";
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
        return boost::lexical_cast<T>(val);
        } catch (std::exception& e) {
            LINK_LOG_ERROR(LINK_GET_ROOT()) << "ConfigVar::toString exception" 
                << e.what() << " convert: string to " << typeid(m_val).name();
        }
    }

private:
    T m_val;
};
}

#endif
