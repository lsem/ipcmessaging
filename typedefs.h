#pragma once

class HandlerBase
{
};

typedef unsigned typeid_t;


template <class TMessage>
class IHandle : public HandlerBase
{
public:
    virtual void Handle(const TMessage &message) = 0;
};


template <class TMessage>
struct TypeIdResolver
{
public:
    static int constexpr getType();
};


#define DEFINE_TYPE_ID_FOR(XMessageClassName, XMessageClassId) \
    template<> struct TypeIdResolver<XMessageClassName> { \
        static int constexpr getType() { return XMessageClassId; } \
    };
