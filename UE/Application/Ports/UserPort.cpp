#include "UserPort.hpp"
#include "UeGui/IListViewMode.hpp"
#include "UeGui/ITextMode.hpp"
#include "UeGui/ISmsComposeMode.hpp"
#include <vector>
#include <iostream>
#include <string>

namespace ue
{

UserPort::UserPort(common::ILogger& logger, IUeGui& gui, common::PhoneNumber phoneNumber)
    : logger(logger, "[USER-PORT]"),
      gui(gui),
      phoneNumber(phoneNumber)
{}

void UserPort::start(IUserEventsHandler& handler)
{
    this->handler = &handler;
    gui.setTitle("Nokia " + to_string(phoneNumber));
}

void UserPort::stop()
{
    handler = nullptr;
}

void UserPort::showNotConnected()
{
    gui.showNotConnected();
}

void UserPort::showConnecting()
{
    gui.showConnecting();
}

void UserPort::showConnected()
{
    showMainMenu();
}

void UserPort::addReceivedSms(const PhoneNumber senderNumber, const std::string& text)
{
    SMSes.emplace_back(senderNumber, text);
}

void UserPort::showNewSms()
{
    gui.showNewSms();
}

const std::vector<UserPort::SMS>& UserPort::getSMSes()
{
    return SMSes;
}

void UserPort::changeCurrentView(GUIView newView)
{
    previousView = currentView;
    currentView = newView;
}

void UserPort::showMainMenu()
{
    changeCurrentView(GUIView::MAIN_MENU);

    IUeGui::IListViewMode& menu = gui.setListViewMode();
    menu.clearSelectionList();
    menu.addSelectionListItem("Compose SMS", "");
    menu.addSelectionListItem("View received SMSes", "");
    menu.addSelectionListItem("View sent SMSes", "");

    gui.setAcceptCallback([&] {
        mainMenuAcceptCallback(menu);
    });
}

void UserPort::mainMenuAcceptCallback(IUeGui::IListViewMode& menu)
{
    switch (menu.getCurrentItemIndex().second) {
        case 0:
            showComposeSmsView(); break;
        case 1:
            showReceivedSmsListView(); break;
        case 2:
            showSentSmsListView(); break;
    }
}

void UserPort::showComposeSmsView()
{
    changeCurrentView(GUIView::COMPOSE_SMS);

    IUeGui::ISmsComposeMode& composeSms = gui.setSmsComposeMode();
    gui.setAcceptCallback([&](){handler->handleSendingSms(composeSms.getPhoneNumber(),composeSms.getSmsText());
        composeSms.clearSmsText();
        showConnected();
    });
    gui.setRejectCallback([&](){
        showConnected();
    });
}

void UserPort::showReceivedSmsListView()
{
    changeCurrentView(GUIView::RECEIVED_SMS_LIST);

    constexpr unsigned int MAX_TOOLTIP_LENGTH = 30;
    auto& smsListView = gui.setListViewMode();
    smsListView.clearSelectionList();

    std::string itemLabel, itemTooltip;
    for (const auto& sms : SMSes) {
        itemLabel = sms.isRead ? "" : "[NEW] ";
        itemLabel += "From: " + common::to_string(sms.senderNumber);
        itemTooltip = sms.text.length() > MAX_TOOLTIP_LENGTH ?
            sms.text.substr(0, MAX_TOOLTIP_LENGTH) + "..." : sms.text;
        smsListView.addSelectionListItem(itemLabel, itemTooltip);
    }

    gui.setAcceptCallback([&] {
        showSmsView(smsListView.getCurrentItemIndex().second);
    });

    gui.setRejectCallback([&] {
        goToView(previousView);
    });
}

void UserPort::showSentSmsListView()
{
    changeCurrentView(GUIView::SENT_SMS_LIST);

    auto& smsListView = gui.setListViewMode();
    smsListView.clearSelectionList();
}

void UserPort::showSmsView(size_t smsIndex)
{
    changeCurrentView(GUIView::SMS);

    auto& sms = SMSes.at(smsIndex);
    IUeGui::ITextMode& smsView = gui.setViewTextMode();
    sms.isRead = true;
    smsView.setText(sms.text);

    gui.setRejectCallback([&] {
        goToView(previousView);
    });
}

void UserPort::goToView(GUIView view)
{
    switch (view) {
        case GUIView::MAIN_MENU:
            showMainMenu(); return;
        case GUIView::SENT_SMS_LIST:
            showSentSmsListView(); return;
        case GUIView::RECEIVED_SMS_LIST:
            showReceivedSmsListView(); return;
        case GUIView::COMPOSE_SMS:
            showComposeSmsView(); return;
    }
}

}
