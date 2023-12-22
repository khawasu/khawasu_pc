#include "khawasu_app.h"
#include "interfaces/mesh_socket_interface.h"

#include <utility>

#if(__unix__)
#include "platform/p2p/unix_p2p.h"
#elif(WIN32)
#include "platform/p2p/win32_p2p.h"
#endif

KhawasuApp::KhawasuApp(std::string freshNetworkName, MeshProto::far_addr_t freshNetworkAddr, std::string freshNetworkPsk)
                     :  fresh_network_name(std::move(freshNetworkName)), fresh_network_addr(freshNetworkAddr),
                        fresh_network_psk(std::move(freshNetworkPsk)) {

    std::cout << ":: Fresh network :: \"" << fresh_network_name << "\" by PSK with address = " << fresh_network_addr << std::endl;

    controller = new MeshController(fresh_network_name.c_str(), fresh_network_addr);
    g_fresh_mesh = controller;

    controller->set_psk_password(fresh_network_psk.c_str());

    controller->user_stream_handler = [this](MeshProto::far_addr_t src_addr, const ubyte* data, ushort size) {
        mesh_packet_handler(src_addr, data, size, &manager);
    };

    OverlayPacketBuilder::log_ovl_packet_alloc =
            new PoolMemoryAllocator<LOG_PACKET_POOL_ALLOC_PART_SIZE, LOG_PACKET_POOL_ALLOC_COUNT>();
}

void KhawasuApp::register_fresh_com_device(std::string& path, int boudrate) {
    // todo: add unixserial
    #if(WIN32)
    Win32Serial serial(path.c_str(), boudrate);
    #elif(__unix__)
    UnixSerial serial(path.c_str(), boudrate);
    #endif
    interfaces.push_back(new P2PUnsecuredShortInterface(true, false, serial, serial));
    Os::sleep_milliseconds(1000);
    controller->add_interface(interfaces.back());

    std::cout << ":: Registered Fresh COM Device: " << path << std::endl;
}

void KhawasuApp::register_fresh_socket_server(std::string& hostname, uint16_t port) {
    try {
        interfaces.push_back(new MeshSocketInterface(hostname, port, true));
        controller->add_interface(interfaces.back());

        std::cout << ":: Registered Fresh Socket Server: " << hostname << ":" << port << std::endl;
    } catch (const std::exception& e) {
        std::cerr << ":: Fresh Socket Server Error: " << e.what() << std::endl;
    }
}

void KhawasuApp::register_fresh_socket_client(std::string& hostname, uint16_t port) {
    try {
        interfaces.push_back(new MeshSocketInterface(hostname, port, false));
        controller->add_interface(interfaces.back());

        std::cout << ":: Registered Fresh Socket Client: " << hostname << ":" << port << std::endl;
    } catch (const MeshSocketInterfaceConnectionException& e) {
        std::cerr << ":: Fresh Socket Server Connection error" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << ":: Fresh Socket Client Error: " << e.what() << std::endl;
    }
}

