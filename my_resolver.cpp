#include "my_resolver.hpp"
#include <google/protobuf/util/type_resolver_util.h>

#include <iostream>

namespace {
    
    template<class...Args>
    void emit(Args&&...args)
    {
        using expand = int[];
        void(expand {
            0,
            ((std::cout << args), 0)...
        });
    }
}

namespace pb = google::protobuf;
namespace util = pb::util;

auto
my_resolver::get_impl()
-> google::protobuf::util::TypeResolver&
{
    static std::unique_ptr<google::protobuf::util::TypeResolver> _resolver
    {
        util::NewTypeResolverForDescriptorPool(prefix(),
                                               pb::DescriptorPool::generated_pool())
    };
    return *_resolver;;
}


auto
my_resolver::ResolveMessageType(const std::string &type_url,
                                google::protobuf::Type *message_type)
-> google::protobuf::util::Status
{
    return get_impl().ResolveMessageType(type_url, message_type);
}

auto
my_resolver::ResolveEnumType(const std::string& type_url,
                     google::protobuf::Enum* enum_type)
-> google::protobuf::util::Status
{
    return get_impl().ResolveEnumType(type_url, enum_type);
}
