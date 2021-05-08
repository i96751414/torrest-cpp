#ifndef TORREST_ENUMS_H
#define TORREST_ENUMS_H

namespace torrest {

    enum PeerTos {
        tos_normal = 0x00,
        tos_min_cost = 0x02,
        tos_max_reliability = 0x04,
        tos_max_throughput = 0x08,
        tos_min_delay = 0x10,
        tos_scavenger = 0x20
    };

    enum State {
        queued,
        checking,
        finding,
        downloading,
        finished,
        seeding,
        allocating,
        checking_resume_data,
        paused,
        buffering
    };

}

#endif //TORREST_ENUMS_H
