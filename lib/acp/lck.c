#include "lck.h"

void acp_lck_waitUnlock(Peer *item, unsigned int interval_us) {
    char state = 1;
    char cmd_unlock[1] = {ACP_CMD_LCK_UNLOCK};
    char cmd_check[1] = {ACP_CMD_LCK_GET_DATA};
    while (1) {
        switch (state) {
            case 1:
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: sending ping request to locker...");
#endif
                acp_pingPeer(item);
                if (item->active) {
                    state = 2;
                }
                break;
            case 2:
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: sending unlock request to locker...");
#endif
                if (acp_sendStrPack(ACP_QUANTIFIER_BROADCAST, cmd_unlock, item)) {
                    state = 3;
                }
                break;
            case 3:
            {
#ifdef MODE_DEBUG
                puts("acp_waitUnlock: checking locker to be unlocked...");
#endif
                int locked = 1;
                if (acp_sendCmdGetInt(item, cmd_check, &locked)) {
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

void acp_lck_lock(Peer *item) {
    char cmd_unlock[1] = {ACP_CMD_LCK_LOCK};
    acp_sendStrPack(ACP_QUANTIFIER_BROADCAST, cmd_unlock, item);
}