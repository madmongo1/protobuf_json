#pragma once

#include <google/protobuf/util/type_resolver.h>


struct my_resolver : google::protobuf::util::TypeResolver
{
    
    
    auto ResolveMessageType(const std::string &type_url,
                            google::protobuf::Type *message_type)
    -> google::protobuf::util::Status
    override;
    
    auto ResolveEnumType(const std::string& type_url,
                         google::protobuf::Enum* enum_type)
    -> google::protobuf::util::Status
    override;
    
    static const std::string& prefix()
    {
        static const std::string _prefix = "test";
        return _prefix;
    }
    
    std::string add_prefix(std::string s)
    {
        s.insert(0, prefix() + '/');
        return s;
    }
    
private:
    static google::protobuf::util::TypeResolver& get_impl();
    
};

