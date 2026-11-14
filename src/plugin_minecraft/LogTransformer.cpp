/*
 * Copyright 2026 jwall64
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * \file LogTransformer.cpp
 * \author jwall64
 * \brief Converts Minecraft's LegacyXMLLayout console output into readable log lines
 */

#include "LogTransformer.hpp"

#include <Core/String.hpp>
#include <Utility/StringBuilder.hpp>

#include <cstring>
#include <ctime>
#include <new>
#include <time.h>

namespace {

constexpr usize NPOS = ~static_cast<usize>(0);

// LegacyXMLLayout writes each event over several lines, and a stack trace can make an
// individual event quite large. Keep this under the frontend's transformation buffer and
// fall back to the raw text if an event grows past it.
constexpr usize MAX_EVENT_SIZE = 96 * 1024;

struct MinecraftLogState {
    char event[MAX_EVENT_SIZE];
    usize eventLength;
    bool inEvent;
};

usize _find(nbl::StringView haystack, nbl::StringView needle, usize start = 0) {
    if(needle.empty() || haystack.length() < needle.length()) return NPOS;

    for(usize i = start; i + needle.length() <= haystack.length(); i++) {
        if(memcmp(haystack.data() + i, needle.data(), needle.length()) == 0) return i;
    }
    return NPOS;
}

bool _contains(nbl::StringView haystack, nbl::StringView needle) {
    return _find(haystack, needle) != NPOS;
}

bool _isEventStart(nbl::StringView line) {
    if(line.startsWith("<log4j:event")) return true;
    if(!line.startsWith("<event")) return false;

    return line.length() == 6 || line[6] == ' ' || line[6] == '>';
}

bool _isEventEnd(nbl::StringView line) {
    return _contains(line, "</log4j:event>") || _contains(line, "</event>");
}

nbl::StringView _attribute(nbl::StringView tag, nbl::StringView name) {
    if(name.empty()) return {};

    for(usize i = 1; i + name.length() < tag.length(); i++) {
        if(tag[i - 1] != ' ') continue;
        if(memcmp(tag.data() + i, name.data(), name.length()) != 0) continue;
        if(tag[i + name.length()] != '=') continue;

        usize start = i + name.length() + 1;
        if(start < tag.length() && tag[start] == '"') start++;

        usize end = start;
        while(end < tag.length() && tag[end] != '"') end++;
        return tag.slice(start, end - start);
    }

    return {};
}

bool _findElement(nbl::StringView xml, nbl::StringView name, nbl::StringView* outContent) {
    char openTag[64];
    char closeTag[64];

    if(name.length() + 2 > sizeof(openTag) || name.length() + 3 > sizeof(closeTag)) return false;

    openTag[0] = '<';
    memcpy(openTag + 1, name.data(), name.length());
    usize openLength = name.length() + 1;

    closeTag[0] = '<';
    closeTag[1] = '/';
    memcpy(closeTag + 2, name.data(), name.length());
    closeTag[name.length() + 2] = '>';
    usize closeLength = name.length() + 3;

    usize openIndex = _find(xml, nbl::StringView(openTag, openLength));
    if(openIndex == NPOS) return false;

    usize contentStart = _find(xml, ">", openIndex + openLength);
    if(contentStart == NPOS) return false;
    contentStart++;

    usize closeIndex = _find(xml, nbl::StringView(closeTag, closeLength), contentStart);
    if(closeIndex == NPOS) return false;

    *outContent = xml.slice(contentStart, closeIndex - contentStart);
    return true;
}

nbl::Result _appendEscaped(nbl::StringView text, nbl::StringBuilder* out) {
    usize start = 0;

    for(usize i = 0; i < text.length(); i++) {
        if(text[i] != '&') continue;

        usize end = 0;
        if(text.findFirst(';', &end, i + 1).error()) continue;

        nbl::StringView entity = text.slice(i + 1, end - i - 1);
        char replacement = 0;

        if(entity == "amp") replacement = '&';
        else if(entity == "lt") replacement = '<';
        else if(entity == "gt") replacement = '>';
        else if(entity == "quot") replacement = '"';
        else if(entity == "apos") replacement = '\'';
        else if(entity == "#39") replacement = '\'';
        else continue;

        if(i > start) NBL_CHECK(out->appendChecked(text.slice(start, i - start)));
        NBL_CHECK(out->appendChecked(replacement));
        start = end + 1;
        i = end;
    }

    if(start < text.length()) {
        NBL_CHECK(out->appendChecked(text.slice(start)));
    }

    return nbl::Result::SUCCESS;
}

nbl::Result _appendCdata(nbl::StringView content, nbl::StringBuilder* out) {
    if(content.startsWith("<![CDATA[")) content = content.slice(9);
    if(content.endsWith("]]>")) content = content.slice(0, content.length() - 3);

    // A literal "]]>" in a CDATA section is written as "]]]]><![CDATA[>".
    nbl::StringView split = "]]]]><![CDATA[>";
    usize start = 0;

    while(true) {
        usize index = _find(content, split, start);
        if(index == NPOS) {
            if(start < content.length()) NBL_CHECK(out->appendChecked(content.slice(start)));
            break;
        }

        if(index > start) NBL_CHECK(out->appendChecked(content.slice(start, index - start)));
        NBL_CHECK(out->appendChecked("]]>"));
        start = index + split.length();
    }

    return nbl::Result::SUCCESS;
}

nbl::Result _appendElementText(nbl::StringView content, nbl::StringBuilder* out) {
    if(content.empty()) return nbl::Result::SUCCESS;

    if(content.startsWith("<![CDATA[")) {
        return _appendCdata(content, out);
    }

    return _appendEscaped(content, out);
}

bool _parseTimestamp(nbl::StringView value, u64* outTimestamp) {
    if(value.empty()) return false;

    u64 timestamp = 0;
    for(char c : value) {
        if(c < '0' || c > '9') return false;
        timestamp = timestamp * 10 + static_cast<u64>(c - '0');
    }

    *outTimestamp = timestamp;
    return true;
}

bool _formatTime(u64 timestampMs, char* out, usize size) {
    std::time_t seconds = static_cast<std::time_t>(timestampMs / 1000);
    std::tm localTime{};

#if defined(_WIN32)
    if(localtime_s(&localTime, &seconds) != 0) return false;
#else
    if(localtime_r(&seconds, &localTime) == nullptr) return false;
#endif

    return std::strftime(out, size, "%H:%M:%S", &localTime) > 0;
}

nbl::Result _formatEvent(nbl::StringView event, nbl::StringBuilder* out, bool* outFormatted) {
    *outFormatted = false;

    usize tagEnd = _find(event, ">");
    if(tagEnd == NPOS) return nbl::Result::SUCCESS;

    nbl::StringView tag = event.head(tagEnd + 1);
    nbl::StringView timestamp = _attribute(tag, "timestamp");
    nbl::StringView level = _attribute(tag, "level");
    nbl::StringView thread = _attribute(tag, "thread");

    nbl::StringView message{};
    if(!_findElement(event, "log4j:message", &message) && !_findElement(event, "message", &message)) {
        return nbl::Result::SUCCESS;
    }

    u64 timestampMs = 0;
    char timeBuffer[16];
    if(_parseTimestamp(timestamp, &timestampMs) && _formatTime(timestampMs, timeBuffer, sizeof(timeBuffer))) {
        NBL_CHECK(out->appendChecked('['));
        NBL_CHECK(out->appendChecked(timeBuffer));
        NBL_CHECK(out->appendChecked("] "));
    }

    if(!thread.empty() || !level.empty()) {
        NBL_CHECK(out->appendChecked('['));
        if(!thread.empty()) NBL_CHECK(_appendEscaped(thread, out));
        if(!thread.empty() && !level.empty()) NBL_CHECK(out->appendChecked('/'));
        if(!level.empty()) NBL_CHECK(out->appendChecked(level));
        NBL_CHECK(out->appendChecked("]: "));
    }

    NBL_CHECK(_appendElementText(message, out));

    nbl::StringView throwable{};
    if(_findElement(event, "log4j:throwable", &throwable) && !throwable.empty()) {
        NBL_CHECK(out->appendChecked('\n'));
        NBL_CHECK(_appendCdata(throwable, out));
    }

    *outFormatted = true;
    return nbl::Result::SUCCESS;
}

nbl::Result _appendEvent(nbl::StringBuilder* out, MinecraftLogState* state) {
    nbl::StringView event(state->event, state->eventLength);

    bool formatted = false;
    NBL_CHECK(_formatEvent(event, out, &formatted));
    if(!formatted) NBL_CHECK(out->appendChecked(event));

    state->eventLength = 0;
    state->inEvent = false;
    return nbl::Result::SUCCESS;
}

}

