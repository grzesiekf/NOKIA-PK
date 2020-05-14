#pragma once
#include <string>
#include "Messages/PhoneNumber.hpp"

namespace ue
{

class IUserEventsHandler
{
public:
    virtual ~IUserEventsHandler() = default;
    virtual void handleSendingSms(common::PhoneNumber receiver, std::string content) = 0;
};

class IUserPort
{
public:
    virtual ~IUserPort() = default;

    virtual void showNotConnected() = 0;
    virtual void showConnecting() = 0;
    virtual void showConnected() = 0;

    virtual void addReceivedSms(const common::PhoneNumber senderNumber, const std::string &text) = 0;
    virtual void showNewSms() = 0;
};

}
