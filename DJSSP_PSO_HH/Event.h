#pragma once

#include "Machine.h"

namespace djssp {

enum class EventType : uint8_t {
    JOB_ARRIVAL,
    OP_COMPLETE,
};

struct Event {
    double time{};
    EventType type{};
    JobId job_id{-1};
    MachineId machine_id{-1};
    Operation* op{nullptr};
    uint64_t seq{0};
};

struct EventCmp {
    bool operator()(const Event& a, const Event& b) const {
        if (a.time != b.time) {
            return a.time > b.time;
        }
        return a.seq > b.seq;
    }
};

struct EventQueue {
    std::priority_queue<Event, std::vector<Event>, EventCmp> pq;

    void push(const Event& event) {
        pq.push(event);
    }

    bool empty() const {
        return pq.empty();
    }

    Event pop() {
        Event event = pq.top();
        pq.pop();
        return event;
    }
};

}  // namespace djssp
