#include <google/protobuf/message.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/json_util.h>

#include <iostream>
#include <utility>
#include <vector>
#include <cstdint>
#include <memory>

#include "test.pb.h"

struct my_resolver : google::protobuf::util::TypeResolver
{
    auto ResolveMessageType(const std::string &type_url,
                            google::protobuf::Type *message_type)
    -> google::protobuf::util::Status
    override
    {
        std::cout << __func__ << "("<< type_url <<")" << std::endl;
        std::cout << "message type currently contains: " << message_type->DebugString() << std::endl;
        
        auto descriptor = test::Foo::descriptor();
        auto syntax = descriptor->file()->syntax();

        for (int f = 0 , limit = descriptor->field_count() ;
             f < limit ; ++f)
        {
            auto field = message_type->add_fields();

            auto desc = descriptor->field(f);
            std::cout << desc->DebugString();
            field->set_name(desc->name());
            field->set_json_name(desc->json_name());
            for (auto& desc_opt : field->options())
            {
                auto fopt = field->add_options();
                fopt->set_name(desc_opt.name());
                auto value = std::make_unique<google::protobuf::Any>();
                value->CopyFrom(desc_opt.value());
                fopt->set_allocated_value(value.release());
            }
            switch(desc->type())
            {
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                    field->set_kind(::google::protobuf::Field_Kind_TYPE_INT32);
                    field->set_default_value(std::to_string(desc->default_value_int32()));
                    field->set_type_url("int32");
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_STRING:
                    field->set_kind(::google::protobuf::Field_Kind_TYPE_STRING);
                    field->set_default_value(desc->default_value_string());
                    field->set_type_url("string");
                    break;
                default:
                    return { google::protobuf::util::error::NOT_FOUND, desc->full_name() };
            }
            field->set_number(desc->number());
            field->set_packed(desc->is_packed());
            if (desc->is_optional()) {
                field->set_cardinality(::google::protobuf::Field_Cardinality_CARDINALITY_OPTIONAL);
            }
            else if (desc->is_repeated()) {
                field->set_cardinality(::google::protobuf::Field_Cardinality_CARDINALITY_REPEATED);
            }
            else {
                field->set_cardinality(::google::protobuf::Field_Cardinality_CARDINALITY_REQUIRED);
            }
        }
        
        

        return { google::protobuf::util::error::OK, "write me" };
    }
    
    auto ResolveEnumType(const std::string& type_url,
                         google::protobuf::Enum* enum_type)
    -> google::protobuf::util::Status
    override
    {
        
        return { google::protobuf::util::error::UNIMPLEMENTED, "write me" };
    }

};

int main()
{
    test::Foo foo;
    foo.set_bar(6);
    foo.set_baz("bongo");
    
    std::cout << foo.ShortDebugString() << std::endl;
    
    auto buffer = foo.SerializeAsString();
    
    google::protobuf::io::ArrayInputStream zistream(buffer.data(), buffer.size());
    auto zostream = std::make_unique<google::protobuf::io::OstreamOutputStream>(std::addressof(std::cout));
    my_resolver resolver;
    auto type_url = std::string("--type-resolver-url");
    
    google::protobuf::util::JsonOptions opts;
    opts.always_print_primitive_fields = true;
    opts.add_whitespace = true;
    auto status = google::protobuf::util::BinaryToJsonStream(std::addressof(resolver),
                                                             type_url,
                                                             std::addressof(zistream),
                                                             zostream.get(),
                                                             opts);
    zostream.reset();
    
    
    std::cout << '\n';
    std::cout << status << std::endl;
    
    
}