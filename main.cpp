#include <google/protobuf/message.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/json_util.h>

#include <iostream>
#include <utility>
#include <vector>
#include <cstdint>
#include <memory>
#include <sstream>
#include <algorithm>

#include "test.pb.h"
#include "my_resolver.hpp"


test::Foo* populate(test::Foo* foo, int bar, std::string baz)
{
    foo->set_bar(bar);
    foo->set_baz(baz);
    return foo;
}

void populate(test::Exception& emsg, const std::exception& e)
{
    emsg.set_what(e.what());
    emsg.set_name(typeid(e).name());
    try {
        std::rethrow_if_nested(e);
    }
    catch(std::exception& e)
    {
        auto pnext = emsg.mutable_nested();
        populate(*pnext, e);
    }
    catch(...) {
        auto pnext = emsg.mutable_nested();
        pnext->set_what("unknown error");
        pnext->set_name("unknown");
    }
}

void as_json(std::ostream& os, const google::protobuf::Message* msg)
{
    auto buffer = msg->SerializeAsString();
    google::protobuf::io::ArrayInputStream zistream(buffer.data(), buffer.size());
    google::protobuf::io::OstreamOutputStream zostream(std::addressof(os));
    my_resolver resolver;
    
    google::protobuf::util::JsonOptions opts;
    opts.always_print_primitive_fields = true;
    opts.add_whitespace = true;
    auto status = google::protobuf::util::BinaryToJsonStream(std::addressof(resolver),
                                                             resolver.add_prefix(msg->GetDescriptor()->full_name()),
                                                             std::addressof(zistream),
                                                             std::addressof(zostream),
                                                             opts);
    if (!status.ok())
    {
        std::ostringstream ss;
        ss << status;
        throw std::runtime_error(ss.str());
    }
}

std::string as_json(const google::protobuf::Message& msg)
{
    auto buffer = msg.SerializeAsString();
    std::string result;
    google::protobuf::io::ArrayInputStream zistream(buffer.data(), buffer.size());
    my_resolver resolver;
    
    google::protobuf::util::JsonOptions opts;
    opts.always_print_primitive_fields = true;
    opts.add_whitespace = true;
    auto status = google::protobuf::util::BinaryToJsonString(std::addressof(resolver),
                                                             resolver.add_prefix(msg.GetDescriptor()->full_name()),
                                                             buffer,
                                                             std::addressof(result),
                                                             opts);
    if (!status.ok())
    {
        std::ostringstream ss;
        ss << status;
        throw std::runtime_error(ss.str());
    }
    return result;
}

template<class MessageType>
std::unique_ptr<MessageType> from_json(const std::string& json)
{
    std::string buffer;
    
    
    google::protobuf::io::StringOutputStream zostream(&buffer);
    my_resolver resolver;
    
    auto status = google::protobuf::util::JsonToBinaryString(std::addressof(resolver),
                                                             resolver.add_prefix("test.Foo"),
                                                             json,
                                                             &buffer);
    if (!status.ok())
    {
        std::ostringstream ss;
        ss << status;
        throw std::runtime_error(ss.str());
    }
    
    auto result = std::make_unique<MessageType>();
    result->ParseFromString(buffer);
    return result;
}

void from_json(google::protobuf::Message& dest_msg, const std::string& json)
{
    std::string buffer;
    google::protobuf::io::StringOutputStream zostream(&buffer);
    my_resolver resolver;
    
    auto status = google::protobuf::util::JsonToBinaryString(std::addressof(resolver),
                                                             resolver.add_prefix("test.Foo"),
                                                             json,
                                                             &buffer);
    if (!status.ok())
    {
        std::ostringstream ss;
        ss << status;
        throw std::runtime_error(ss.str());
    }
    if (!dest_msg.ParseFromString(buffer))
    {
        throw std::logic_error("incomplete json for message: "
                               + dest_msg.GetDescriptor()->full_name()
                               + ", json=" + json);
    }
}

/*
 
 POST: se.cr/api/api.add_user
 HEADER: Content-Type: application/json;charset=utf-8
 HEADER: secr-command: cr.se.api.add_user
 BODY:
 {
 "bar": 6,
 "baz": "bongo",
 "nextFoo": {
 "bar": 7,
 "baz": "chicken",
 "values": [],
 "bars": [],
 "anotherBar": {
 "aaa": "400"
 }
 },
 "values": [
 1000,
 2000,
 3000
 ],
 "bars": [
 {
 "aaa": "100"
 },
 {
 "aaa": "106"
 }
 ],
 "anotherFoo": {
 "bar": 8,
 "baz": "far out"
 }
 }
 
 response:
 
 
 */

struct my_test_service : test::test_service
{
    void foo_mee(::google::protobuf::RpcController *controller,
                 const ::test::Foo *request,
                 ::test::Bar *response,
                 ::google::protobuf::Closure *done) override
    {
        std::cout << "request message is: " << request->ShortDebugString() << std::endl;
        response->set_aaa(10);
        done->Run();
    }
    
    void foo_failure(::google::protobuf::RpcController *controller,
                     const ::test::Foo *request,
                     ::test::Bar *response,
                     ::google::protobuf::Closure *done) override
    {
        try
        {
            std::cout << "request message is: " << request->ShortDebugString() << std::endl;
            
            try {
                throw std::runtime_error("I always throw");
            }
            catch(...)
            {
                std::throw_with_nested(std::runtime_error("I always throw too"));
            }
            response->set_aaa(100);
        }
        catch(std::exception& e)
        {
            test::Exception emsg;
            populate(emsg, e);
            std::string buffer;
            emsg.SerializeToString(std::addressof(buffer));
            controller->SetFailed(buffer);
        }
        done->Run();
    }
    
};