nbl::Result nbl::mc::createMinecraftLogTransformer(PluginContext*, void** outState, void*) {
    auto* state = new (std::nothrow) MinecraftLogState{};
    if(state == nullptr) return nbl::Result::OUT_OF_MEMORY;

    *outState = state;
    return nbl::Result::SUCCESS;
}

nbl::Result nbl::mc::transformMinecraftLogLine(PluginContext*, void* statePtr, StringView line, StringBuilder* out, void*) {
    auto* state = static_cast<MinecraftLogState*>(statePtr);
    if(state == nullptr) {
        if(!line.empty()) NBL_CHECK(out->appendChecked(line));
        return nbl::Result::SUCCESS;
    }

    bool eventStart = _isEventStart(line);
    bool eventEnd = _isEventEnd(line);

    if(state->inEvent) {
        if(eventStart) {
            // The previous event was never closed. Keep its text instead of swallowing it.
            NBL_CHECK(out->appendChecked(nbl::StringView(state->event, state->eventLength)));
            NBL_CHECK(out->appendChecked('\n'));
            state->eventLength = 0;
            state->inEvent = false;
        } else {
            usize required = line.length() + (state->eventLength > 0 ? 1 : 0);
            if(state->eventLength + required > MAX_EVENT_SIZE) {
                NBL_CHECK(out->appendChecked(nbl::StringView(state->event, state->eventLength)));
                NBL_CHECK(out->appendChecked('\n'));
                if(!line.empty()) NBL_CHECK(out->appendChecked(line));
                state->eventLength = 0;
                state->inEvent = false;
                return nbl::Result::SUCCESS;
            }

            if(state->eventLength > 0) state->event[state->eventLength++] = '\n';
            if(!line.empty()) {
                memcpy(state->event + state->eventLength, line.data(), line.length());
                state->eventLength += line.length();
            }

            if(eventEnd) NBL_CHECK(_appendEvent(out, state));
            return nbl::Result::SUCCESS;
        }
    }

    if(eventStart) {
        if(eventEnd) {
            bool formatted = false;
            NBL_CHECK(_formatEvent(line, out, &formatted));
            if(!formatted) NBL_CHECK(out->appendChecked(line));
            return nbl::Result::SUCCESS;
        }

        state->eventLength = 0;
        state->inEvent = true;
        memcpy(state->event, line.data(), line.length());
        state->eventLength = line.length();
        return nbl::Result::SUCCESS;
    }

    // Minecraft's XML layout puts a blank line between events. Do not show those.
    if(line.empty()) return nbl::Result::SUCCESS;

    NBL_CHECK(out->appendChecked(line));
    return nbl::Result::SUCCESS;
}

void nbl::mc::destroyMinecraftLogTransformer(PluginContext*, void* state, void*) {
    delete static_cast<MinecraftLogState*>(state);
}
