#pragma once

#include <cstddef>

namespace Deserializer {

  class IDeserializer
  {
  public:
      virtual ~IDeserializer()
      { }

      // Returns how much of provided data consumed for deserialization
      virtual size_t Deserialize(const void *dataPtr, size_t dataSize) = 0;

      virtual bool CanBeDeserialized(const void *dataPtr, size_t dataSize) = 0;
  };


  template<class TMessage, typename TMessageBus>
  class DeserializerTemplate : public IDeserializer
  {
  public:
      DeserializerTemplate(TMessageBus &messageBus) :
              m_messageBus(messageBus)
      { }

      virtual size_t Deserialize(const void *dataPtr, size_t dataSize) override
      {
          TMessage message(*reinterpret_cast<const TMessage *>(dataPtr));
          m_messageBus.IssueMessage(message);
          return sizeof(TMessage);
      }

      virtual bool CanBeDeserialized(const void *dataPtr, size_t dataSize) override
      {
          return sizeof(TMessage) <= dataSize;
      }

  private:
      TMessageBus &m_messageBus;
  };

}