struct service_generator
{
    struct concept
    {
        virtual std::unique_ptr<google::protobuf::Service> generate_service() = 0;
        virtual ~concept() = default;
    };
    
    template<class ServiceImpl>
    struct model : concept
    {
        std::unique_ptr<google::protobuf::Service> generate_service() override
        {
            return std::make_unique<ServiceImpl>();
        }
    };
    
    template<class ServiceImpl>
    static service_generator create()
    {
        return service_generator {
            ServiceImpl::descriptor(),
            std::make_unique<model<ServiceImpl>>()
        };
    }
    
    const google::protobuf::ServiceDescriptor& descriptor() const {
        return *_descriptor;
    }
    
    std::unique_ptr<google::protobuf::Service> generate_service() const
    {
        return _concept->generate_service();
    }
    
private:
    using concept_ptr = std::unique_ptr<concept>;
    
    service_generator(const google::protobuf::ServiceDescriptor* descriptor,
                      concept_ptr ptr)
    : _descriptor(descriptor)
    , _concept(std::move(ptr))
    {
        
    }
private:
    const google::protobuf::ServiceDescriptor* _descriptor;
    std::unique_ptr<concept> _concept;
};


struct service_factory
{
    template<class ServiceImpl>
    bool register_service()
    {
        return _generators.emplace(ServiceImpl::descriptor()->full_name(),
                                   service_generator::create<ServiceImpl>()).second;
    }
    
    
    const service_generator& locate(const std::string& service_name) const {
        
        auto i = _generators.find(service_name);
        if (i == std::end(_generators)) {
            throw std::logic_error("unknown service: " + service_name);
        }
        return i->second;
    }
    
    
    std::map<std::string, service_generator> _generators;
};


struct simple_rpc_controller : google::protobuf::RpcController
{
    void Reset() override {
        _failed = false;
        _cancelled = false;
        _cancel_closure.reset();
    }
    
    bool Failed() const override {
        return _failed;
    }

    std::string ErrorText() const override
    {
        return _error_message;
    }

    void StartCancel() override
    {
        if (_cancel_closure) {
            _cancelled = true;
            _cancel_closure->Run();
        }
    }
    
    void SetFailed(const std::string& errtext) override
    {
        _error_message = errtext;
        _failed = true;
    }

    bool IsCanceled() const override
    {
        return _cancelled;
    }

    void NotifyOnCancel(google::protobuf::Closure* closure) override
    {
        _cancel_closure.reset(closure);
    }

private:
    bool _failed = false;
    std::string _error_message;
    
    bool _cancelled = false;
    std::unique_ptr<google::protobuf::Closure> _cancel_closure;
};

struct sync_call_closure : google::protobuf::Closure
{
    void Run() override {}
};

std::pair<std::string, std::string>
execute_json(const service_factory& factory,
             const std::string service_name,
             const std::string method_name,
             const std::string request_json)
{
    auto& generator = factory.locate(service_name);
    auto service = generator.generate_service();
    auto method = service->GetDescriptor()->FindMethodByName(method_name);
    if (!method) {
        throw std::runtime_error ("method not found: " + service_name + ":" + method_name);
    }
    
    auto arena_ptr = std::make_shared<google::protobuf::Arena>();
    
    
    auto& request_proto = service->GetRequestPrototype(method);
    auto request_ptr = std::shared_ptr<google::protobuf::Message>(arena_ptr, request_proto.New(arena_ptr.get()));
    from_json(*request_ptr, request_json);
    
    auto& response_proto = service->GetResponsePrototype(method);
    auto response_ptr = std::shared_ptr<google::protobuf::Message>(arena_ptr,
                                                                   response_proto.New(arena_ptr.get()));
    
    simple_rpc_controller my_controller;
    sync_call_closure closure;
    
    service->CallMethod(method,
                        std::addressof(my_controller),
                        request_ptr.get(),
                        response_ptr.get(),
                        std::addressof(closure));
    
    if (my_controller.Failed())
    {
        auto error_msg = std::shared_ptr<test::Exception>(arena_ptr,
                                                          google::protobuf::Arena::CreateMessage<test::Exception>(arena_ptr.get()));
        error_msg->ParseFromString(my_controller.ErrorText());
        response_ptr = error_msg;
    }
    
    
    
    
    
    return std::make_pair(response_ptr->GetDescriptor()->full_name(), as_json(*response_ptr));
}

int main()
{
    service_factory api_services;
    api_services.register_service<my_test_service>();
    
    test::Foo foo;
    populate(std::addressof(foo), 6, "bongo");
    foo.add_bars()->set_aaa(100);
    foo.add_bars()->set_aaa(106);
    foo.add_values(1000);
    foo.add_values(2000);
    foo.add_values(3000);
    populate(foo.mutable_another_foo(), 8, "far out");
    auto mybar = populate(foo.mutable_next_foo(), 7, "chicken")->mutable_another_bar();
    mybar->set_aaa(400);
    
    std::cout << foo.ShortDebugString() << std::endl;
    as_json(std::cout, std::addressof(foo));
    
    std::stringstream ss;
    as_json(ss, std::addressof(foo));
    auto copy = from_json<test::Foo>(ss.str());
    std::cout << copy->DebugString();
    
    auto pair = execute_json(api_services, "test.test_service", "foo_mee", ss.str());
    std::cout << "response type : " << pair.first << std::endl;
    std::cout << "response payload :\n" << pair.second << std::endl;
    
    pair = execute_json(api_services, "test.test_service", "foo_failure", "{}");
    std::cout << "response type : " << pair.first << std::endl;
    std::cout << "response payload :\n" << pair.second << std::endl;
    
    
}