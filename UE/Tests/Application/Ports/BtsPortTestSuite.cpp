#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Ports/BtsPort.hpp"
#include "Mocks/ILoggerMock.hpp"
#include "Mocks/IBtsPortMock.hpp"
#include "Messages/PhoneNumber.hpp"
#include "Mocks/ITransportMock.hpp"
#include "Messages/OutgoingMessage.hpp"
#include "Messages/IncomingMessage.hpp"

namespace ue
{
using namespace ::testing;

class BtsPortTestSuite : public Test
{
protected:
    const common::PhoneNumber PHONE_NUMBER{112};
    const common::BtsId BTS_ID{13121981ll};
    const std::string TEXT{"example text"};
    NiceMock<common::ILoggerMock> loggerMock;
    StrictMock<IBtsEventsHandlerMock> handlerMock;
    StrictMock<common::ITransportMock> transportMock;
    common::ITransport::MessageCallback messageCallback;
    common::ITransport::DisconnectedCallback disconnectedCallback;

    BtsPort objectUnderTest{loggerMock, transportMock, PHONE_NUMBER};

    BtsPortTestSuite()
    {
        EXPECT_CALL(transportMock, registerMessageCallback(_))
            .WillOnce(SaveArg<0>(&messageCallback));

        EXPECT_CALL(transportMock, registerDisconnectedCallback(_))
            .WillOnce(SaveArg<0>(&disconnectedCallback));

        objectUnderTest.start(handlerMock);
    }

