#include <random>
#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "packet.h"

TEST(Simple, TestPacketEncryptionRoundTrip) {
    const char * test_string = "encryption payload";
    const char * key = "0123456789abcdef013456789abcdef";
    Packet mypacket;
    mypacket.Clear();
    mypacket.WriteString(test_string);
    
    // Expect the string to be read into the packet
    EXPECT_EQ(mypacket.length, strlen(test_string));
    // Expect packet contents to be equal to the string
    ASSERT_STREQ((char *)mypacket.data, test_string);

    // Encrypt packet
    mypacket.Encrypt((Uint8*) key);
    ASSERT_STRNE((char *)mypacket.data, test_string);
    // Decrypt packet
    mypacket.Decrypt((Uint8*) key);
    // Contents should be equal to start, but it has padding
    ASSERT_THAT((char *)mypacket.data, testing::StartsWith(test_string));
}


TEST(Simple, TestBlockSizeLengthPayloadDoesNotAddPadding) {
    const char * test_string = "0123456789abcdef";
    const char * key = "0123456789abcdef013456789abcdef";
    Packet mypacket;
    mypacket.Clear();
    mypacket.WriteString(test_string);
    
    ASSERT_EQ(strlen(test_string), AES_BLOCKSIZE);

    // Expect the string to be read into the packet
    EXPECT_EQ(mypacket.length, strlen(test_string));
    // Expect packet contents to be equal to the string
    ASSERT_STREQ((char *)mypacket.data, test_string);

    // Encrypt packet
    mypacket.Encrypt((Uint8*) key);
    ASSERT_STRNE((char *)mypacket.data, test_string);
    // Decrypt packet
    mypacket.Decrypt((Uint8*) key);
    // Contents should be exactly equal to start
    ASSERT_STREQ((char *)mypacket.data, test_string);
}


TEST(Simple, TestPaddingAmountAsExpected) {
    // Create a random string of length 1-(AES_BLOCKSIZE-1)
    std::random_device rand_dev;
    unsigned char length = AES_BLOCKSIZE + static_cast<unsigned char>(rand_dev() % (AES_BLOCKSIZE-2)) + 1;
    unsigned char expected_pad_byte = AES_BLOCKSIZE - length;
    std::string test_string;
    for(int i; i<length; i++){
        test_string.append("x");
    }

    const char * key = "0123456789abcdef013456789abcdef";
    Packet mypacket;
    mypacket.Clear();
    mypacket.WriteString(test_string.c_str());

    // Expect the string to be read into the packet
    EXPECT_EQ(mypacket.length, strlen(test_string.c_str()));

    // Expect packet contents to be equal to the string
    ASSERT_STREQ((char *)mypacket.data, test_string.c_str());

    // Encrypt packet
    mypacket.Encrypt((Uint8*) key);
    ASSERT_STRNE((char *)mypacket.data, test_string.c_str());
    // Expect the length of the encrypted packet to be congruent 0 mod blocksize
    EXPECT_EQ(mypacket.length % AES_BLOCKSIZE, 0);

    // Decrypt packet
    mypacket.Decrypt((Uint8*) key);

    // Expect the length of the decrypted packet to be congruent 0 mod blocksize
    EXPECT_EQ(mypacket.length % AES_BLOCKSIZE, 0);

    // Seek the buffer to the position where the padding starts
    // Expect the last AES_BLOCKSIZE - length bytes to contain that value
    mypacket.Seek(mypacket.length - expected_pad_byte - 1);
    for(int i; i < expected_pad_byte; i++){
        ASSERT_EQ(mypacket.Read8(), expected_pad_byte);
    }
}
