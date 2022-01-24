//
// Created by Алексей Гладков on 20.01.2022.
//

#ifndef WEBAPP_SERVER_HPP
#define WEBAPP_SERVER_HPP

#include <set>
#include <vector>

#include "ClientRepr.hpp"

typedef unsigned long long ulong;

class Server
{
private:
    int sock_d;
    std::set<ClientRepr> clients;

    void serve_user(ClientRepr& repr);

    void run_user_thread(const ClientRepr& repr) const;

    void send_all(const std::string& message, const ClientRepr& repr) const;

    long send_chunk(int sock, char* buf, size_t length) const;

    [[nodiscard]] static std::string receive_all(const ClientRepr& repr);

    static std::string receive_chunk(int sock, ulong to_receive);

    static void save_file(const std::string& filename, const std::string& data);

public:
    Server(const std::string& ip_address, int port);

    [[noreturn]] void run();
};


#endif //WEBAPP_SERVER_HPP
