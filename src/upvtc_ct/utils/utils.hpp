#ifndef UPVTC_CT_UTILS_UTILS_HPP_
#define UPVTC_CT_UTILS_UTILS_HPP_

#include <unordered_map>
#include <string>

namespace upvtc_ct::utils
{
  template <class Key, class ValueType, class Error>
  ValueType& getValueRefFromMap(const Key key,
                                std::unordered_map<Key, ValueType>& map,
                                const char* errorMsg)
  {
    auto item = map.find(key);
    if (item == map.end()) {
      throw Error{errorMsg};
    }

    return item->second;
  }
}

#endif
