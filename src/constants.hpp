#pragma once

#include <string>
#include <vector>
#include <utility>  // for std::pair

// R"(...)" is a raw string literal — backslashes are literal, no escaping needed
// Exactly like Python's r"..." prefix
// This makes regex patterns readable — without it, every \d would need to be \\d

// Matches a full log line with optional timestamp, level, optional component, message
// This is the same pattern as Python's LOG_LINE_RE
inline const std::string LOG_LINE_PATTERN =
    R"(^(?:(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?)\s+)?)"
    R"((?:\[?)([A-Z]+)(?:\]?)\s+)"
    R"((?:\[?([a-zA-Z0-9._-]+)\]?\s+)?)"
    R"((.+)$)";
// Adjacent string literals are joined automatically — like Python's "a" "b" → "ab"
// Used here to split the long pattern across multiple lines for readability

// std::pair<A, B> is like a 2-element Python tuple: (pattern, replacement)
// std::vector<std::pair<...>> is like Python's list of (pattern, replacement) tuples
// Each rule replaces a variable pattern with a fixed placeholder
inline const std::vector<std::pair<std::string, std::string>> NORMALIZE_RULES = {
    // UUIDs: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx → <UUID>
    {R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})", "<UUID>"},
    // Hex values: 0x1a2b → <HEX>
    {R"(0x[0-9a-fA-F]+)", "<HEX>"},
    // IP addresses: 192.168.1.1 → <IP>
    {R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)", "<IP>"},
    // Quoted strings: "hello world" → <STR>
    // Note: raw string uses 'q' delimiter because the pattern itself contains )"
    // which would otherwise end the raw string early — a C++ quirk worth knowing
    {R"q("([^"]*)")q", "<STR>"},
    // File paths: /var/log/app.log → <PATH>
    {R"(/[^\s]+)", "<PATH>"},
    // Plain numbers: 42, 3000, 123ms → <NUM>
    // Using \d+ without \b word boundaries so "123ms" → "<NUM>ms"
    // Same behaviour as the Python original
    {R"(\d+)", "<NUM>"},
    // Collapse multiple spaces to one
    {R"(\s{2,})", " "},
};