    ~BtsPortTestSuite()
    {
        EXPECT_CALL(transportMock, registerDisconnectedCallback(IsNull()));
        EXPECT_CALL(transportMock, registerMessageCallback(IsNull()));
        objectUnderTest.stop();
    }
};

TEST_F(BtsPortTestSuite, shallRegisterHandlersBetweenStartStop)
{
}

TEST_F(BtsPortTestSuite, shallIgnoreWrongMessage)
{
    common::OutgoingMessage wrongMsg{};
    wrongMsg.writeBtsId(BTS_ID);
    messageCallback(wrongMsg.getMessage());
}

TEST_F(BtsPortTestSuite, shallHandleSib)
{
    EXPECT_CALL(handlerMock, handleSib(BTS_ID));
    common::OutgoingMessage msg{common::MessageId::Sib,
                                common::PhoneNumber{},
                                PHONE_NUMBER};
    msg.writeBtsId(BTS_ID);
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallHandleAttachAccept)
{
    EXPECT_CALL(handlerMock, handleAttachAccept());
    common::OutgoingMessage msg{common::MessageId::AttachResponse,
                                common::PhoneNumber{},
                                PHONE_NUMBER};
    msg.writeNumber(true);
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallHandleAttachReject)
{
    EXPECT_CALL(handlerMock, handleAttachReject());
    common::OutgoingMessage msg{common::MessageId::AttachResponse,
                                common::PhoneNumber{},
                                PHONE_NUMBER};
    msg.writeNumber(false);
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallSendAttachRequest)
{
    common::BinaryMessage msg;
    EXPECT_CALL(transportMock, sendMessage(_)).WillOnce([&msg](auto param) {
        msg = std::move(param);
        return true;
    });
    objectUnderTest.sendAttachRequest(BTS_ID);
    common::IncomingMessage reader(msg);
    ASSERT_NO_THROW(EXPECT_EQ(common::MessageId::AttachRequest, reader.readMessageId()));
    ASSERT_NO_THROW(EXPECT_EQ(PHONE_NUMBER, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(common::PhoneNumber{}, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(BTS_ID, reader.readBtsId()));
    ASSERT_NO_THROW(reader.checkEndOfMessage());
}

TEST_F(BtsPortTestSuite, shallHandleDiscconnected)
{
    EXPECT_CALL(handlerMock, handleDisconnected());
    disconnectedCallback();
}

TEST_F(BtsPortTestSuite, shallReceiveCorrectSmsFromUe)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::OutgoingMessage o_msg{common::MessageId::Sms,
                                  receivingPhoneNumber,
                                  PHONE_NUMBER};
    o_msg.writeBtsId(BTS_ID);
    o_msg.writeText(TEXT);
    common::BinaryMessage msg = o_msg.getMessage();
    common::IncomingMessage reader(msg);

    ASSERT_NO_THROW(EXPECT_EQ(common::MessageId::Sms, reader.readMessageId()));
    ASSERT_NO_THROW(EXPECT_EQ(receivingPhoneNumber, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(PHONE_NUMBER, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(BTS_ID, reader.readBtsId()));
    ASSERT_NO_THROW(EXPECT_EQ(TEXT, reader.readRemainingText()));
    ASSERT_NO_THROW(reader.checkEndOfMessage());
}

TEST_F(BtsPortTestSuite, shallReceiveSms)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::OutgoingMessage msg{common::MessageId::Sms,
                                receivingPhoneNumber,
                                PHONE_NUMBER};
    msg.writeText(TEXT);

    EXPECT_CALL(handlerMock, handleReceivingSms(receivingPhoneNumber, TEXT));
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallSendCallRequest)
{
    common::BinaryMessage msg;
    EXPECT_CALL(transportMock, sendMessage(_)).WillOnce([&msg](auto param) {
        msg = std::move(param);
        return true;
    });
    objectUnderTest.sendCallRequest(common::PhoneNumber{});
    common::IncomingMessage reader(msg);
    ASSERT_NO_THROW(EXPECT_EQ(common::MessageId::CallRequest, reader.readMessageId()));
    ASSERT_NO_THROW(EXPECT_EQ(PHONE_NUMBER, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(common::PhoneNumber{}, reader.readPhoneNumber()));
    ASSERT_NO_THROW(reader.checkEndOfMessage());
}

TEST_F(BtsPortTestSuite, shallSendCallDrop)
{
    common::BinaryMessage msg;
    EXPECT_CALL(transportMock, sendMessage(_)).WillOnce([&msg](auto param) {
        msg = std::move(param);
        return true;
    });
    objectUnderTest.sendCallDrop(common::PhoneNumber{});
    common::IncomingMessage reader(msg);
    ASSERT_NO_THROW(EXPECT_EQ(common::MessageId::CallDropped, reader.readMessageId()));
    ASSERT_NO_THROW(EXPECT_EQ(PHONE_NUMBER, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(common::PhoneNumber{}, reader.readPhoneNumber()));
    ASSERT_NO_THROW(reader.checkEndOfMessage());
}

TEST_F(BtsPortTestSuite, shallSendCallAccept)
{
    common::BinaryMessage msg;
    EXPECT_CALL(transportMock, sendMessage(_)).WillOnce([&msg](auto param) {
        msg = std::move(param);
        return true;
    });
    objectUnderTest.sendCallAccept(common::PhoneNumber{});
    common::IncomingMessage reader(msg);
    ASSERT_NO_THROW(EXPECT_EQ(common::MessageId::CallAccepted, reader.readMessageId()));
    ASSERT_NO_THROW(EXPECT_EQ(PHONE_NUMBER, reader.readPhoneNumber()));
    ASSERT_NO_THROW(EXPECT_EQ(common::PhoneNumber{}, reader.readPhoneNumber()));
    ASSERT_NO_THROW(reader.checkEndOfMessage());
}

TEST_F(BtsPortTestSuite, shallReceiveCorrectCallRequest)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::OutgoingMessage msg{common::MessageId::CallRequest,
                                receivingPhoneNumber,
                                PHONE_NUMBER};

    EXPECT_CALL(handlerMock, handleReceivingCallRequest(receivingPhoneNumber));
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallReceiveSameFromAndToNumber)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::PhoneNumber sendingPhoneNumber{113};
    common::OutgoingMessage msg{common::MessageId::CallRequest,
                                receivingPhoneNumber,
                                sendingPhoneNumber};

    EXPECT_CALL(handlerMock, handleUnknownReceiver(receivingPhoneNumber));
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallReceiveCallDropped)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::OutgoingMessage msg{common::MessageId::CallDropped,
                                receivingPhoneNumber,
                                PHONE_NUMBER};

    EXPECT_CALL(handlerMock, handleReceivingCallDrop(receivingPhoneNumber));
    messageCallback(msg.getMessage());
}

TEST_F(BtsPortTestSuite, shallReceiveCallAccept)
{
    common::PhoneNumber receivingPhoneNumber{113};
    common::OutgoingMessage msg{common::MessageId::CallAccepted,
                                receivingPhoneNumber,
                                PHONE_NUMBER};

    EXPECT_CALL(handlerMock, handleReceivingCallAccept(receivingPhoneNumber));
    messageCallback(msg.getMessage());
}

}
