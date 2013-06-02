#ifndef header_E21A8E22
#define header_E21A8E22

// EEPROM content:
// byte 0-15: device revision UUID
// byte 16-31: firmware revision UUID
// byte 32-47: device UUID
// byte 0: UUID consistency state (0xff = consistent or else identifier of UUID being written [inconsistent state])

namespace Nebula
{
    namespace Firmware
    {
        class Driver
        {
        public:
            typedef unsigned char uuid_t[16];

            struct EepromContent {
                uuid_t uuid[3];
                unsigned char uuidState;
                unsigned char numberOfChannels;
            };

        protected:
            Channel channels[NUMBER_OF_CHANNELS];
            unsigned int numberOfBytesProcessed;

            void initChannels(Channel* channels);
            virtual void iterate();

        public:
            Driver();
            void runLoop();
        };
    }
}

#endif
