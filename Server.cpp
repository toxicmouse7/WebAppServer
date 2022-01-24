//
// Created by Алексей Гладков on 20.01.2022.
//
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <cerrno>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>


#include "Server.hpp"

Server::Server(const std::string& ip_address, int port)
{
    this->sock_d = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address_info{};
    address_info.sin_family = AF_INET;
    address_info.sin_port = htons(port);
    inet_aton(ip_address.c_str(), reinterpret_cast<in_addr*>(&(address_info.sin_addr.s_addr)));

    int bind_result = bind(this->sock_d,
                           reinterpret_cast<const sockaddr*>(&address_info),
                           sizeof(address_info));
    if (bind_result == -1)
    {
        throw std::runtime_error(strerror(errno));
    }
}

[[noreturn]] void Server::run()
{
    if (listen(this->sock_d, 5) == -1)
    {
        throw std::runtime_error(strerror(errno));
    }

    while (true)
    {
        int client_sock_d = accept(this->sock_d, nullptr, nullptr);
        auto[client_iter, is_inserted] = this->clients.
                insert(ClientRepr(client_sock_d));
        this->run_user_thread(*client_iter);
    }

}

void Server::run_user_thread(const ClientRepr& repr) const
{
    std::thread user_thread(&Server::serve_user,
                            const_cast<Server*>(this),
                            std::ref(const_cast<ClientRepr&>(repr)));
    user_thread.detach();
}

void Server::serve_user(ClientRepr& repr)
{
    std::string string_message;
    send_all("Username: ", repr);
    try
    {
        repr.username = receive_all(repr);
    }
    catch (std::runtime_error& error)
    {
        std::cout << error.what() << std::endl;
        this->clients.erase(repr);
    }

    while (true)
    {
        try
        {
            auto message = receive_all(repr);

            if (message.find("send") == 0)
            {
                auto data = message.substr(5);
                send_all("Please, provide a name for file!", repr);
                auto filename = receive_all(repr);
                save_file(filename, data);
                send_all("Done!", repr);
            }
        }
        catch (std::runtime_error& error)
        {
            std::cout << error.what() << std::endl;
            break;
        }
    }

    this->clients.erase(repr);
}

void Server::send_all(const std::string& message, const ClientRepr& repr) const
{
    char data[4096] = {0};
    size_t to_send = message.size() + sizeof(ulong);

    std::cout << "Chunks in message:" << message.size() / 4096 + 1 << std::endl;

    reinterpret_cast<ulong*>(data)[ 0 ] = static_cast<ulong>(to_send);


    memcpy(reinterpret_cast<char*>(data + sizeof(ulong)),
           message.data(),
           std::min(sizeof(data) - sizeof(ulong), to_send));

    to_send -= send_chunk(repr.sock_d, data, std::min(to_send, sizeof(data)));

    int count = 1;
    while (to_send > 0)
    {
        ulong length = (ulong)std::min(to_send, sizeof(data));

        memcpy(reinterpret_cast<char*>(data),
               (message.data() + sizeof(data) * count - sizeof(ulong)),
               length);
        ++count;

        to_send -= send_chunk(repr.sock_d, data, std::min(to_send, sizeof(data)));
    }
}

long Server::send_chunk(int sock, char* buf, size_t length) const
{
    ssize_t total = 0;
    ssize_t sent = 0;
    while (total < length)
    {
        sent = send(sock, buf + total, length - total, 0);
        if (sent == -1)
        {
            throw std::runtime_error(strerror(errno));
        }
        total += sent;
    }

    return total;
}

std::string Server::receive_all(const ClientRepr& repr)
{
    std::string result;
    char data[4096] = {0};

    auto size_chunk = receive_chunk(repr.sock_d, sizeof(ulong));
    for (int i = 0; i < sizeof(ulong); ++i)
        data[ i ] = size_chunk[ i ];

    if (reinterpret_cast<ulong*>(data)[ 0 ] > sizeof(data) - sizeof(ulong))
    {
        auto to_receive = reinterpret_cast<ulong*>(data)[ 0 ] - sizeof(ulong);

        while (to_receive > 0)
        {
            auto chunk = std::string();
            if (to_receive < sizeof(data) - sizeof(ulong))
                chunk = receive_chunk(repr.sock_d, to_receive);
            else
                chunk = receive_chunk(repr.sock_d, sizeof(data));

            to_receive -= chunk.size();
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
    }
    else
    {
        auto chunk = receive_chunk(repr.sock_d,
                                   reinterpret_cast<ulong*>(data)[ 0 ] - sizeof(ulong));
        result.insert(result.end(), chunk.begin(), chunk.end());
    }

    return result;
}

std::string Server::receive_chunk(int sock, ulong to_receive)
{
    std::string result;
    char buf[4096] = {0};
    long total = 0;

    while (total != to_receive)
    {
        auto received = recv(sock, buf + total, to_receive - total, 0);
        if (received == -1)
            throw std::runtime_error(strerror(errno));
        total += received;
    }

    for (int i = 0; i < to_receive; ++i)
    {
        result.push_back(buf[i]);
    }

    return result;
}

void Server::save_file(const std::string& filename, const std::string& data)
{
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Write error!");

    std::stringstream s(data);
    s >> file.rdbuf();

    file.flush();

    file.close();

    system(("md5 " + filename).c_str());
}
