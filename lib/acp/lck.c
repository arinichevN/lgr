#include "lck.h"

void acp_lck_waitUnlock(Peer *peer, unsigned int interval_us) {
    char state = 1;
    while (1) {
        switch (state) {
            case 1:
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: sending ping request to locker...");
#endif
                acp_pingPeer(peer);
                if (peer->active) {
                    state = 2;
                }
                break;
            case 2:
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: sending unlock request to locker...");
#endif
                if (acp_requestSendUnrequitedCmd(ACP_CMD_LCK_UNLOCK, peer)) {
                    state = 3;
                }
                break;
            case 3:
            {
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: checking locker to be unlocked...");
#endif
                int locked = 1;
                if (acp_sendCmdGetInt(peer, ACP_CMD_GET_DATA, &locked)) {
                    if (!locked) {
                        state = 4;
                    }
                }
                break;
            }
            case 4:
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: done!");
#endif
                return;
                break;
        }
        delayUsIdle(interval_us);
    }
}

void acp_lck_lock(Peer *peer) {
    acp_requestSendUnrequitedCmd(ACP_CMD_LCK_LOCK, peer);
}