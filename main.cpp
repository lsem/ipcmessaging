#include <iostream>
#include <map>
#include <unordered_map>
#include <iterator>

#include "typedefs.h"
#include "messagebus.h"
#include "stream_processor.h"


using namespace std;


//
// Message types
//
struct MessageBase
{
    enum MessageID
    {
        MI_HelloMessage,
        MI_ByeMessage,
    };

    MessageBase(MessageID id) : messageId(id)
    { }

    MessageID messageId;
};


struct HelloMessage : public MessageBase
{
    HelloMessage() :
            MessageBase(MI_HelloMessage)
    { }

    explicit HelloMessage(int a) :
            MessageBase(MI_HelloMessage),
            NameId(a)
    { }

    int NameId = 0;
};
DEFINE_TYPE_ID_FOR(HelloMessage, MessageBase::MI_HelloMessage);

struct ByeMessage : public MessageBase
{
    ByeMessage() : MessageBase(MI_ByeMessage)
    { }

    int NameId = 2;
};
DEFINE_TYPE_ID_FOR(ByeMessage, MessageBase::MI_ByeMessage);


//
// Handlers
//
class TalkativeHandler : public IHandle<HelloMessage>,
                         public IHandle<ByeMessage>
{
public:
    virtual void Handle(const HelloMessage &message)
    {
        std::cout << "Hello, " << message.NameId << "! Nice to see you!" << std::endl;
    }

    virtual void Handle(const ByeMessage &message)
    {
        std::cout << "Thank you, " << message.NameId << " Bye!" << std::endl;
    }
};

class ModestHandler : public IHandle<ByeMessage>
{
public:
    virtual void Handle(const ByeMessage &message) override
    {
        std::cout << "Bye, " << message.NameId << "." << std::endl;
    }
};


//
// Utils
//
template <class TMessage, class TContainer>
void serialize(const TMessage &message, back_insert_iterator<TContainer> it)
{
    std::copy((const uint8_t *)&message, (const uint8_t *)&message + sizeof(message), it);
}


typedef MessageBus::MessageBusTemplate<MessageBus::SyncedExecutor> MessagesBusT;
typedef MessageStreamProcessor<MessagesBusT, MessageBase> MassageHandlingMachineT;

void RunDemo()
{
    std::cout << "** demo ** \n\n";

    TalkativeHandler h1;
    ModestHandler h2;

    MassageHandlingMachineT machine;

    machine.Initialize();

    machine.RegisterHandleFor<HelloMessage>(&h1);
    machine.RegisterHandleFor<ByeMessage>(&h1);
    machine.RegisterHandleFor<ByeMessage>(&h2);

    //
    // Prepare a stream of data
    //
    std::vector<uint8_t> dataBytes;

    auto bii = std::back_inserter(dataBytes);
    serialize(HelloMessage{10}, bii);
    serialize(HelloMessage{20}, bii);
    serialize(HelloMessage{30}, bii);
    serialize(ByeMessage{}, bii);
    serialize(ByeMessage{}, bii);

    //
    // Processing a stream of data
    //
    size_t start = 0;
    const size_t end = dataBytes.size();

    while (true)
    {
        const void *dataPtr = (const void *) (&dataBytes[0] + start);
        const size_t dataSize = end-start;

        size_t bytesProcessed = machine.ProcessInputMessageData(dataPtr, dataSize);
        if (bytesProcessed == 0)
        {
            std::cerr << "error: cannot deserialize; exiting...\n";
            break;
        }

        start += bytesProcessed;

        if (start >= end)
        {
            break;
        }
    }

    machine.Shutdown();

}

int main()
{
    RunDemo();
    return 0;
}

