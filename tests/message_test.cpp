#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "net/net.h"
#include "messages/event.h"
#include "net/message.h"

TEST(Simple, TestMessageType) {
    auto event = new EventPlayerJoins(0, 0, ""); 

    event->SetMessageSubType(0b00000101);
    event->SetMessageType(  (MessageType) 0b00000010);
    event->SetReliability( true );

    ASSERT_EQ(event->GetFullType(), 0b01000101);
    event->SetReliability( false );
    ASSERT_EQ(event->GetFullType(), 0b01000101);

    event->SetMessageSubType(0b00000011);
    event->SetMessageType(   (MessageType) 0b00000011);
    event->SetReliability( true );
    ASSERT_EQ(event->GetFullType(), 0b01100011);
}

TEST(Simple, TestConstructHeader) {

    Uint8 header = ConstructHeader((MessageType) 0b00000010, 0b00000101);
    ASSERT_EQ(header, 0b01000101);

    header = ConstructHeader((MessageType) 0b00000011, 0b00000011);
    ASSERT_EQ(header, 0b01100011);
}