#pragma once

#include <string>
#include <vector>
#include <utility>

inline const std::string LOG_LINE_PATTERN =
    R"(^(?:(\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?)\s+)?)"
    R"((?:\[?)([A-Z]+)(?:\]?)\s+)"
    R"((?:\[?([a-zA-Z0-9._-]+)\]?\s+)?)"
    R"((.+)$)";

inline const std::vector<std::pair<std::string, std::string>> NORMALIZE_RULES = {
    {R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})", "<UUID>"},
    {R"(0x[0-9a-fA-F]+)",                                                  "<HEX>"},
    {R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)",                        "<IP>"},
    {R"q("([^"]*)")q",                                                     "<STR>"},
    {R"(/[^\s]+)",                                                          "<PATH>"},
    {R"(\d+)",                                                              "<NUM>"},
    {R"(\s{2,})",                                                           " "},
};
