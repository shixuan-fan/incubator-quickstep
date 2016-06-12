/**
 *   Copyright 2016, Quickstep Research Group, Computer Sciences Department,
 *     University of Wisconsin—Madison.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 **/

#ifndef QUICKSTEP_UTILITY_EVENT_PROFILER_HPP_
#define QUICKSTEP_UTILITY_EVENT_PROFILER_HPP_

#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <map>
#include <ostream>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "threading/Mutex.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Utility
 *  @{
 */

using clock = std::chrono::steady_clock;

class EventProfiler {

 public:
  EventProfiler()
      : zero_time(clock::now()) {
  }

  struct EventInfo {
    clock::time_point start_time;
    clock::time_point end_time;
    bool is_finished;
    std::size_t payload;

    explicit EventInfo(const clock::time_point &start_time_in)
        : start_time(start_time_in),
          is_finished(false) {
    }

    EventInfo()
        : start_time(clock::now()),
          is_finished(false) {
    }

    inline void setPayload(const std::size_t &in_payload) {
      payload = in_payload;
    }

    inline void endEvent() {
      end_time = clock::now();
      is_finished = true;
    }
  };

  struct EventContainer {
    inline void startEvent(const std::string &tag) {
      events[tag].emplace_back(clock::now());
    }

    inline void endEvent(const std::string &tag) {
      auto &event_info = events.at(tag).back();
      event_info.is_finished = true;
      event_info.end_time = clock::now();
    }

    inline std::vector<EventInfo> *getEventLine(const std::string &tag) {
      return &events[tag];
    }

    std::map<std::string, std::vector<EventInfo>> events;
  };

  EventContainer *getContainer() {
    MutexLock lock(mutex_);
    return &thread_map_[std::this_thread::get_id()];
  }

  void writeToStream(std::ostream &os) const {
    time_t rawtime;
    time(&rawtime);
    char event_id[32];
    strftime(event_id, sizeof event_id, "%Y-%m-%d %H:%M:%S", localtime(&rawtime));

    int thread_id = 0;
    for (const auto &thread_ctx : thread_map_) {
      for (const auto &event_group : thread_ctx.second.events) {
        for (const auto &event_info : event_group.second) {
          CHECK(event_info.is_finished) << "Unfinished profiling event";

          os << std::setprecision(12)
             << event_id << ","
             << thread_id << "," << event_group.first << ","
             << event_info.payload << ","
             << std::chrono::duration<double>(event_info.start_time - zero_time).count()
             << ","
             << std::chrono::duration<double>(event_info.end_time - zero_time).count()
             << "\n";
        }
      }
      ++thread_id;
    }
  }

  void clear() {
    zero_time = clock::now();
    thread_map_.clear();
  }

 private:
  clock::time_point zero_time;
  std::map<std::thread::id, EventContainer> thread_map_;
  Mutex mutex_;
};

extern EventProfiler simple_profiler;

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_EVENT_PROFILER_HPP_
