#include "mobile.h"
#include "../app.h"

int acp_sendSMS(Peer *peer, const char *phone, const char *message) {
    S2 di[1];
    memcpy(di[0].p0, phone, LINE_SIZE);
    memcpy(di[0].p1, message, LINE_SIZE);
    S2List data = {.item = di, .length = 1, .max_length = 1};
    if (!acp_requestSendUnrequitedS2List(ACP_CMD_MOBILE_SEND_SMS, &data, peer)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "ERROR: acp_sendSMS: failed to send request where peer.id = %s\n", peer->id);
#endif
        return 0;
    }
    return 1;
}

int acp_makeCall(Peer *peer, const char *phone) {
        S1 di[LINE_SIZE];
        memcpy(di, phone, LINE_SIZE);
        S1List data = {.item = di, .length = 1, .max_length = 1};
        if (!acp_requestSendUnrequitedS1List(ACP_CMD_MOBILE_RING, &data, peer)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "ERROR: acp_makeCall: failed to send request where peer.id = %s\n", peer->id);
#endif
            return 0;
        }
        return 1;
}
