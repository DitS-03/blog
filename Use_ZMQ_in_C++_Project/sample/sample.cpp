#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, zmq::socket_type::pub);
    sock.bind("tcp://*:5556");
    while (true) {
        std::cout << "send message..." << std::endl;
        sock.send(zmq::str_buffer("Hello, world"));
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}