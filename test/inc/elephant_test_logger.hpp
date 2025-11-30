#pragma once
#include <iostream>
#include <diagnostics/logger.hpp>

namespace elephant_test {
namespace logging_internals {

class MessageStream {
public:
    MessageStream(std::ostream& stream = std::cout) :
        m_stream(stream)        
    {
    }

    ~MessageStream()
    { 
        m_stream << "\u001b[0m\n";        
    }

    template<typename T>
    inline MessageStream& operator<<(const T& t)
    {
        m_stream << t;
        return *this;
    }    

    template<typename T>
    inline MessageStream& operator<<(T* const& pointer)
    {
        if (pointer == nullptr)
            m_stream << "(nullptr)";
        else
            m_stream << pointer;

        return *this;
    }

    inline MessageStream& operator<<(bool value)
    {
        m_stream << (value ? "true" : "false");
        return *this;
    }
    
    std::ostream& stream() { return m_stream; }

private:    
    
    std::ostream& m_stream;
    void operator=(const MessageStream& other);
};

} // namespace LoggingInternals

class BasicMessage {
private:
    std::ostream& m_stream;

public:
    BasicMessage( std::string id = "", std::ostream& stream = std::cout) :
        m_stream(stream)
    {
        if (!id.empty())
        {
            size_t additionalSpaces = 8 - id.size();
            if (additionalSpaces > 0)
            {
                id.append(additionalSpaces, ' ');
            }  
            m_stream << "\u001b[32m[ " << id << " ] \u001b[0m";         
        }
        else
        {
            m_stream << "\u001b[32m[          ] \u001b[0m";
        }
    }

    template<typename T>
    inline logging_internals::MessageStream operator<<(const T& value)
    {
        logging_internals::MessageStream msg(m_stream);
        msg << value;
        return msg;
    }

    // Provide access to the underlying stream for functions like io::printer::board()
    std::ostream& stream() { return m_stream; }
};

} // namespace ElephantTest

#define OUT() \
    switch (0)    \
    case 0:       \
    default:      \
        elephant_test::BasicMessage()

#define OUT_ID(id) \
    switch (0)    \
    case 0:       \
    default:      \
        elephant_test::BasicMessage(id)

#define OUT_ID_STREAM(id, stream) \
    switch (0)    \
    case 0:       \
    default:      \
        elephant_test::BasicMessage(id, stream)

#define OUT_PASSED() \
    switch (0)    \
    case 0:       \
    default:      \
        elephant_test::BasicMessage(" PASSED ")

#define OUT_FAILED() \
    switch (0)    \
    case 0:       \
    default:      \
        elephant_test::BasicMessage("\033[31m FAILED \u001b[32m")

// Macro to get the underlying stream from BasicMessage for use with functions like io::printer::board()
#define OUT_STREAM() \
    elephant_test::BasicMessage().stream()