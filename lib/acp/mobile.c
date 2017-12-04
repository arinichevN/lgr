#include "mobile.h"
#include "../app.h"
int acp_sendSMS(Peer *peer, char *phone, char *message) {
    if (lockPeer(peer)) {
        S2 di[1];
        memcpy(di[0].p0, phone, LINE_SIZE);
        memcpy(di[0].p1, message, LINE_SIZE);
        S2List data = {di, 1};
        if (!acp_requestSendUnrequitedS2List(ACP_CMD_MOBILE_SEND_SMS, &data, peer)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "ERROR: acp_sendSMS: failed to send request where peer.id = %s\n", peer->id);
#endif
            unlockPeer(peer);
            return 0;
        }
        unlockPeer(peer);
        return 1;
    }
    return 0;
}

int acp_makeCall(Peer *peer, char *phone) {
    if (lockPeer(peer)) {
        S1List data = {phone, 1};
        if (!acp_requestSendUnrequitedS1List(ACP_CMD_MOBILE_RING, &data, peer)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "ERROR: acp_makeCall: failed to send request where peer.id = %s\n", peer->id);
#endif
            unlockPeer(peer);
            return 0;
        }
        unlockPeer(peer);
        return 1;
    }
    return 0;
}
