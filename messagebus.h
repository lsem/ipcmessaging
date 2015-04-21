#pragma once

#include "typedefs.h"

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <future>
#include <thread>
#include <condition_variable>


namespace MessageBus {

  class SyncedExecutor
  {
  public:
      template<class TMessage>
      void Execute(IHandle<TMessage> *handler, const TMessage &message)
      {
          handler->Handle(message);
      }

      bool Shutdown()
      { return true; }
  };

  class AsyncExecutor
  {
  public:
      template<class TMessage>
      void Execute(IHandle<TMessage> *handler, const TMessage &message)
      {
          std::async(std::launch::async, [&]
          {
              handler->Handle(message);
          });
      }

      bool Shutdown()
      { return true; }
  };

  class QueuedAsyncExecutor
  {
      std::mutex m_queueLock;
      std::condition_variable m_dataAvailable;
      std::list<std::function<void()>> m_queue;
      bool m_dataAvailableFlag = false;
      bool m_shutdownRequested = false;
      bool m_shutdownConfirmedFlag = false;

  public:
      QueuedAsyncExecutor()
      {
          std::thread thread(&QueuedAsyncExecutor::ThreadEntryPoint, this);
          thread.detach();
      }

      bool Shutdown()
      {
          RequestShutdown();
          WaitShutdownSpinLocked();
          return true;
      }

      template<class TMessage>
      void Execute(IHandle<TMessage> *handler, const TMessage &message)
      {
          if (handler != nullptr)
          {
              std::cout << "pushed\n";
              std::unique_lock<std::mutex> locked(m_queueLock);

              m_queue.push_back([=] { handler->Handle(message); });
              m_dataAvailableFlag = true;
              m_dataAvailable.notify_one();
          }
      }

  private:
      static void ThreadEntryPoint(QueuedAsyncExecutor *instance)
      { instance->Execute(); }

      void Execute()
      {
          while (true)
          {
              std::unique_lock<std::mutex> locked(m_queueLock);

              while (!m_dataAvailableFlag && !m_shutdownRequested)
              {
                  m_dataAvailable.wait(locked);
              }

              if (m_shutdownRequested)
              {
                  break;
              }
              else if (m_dataAvailableFlag)
              {
                  while (!m_queue.empty())
                  {
                      std::function<void()> dispatcher = m_queue.front();
                      m_queue.pop_front();
                      dispatcher();
                  }

                  m_dataAvailableFlag = false;
              }
          }

          m_shutdownConfirmedFlag = true;
      }

      bool WaitShutdownSpinLocked()
      {
          while (!m_shutdownConfirmedFlag)
          {
              ;
          }

          return true;
      }

      void RequestShutdown()
      {
          std::unique_lock<std::mutex> locked(m_queueLock);
          m_shutdownRequested = true;
          m_dataAvailable.notify_one();
      }
  };


  struct CompilerTimeTypeIDResolver
  {
    template <class TMessage>
      static typeid_t Resolve()
    {
        return TypeIdResolver<TMessage>::getType();
    }
  };


  template<class TExecutor, class TMessageTypeIDResolver=CompilerTimeTypeIDResolver>
  class MessageBusTemplate : private TExecutor
  {
  public:
      template<class TMessage>
      void RegisterMessageHandler(IHandle<TMessage> *handler)
      {
          const typeid_t typeId = TMessageTypeIDResolver::template Resolve<TMessage>();
          handlersMap[typeId].push_back(handler);
      }

      template<class TMessage>
      bool IssueMessage(const TMessage &message)
      {
          const typeid_t typeId = TMessageTypeIDResolver::template Resolve<TMessage>();

          const auto &handlersList = handlersMap[typeId];
          for (auto &handlerBase : handlersList)
          {
              auto *handler = static_cast<IHandle<TMessage> *>(handlerBase);
              TExecutor::Execute(handler, message);
          }

          return true; // explicitly return true, but it should be results of from TExecutor::Execute
      }

      bool Shutdown()
      {
          return TExecutor::Shutdown();
      }

  private:
      // note, vector is used here intentionally (instead of list)
      std::map<typeid_t, std::vector<HandlerBase *>> handlersMap;
  };

}
