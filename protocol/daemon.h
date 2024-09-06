#pragma once
#include <iostream>
#include <span>
#include <protocols/logical_proto.h>

#include "Request.h"

using namespace LogicalProto;
namespace Message = Mess::KhawasuServiceProtocol::High;

enum class RequestType : int {
    Unknown,
    GetDevices,
    ExecuteAction,
    FetchAction
};


class DaemonProtocol {
public:

    static std::string requestTypeToString(RequestType type) {
#define CASE(type) case RequestType::type: return #type
        switch (type) {
            CASE(GetDevices);
            CASE(ExecuteAction);
            CASE(FetchAction);
            default:
                assert(false && "Unknown RequestType");
                return "UNKNOWN";
        }
    }

    static RequestType stringToRequestType(const std::string& stype) {
#define IFCASE(type) if (strcmp(stype.c_str(), #type) == 0) { return RequestType::type; }

        IFCASE(GetDevices);
        IFCASE(ExecuteAction);
        IFCASE(FetchAction);
        assert(false && "Unknown RequestType");
        return RequestType::Unknown;
    }

    static std::vector<char> ProcessRequest(std::span<char> request_data) {
        auto request_obj = Message::Reader::read<Message::Request>(request_data);
        auto request_name = std::string((char*)request_obj.name._values.data(), request_obj.name._values.size());
        std::cout << "Received request " << request_name <<  std::endl;

        std::vector<char> response_data;

        switch (stringToRequestType(request_name)) {
            case RequestType::GetDevices:
                response_data = Message::Writer::write(Message::GetDevicesResponse {
                    .devices = {
                        {
                            Message::Device {
                                .address = {.log = 2222, .far = "2B 49 7C AA"},
                                .type = KhawasuReflection::getAllDeviceClasses()[DeviceClassEnum::BUTTON],
                                .name = "My Awesome Button",
                            },
                            Message::Device {
                                .address = {.log = 2222, .far = "2B 49 7C AA"},
                                .type = KhawasuReflection::getAllDeviceClasses()[DeviceClassEnum::BUTTON],
                                .name = "My Awesome Button",
                            },
                        }
                    }
                });
                break;
            case RequestType::ExecuteAction:
                break;
            case RequestType::FetchAction:
                break;
            default:
            case RequestType::Unknown:
                break;
        }

        auto response_obj = Message::Response();
        response_obj.data = std::vector<std::int8_t>{response_data.begin(), response_data.end()};

        return Message::Writer::write(response_obj);
    }
};
