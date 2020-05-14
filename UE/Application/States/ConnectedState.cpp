#include "ConnectedState.hpp"
#include "NotConnectedState.hpp"

namespace ue
{

ConnectedState::ConnectedState(Context& context)
    : BaseState(context, "ConnectedState")
{
    context.user.showConnected();
}

void ConnectedState::handleDisconnected()
{
    logger.logInfo("Disconnected");
    context.setState<NotConnectedState>();
}

void ConnectedState::handleReceivingSms(common::PhoneNumber senderNumber, const std::string& text)
{
    logger.logInfo("Receive sms from ", senderNumber);
    context.user.addReceivedSms(senderNumber, text);
    //context.user.showNewSms();
}

}
