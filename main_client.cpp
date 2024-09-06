#include <iostream>

#include "protocol/Request.h"
#include "thirdparties/cppzmq/zmq.hpp"

int main(int argc, char* argv[]) {
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, zmq::socket_type::req);

    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:7878");

    // char buf[1024] {0};
    // // const auto requestData = reinterpret_cast<Mess::KhawasuServiceProtocol::Request *>(buf);
    // Mess::KhawasuServiceProtocol::Request::Initialize(reinterpret_cast<std::int8_t *>(buf), strlen("GetDevices"), 0);
    // auto requestData = (Mess::KhawasuServiceProtocol::Request *)(buf);
    //
    // std::strcpy(reinterpret_cast<char *>(requestData->name().values), "GetDevices");
    //
    // zmq::message_t request (requestData->get_size());
    // memcpy (request.data(), requestData, requestData->get_size());
    // std::cout << "Sending Hello " << requestData->get_size() << "" << std::string((char*)requestData->name().values, requestData->name().size) << "..." << std::endl;
    // socket.send (request, zmq::send_flags::none);

    using namespace Mess::KhawasuServiceProtocol::High;
    auto requestBuf = Writer::write(Request {
        .name = "GetDevices"
    });

    zmq::message_t request (requestBuf.size());
    memcpy (request.data(), requestBuf.data(), requestBuf.size());
    std::cout << "Sending Request " << requestBuf.size() << std::endl;
    socket.send (request, zmq::send_flags::none);

    //  Get the reply.
    zmq::message_t reply;
    socket.recv (reply, zmq::recv_flags::none);
    std::cout << "Received reply " << reply.size() << std::endl;

    auto responseObj = Reader::read<Response>(std::span((char*)reply.data(), reply.size()));
    auto responseGetDevicesObj = Reader::read<GetDevicesResponse>(std::span((char*)responseObj.data._values.data(), responseObj.data._values.size()));
    for(const auto& device : responseGetDevicesObj.devices._values) {
        auto address = device.address.far.str() + ":" + std::to_string(device.address.log);
        std::cout << "New device \"" << device.name.str() << "\" (" << address << ") " << device.type.str() << std::endl;
    }
    // auto response = (Mess::KhawasuServiceProtocol::Response*) reply.data();
    // auto responseData = (Mess::KhawasuServiceProtocol::GetDevicesResponse*)(request.data());
    //
    // for(auto i = 0; i < responseData->devices().size; ++i) {
    //     auto& device = responseData->devices()[i];
    //     auto name = std::string((char*)device.name().values, device.name().size);
    //     auto address = std::string((char*)device.address().far().values, device.address().far().size) + \
    //                         ":" + std::to_string(device.address().log());
    //
    //     auto type = std::string((char*)device.type().values, device.type().size);
    //
    //     std::cout << "New device \"" << name << "\" (" << address << ") " << type << std::endl;
    // }
    return 0;
}
