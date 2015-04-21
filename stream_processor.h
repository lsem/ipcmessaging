#pragma once

#include "typedefs.h"
#include "deserializer.h"

#include <memory>
#include <unordered_map>


// Warning: this class is not thread safe.
template <typename TMessageBus, class TMessageBase>
class MessageStreamProcessor
{
public:
    template<class TMessage>
    void RegisterHandleFor(IHandle<TMessage> *handler)
    {
        const auto id = static_cast<const TMessageBase &>(TMessage()).messageId;

        auto findIt = m_registry.find(id);

        m_registry.insert(std::make_pair(id, findIt == m_registry.end()
                 ? std::make_shared<Deserializer::DeserializerTemplate<TMessage, TMessageBus>>(m_messageBus)
                 : findIt->second));

        m_messageBus.RegisterMessageHandler(handler);
    }

    size_t ProcessInputMessageData(const void *dataPtr, size_t dataSize)
    {
        size_t result = 0;

        const auto *messageBase = reinterpret_cast<const TMessageBase *>(dataPtr);

        auto deserializer = m_registry[messageBase->messageId];
        if (deserializer->CanBeDeserialized(dataPtr, dataSize))
        {
            result = deserializer->Deserialize(dataPtr, dataSize);
        }

        return result;
    }

    void Initialize()
    { }

    void Shutdown()
    {
        m_messageBus.Shutdown();
    }

private:
    std::unordered_map<unsigned, std::shared_ptr<Deserializer::IDeserializer>> m_registry;
    TMessageBus m_messageBus;
};

