// #my_engine_source_file
#include "my/debug/dap/dap.h"
#include "my/utils/string_utils.h"

namespace my::dap {

ProtocolMessage::ProtocolMessage(unsigned messageSeq, std::string messageType) :
    seq(messageSeq),
    type(std::move(messageType))
{
}

EventMessage::EventMessage(unsigned messageSeq, std::string_view eventType_) :
    ProtocolMessage(messageSeq, std::string{ProtocolMessage::kMessageType_Event}),
    eventType(eventType_)
{
}

InitializedEvent::InitializedEvent(unsigned messageSeq) :
    EventMessage{messageSeq, kEvent_Initialized}
{
}

ResponseMessage::ResponseMessage(unsigned messageSeq, const RequestMessage& request) :
    ProtocolMessage(messageSeq, std::string{ProtocolMessage::kMessageType_Response}),
    request_seq(request.seq),
    command(request.command)
{
}

ResponseMessage::ResponseMessage(unsigned messageSeq, RequestMessage&& request) :
    ProtocolMessage(messageSeq, std::string{ProtocolMessage::kMessageType_Response}),
    request_seq{request.seq},
    command{std::move(request.command)}
{
}

ResponseMessage& ResponseMessage::setError(std::string_view errorMessage)
{
    success = false;
    if (!errorMessage.empty())
    {
        message.emplace(errorMessage.data(), errorMessage.size());
    }

    return *this;
}


StoppedEventBody::StoppedEventBody(std::string eventReason, std::string eventDescription) :
    reason(std::move(eventReason)),
    description(std::move(eventDescription))
{
}

Source::Source(std::string_view sourcePath) :
    path(sourcePath)
{
}

bool operator==(const Source& left, const Source& right)
{
    return left == right.path;
}

bool operator==(const Source& left, std::string_view path)
{
    if (left.path.empty() && path.empty())
    {
        return true;
    }

    if (left.path.empty() || path.empty())
    {
        return false;
    }

    // #if ENGINE_OS_WINDOWS
    return strings::icaseEqual(left.path, path);
    // #else
    //     return left.path == path;
    // #endif
}

bool Source::EqualFast(const Source& source, std::string_view path)
{
    if (source.path.length() != path.length())
    {
        return false;
    }

    auto iter1 = source.path.begin();
    auto iter2 = path.begin();

    while (iter1 != source.path.end())
    {
        const char ch1 = *iter1;
        const char ch2 = *iter2;

        if (tolower(static_cast<int>(ch1)) != tolower(static_cast<int>(ch2)))
        {
            if (!((ch1 == '\\' && ch2 == '/') || (ch1 == '/' && ch2 == '\\')))
            {
                return false;
            }
        }

        ++iter1;
        ++iter2;
    }

    return true;
}

bool Source::EqualFast(const Source& source1, const Source& source2)
{
    return Source::EqualFast(source1, source2.path);
}

OutputEventBody::OutputEventBody(std::string evenOutput, std::string eventCategory) :
    output(std::move(evenOutput)),
    category(std::move(eventCategory))
{
}

GenericEventMessage<OutputEventBody> OutputEventBody::MakeEvent(unsigned seq)
{
    return GenericEventMessage<OutputEventBody>{seq, "output"};
}

GenericEventMessage<TerminatedEventBody> TerminatedEventBody::MakeEvent(unsigned seq)
{
    return GenericEventMessage<TerminatedEventBody>{seq, "terminated"};
}

Thread::Thread(unsigned threadId, std::string_view threadName) :
    id(threadId),
    name(threadName)
{
}

StackFrame::StackFrame(unsigned frameId, std::string_view frameName) :
    id(frameId),
    name(frameName)
{
}

Variable::Variable(std::string_view variableName, std::string_view variableValue, unsigned refId) :
    name(variableName),
    value(variableValue),
    variablesReference(refId)
{
}

Variable::Variable(std::string_view variableName, unsigned refId) :
    name(variableName),
    variablesReference(refId)
{
}

Scope::Scope(std::string_view scopeName, unsigned varRef) :
    name(scopeName),
    variablesReference(varRef)
{
}

EvaluateResponseBody EvaluateResponseBody::FromVariable(Variable variable)
{
    EvaluateResponseBody response;

    response.result = !variable.value.empty() ? std::move(variable.value) : std::move(variable.name);
    response.type = std::move(variable.type);
    response.variablesReference = variable.variablesReference;
    response.indexedVariables = variable.indexedVariables;
    response.namedVariables = variable.namedVariables;
    response.presentationHint = std::move(variable.presentationHint);
    response.memoryReference = std::move(variable.memoryReference);

    return response;
}

}  // namespace my::dap
