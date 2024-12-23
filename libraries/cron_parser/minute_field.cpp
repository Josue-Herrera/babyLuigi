#include <algorithm>
#include "minute_field.hpp"

namespace geheb {

minute_field::minute_field() {
    _allowBackwardRange = true;
}

date_time minute_field::increment(const std::string &value, const date_time &time) const {
    date_time nextTime(time);
    if (value == "*") {
        nextTime += minutes(1);
        return nextTime;
    }
    size_t pos = 0;
    auto list = create_list(value);
    for (size_t i = 0; i < list.size() - 1; i++) {
        int currentVal = list[i];
        int nextVal = list[i + 1];
        if (time.minute() >= currentVal && time.minute() < nextVal) {
            pos = i + 1;
            break;
        }
    }
    nextTime -= minutes(nextTime.minute()); // reset minutes to 0
    if (time.minute() >= list[pos]) {
        nextTime += hours(1);
    }
    else {
        nextTime += minutes(list[pos]);
    }
    return nextTime;
}

} // namespace geheb
