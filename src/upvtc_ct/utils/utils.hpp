#ifndef UPVTC_CT_UTILS_UTILS_HPP_
#define UPVTC_CT_UTILS_UTILS_HPP_

#include <unordered_map>
#include <string>

#include <upvtc_ct/ds/models.hpp>

namespace upvtc_ct::utils
{
  template <class Key, class ValueType, class Error>
  ValueType& getValueRefFromMap(const Key key,
                                const std::unordered_map<Key, ValueType>> map,
                                const std::string errorMsg)
  {
    auto item = map.find(key);
    if (item == map.end()) {
      throw Error{errorMsg};
    }

    return item->second;
  }
}

#endif
