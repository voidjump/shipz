#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "net.h"
#include "event.h"
#include "team.h"

TEST(Simple, TestEventPlayerJoinsRoundTrip) {
    Buffer buffer;
    buffer.Clear();
    auto event = new EventPlayerJoins(42, BLUE, "player_nickname"); 
    EventPlayerJoins * deserialized;
    
    event->Serialize(&buffer);

    ASSERT_TRUE(buffer.length > 5);
    buffer.Seek(0);

    deserialized = static_cast<EventPlayerJoins*>(Event::Deserialize(&buffer));

    ASSERT_EQ(deserialized->GetMessageSubType(), PLAYER_JOINS);
    ASSERT_EQ(deserialized->client_id, 42);
    ASSERT_EQ(deserialized->team, BLUE);
    ASSERT_STREQ(deserialized->player_name.c_str(), "player_nickname");
}