#pragma once

#include <vector>
#include <string>
#include <array>
#include <queue>
#include <deque>
#include <unordered_map>
#include <unordered_set>

template<typename T>
using DynArr = std::vector<T>;
template<typename T, std::size_t len>
using Arr = std::array<T, len>;
template<typename T>
using Queue = std::queue<T>;
template<typename T>
using Deque = std::deque<T>;
template<typename T, typename Hasher>
using HashMap = std::unordered_map<T, Hasher>;
template<typename T, typename Hasher>
using HashSet = std::unordered_set<T, Hasher>;

typedef std::string String;