//
// Created by Алексей Гладков on 20.01.2022.
//

#ifndef WEBAPP_CLIENTREPR_HPP
#define WEBAPP_CLIENTREPR_HPP


#include <string>

struct ClientRepr
{
    explicit ClientRepr(int sock_descriptor) : sock_d(sock_descriptor)
    {

    }

    bool operator<(const ClientRepr& cl) const
    {
        return this->sock_d < cl.sock_d;
    }

    int sock_d;
    std::string username;
};


#endif //WEBAPP_CLIENTREPR_HPP
