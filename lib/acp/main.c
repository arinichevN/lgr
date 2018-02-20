
#include "main.h"

FUN_LIST_GET_BY_IDSTR(Peer)

FUN_LIST_GET_BY_ID(SensorFTS)

FUN_LIST_GET_BY_ID(EM)

FUN_LIST_INIT(I1)

FUN_LIST_INIT(I2)

FUN_LIST_INIT(I3)

FUN_LIST_INIT(F1)

FUN_LIST_INIT(I1F1)

FUN_LIST_INIT(D1)

FUN_LIST_INIT(S1)

FUN_LIST_INIT(I1S1)

FUN_LIST_INIT(S2)

FUN_LIST_INIT(FTS)

FUN_LIST_INIT(Peer)

FUN_LIST_INIT(SensorInt)

FUN_LIST_INIT(SensorFTS)

FUN_LIST_INIT(EM)

void freePeerList(PeerList *list) {
    for (int i = 0; i < list->length; i++) {
        free(list->item[i].id);
        free(list->item[i].addr_str);
    }
    FREE_LIST(list);
}

static void acp_dumpBuf(const char *buf, size_t buf_size) {
    int i;
    for (i = 0; i < buf_size; i++) {
        printf("%hhu.", buf[i]);
        if (buf[i] == '\0') {
            putchar('\n');
            return;
        }
    }
    putchar('\n');
}

static size_t acp_packlen(const char *buf, size_t buf_size) {
    int i, state = 0;
    size_t n = 0;
    for (i = 0; i < buf_size; i++) {
        switch (state) {
            case 0:
                if (buf[i] == ACP_DELIMITER_BLOCK) {
                    state = 1;
                }
                n++;
                break;
            case 1:
                if (buf[i] == ACP_DELIMITER_BLOCK) {
                    state = 2;
                }
                n++;
                break;
            case 2:
                if (buf[i] == ACP_DELIMITER_BLOCK) {
                    state = 3;
                }
                n++;
                break;
            case 3:
                state = 4;
                n++;
                break;
            case 4:
                n++;
                return n;
        }

    }
    return 0;
}

static int acp_crcCheck(const char * buf, size_t buf_size) {
    uint8_t crc, crc_fact = 0;
    char state = 0;
    int i, found = 0, block_count = 0;
    for (i = 0; i < buf_size; i++) {
        switch (state) {
            case 0:
                if (buf[i] == ACP_DELIMITER_BLOCK) {
                    block_count++;
                }
                if (block_count == ACP_BLOCK_IND_CRC) {
                    state = 1;
                }
                crc_update(&crc_fact, buf[i]);
                break;
            case 1:
                crc = buf[i];
                found = 1;
                break;
        }
        if (found) {
            break;
        }
    }
    if (found && crc == crc_fact) {
        return 1;
    } else {
        printde("crc_found: %d, crc: %hhu, crc_fact: %hhu\n", found, crc, crc_fact);
        return 0;
    }
}

static void acp_requestSetNewId(ACPRequest * item) {
    clock_t t = clock();
    srand((unsigned int) t);
    item->id = (unsigned int) rand();
}

static int acp_read(char *buf, size_t buf_size, Peer *peer) {
    ssize_t n = recvfrom(*peer->fd, buf, buf_size, 0, (struct sockaddr*) (&(peer->addr)), &(peer->addr_size));
    if (n < 0) {
        perrord("recvfrom()");
        return 0;
    }
#ifdef MODE_VERBOSE
    printf("%s(): count:%d dump:\n", __func__, n);
    acp_dumpBuf(buf, buf_size);
    puts(buf);
#endif
    return acp_crcCheck(buf, buf_size);
}

static int acp_responseParse(ACPResponse *item) {
    if (strlen(item->buf) < ACP_RESPONSE_BUF_SIZE_MIN) {
        putsde("data shortage\n");
        return 0;
    }
    int block_count = 0;
    size_t j = 0;
    int f1 = 0;
    char seq_str[ACP_NUM_STR_LENGTH];
    char id_str[ACP_NUM_STR_LENGTH];
    char inl_str[ACP_NUM_STR_LENGTH];
    memset(seq_str, 0, sizeof seq_str);
    memset(id_str, 0, sizeof id_str);
    memset(inl_str, 0, sizeof inl_str);
    for (size_t i = 0; i < item->buf_size; i++) {
        if (item->buf[i] == ACP_DELIMITER_BLOCK) {
            block_count++;
            j = 0;
            continue;
        }
        if (block_count == 0) {
            if (item->buf[i] == ACP_DELIMITER_COLUMN) {
                f1 = 1;
                j = 0;
                continue;
            }
            if (f1) {
                if (!isdigit((int) item->buf[i])) {
                    printde("parsing is_not_last: expected digit but %hhu found \n", item->buf[i]);
                    return 0;
                }
                inl_str[j] = item->buf[i];
            } else {
                if (!isdigit((int) item->buf[i])) {
                    printde("parsing seq: expected digit but %hhu found \n", item->buf[i]);
                    return 0;
                }
                seq_str[j] = item->buf[i];
            }
            j++;
            continue;
        }
        if (block_count == 1) {
            if (j < item->data_size) {
                item->data[j] = item->buf[i];
                j++;
            } else {
                putsde("data buffer overflow\n");
                return 0;
            }
            continue;
        }
        if (block_count == 2) {
            if (!isdigit((int) item->buf[i])) {
                printde("parsing id: expected digit but %hhu found \n", item->buf[i]);
                return 0;
            }
            id_str[j] = item->buf[i];
            j++;
            continue;
        }
        if (block_count > 2) {
            break;
        }
    }
    if (strlen(seq_str) <= 0) {
        putsde("seq not found\n");
        return 0;
    }
    if (strlen(inl_str) <= 0) {
        putsde("is_not_last not found\n");
        return 0;
    }
    if (strlen(id_str) <= 0) {
        putsde("id not found\n");
        return 0;
    }
    item->seq = atoi(seq_str);
    item->is_not_last = atoi(inl_str);
    item->id = atoi(id_str);
    return 1;
}

static void acp_countDataRows(ACPRequest *item) {
    item->data_rows_count = 0;
    for (size_t i=0;i<item->data_size;i++) {
        if(item->data[i]==ACP_DELIMITER_ROW){
            item->data_rows_count++;
        }
        if(item->data[i]==0){
            break;
        }
    }
}

static int acp_requestParse(ACPRequest *item) {
    if (strlen(item->buf) < ACP_REQUEST_BUF_SIZE_MIN) {
        putsde("data shortage\n");
        return 0;
    }
    int block_count = 0;
    size_t j = 0;
    char id_str[ACP_NUM_STR_LENGTH];
    memset(id_str, 0, sizeof id_str);
    for (size_t i = 0; i < item->buf_size; i++) {
        if (item->buf[i] == ACP_DELIMITER_BLOCK) {
            block_count++;
            j = 0;
            continue;
        }
        if (block_count == 0) {
            if (j < item->cmd_size) {
                item->cmd[j] = item->buf[i];
                j++;
            } else {
                putsde("cmd buffer overflow\n");
                return 0;
            }
            continue;
        }
        if (block_count == 1) {
            if (j < item->data_size) {
                item->data[j] = item->buf[i];
                j++;
            } else {
                putsde("data buffer overflow\n");
                return 0;
            }
            continue;
        }
        if (block_count == 2) {
            if (!isdigit((int) item->buf[i])) {
                printde("parsing id: expected digit but %hhu found \n", item->buf[i]);
                return 0;
            }
            id_str[j] = item->buf[i];
            j++;
            continue;
        }
        if (block_count > 2) {
            break;
        }
    }
    if (strlen(id_str) <= 0) {
        putsde("id not found\n");
    }
    acp_countDataRows(item);
    item->id = atoi(id_str);
    return 1;
}

static void acp_bufnrow(char **v) {
    char *c;
    c = strchr(*v, ACP_DELIMITER_ROW);
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if (**v == ACP_DELIMITER_BLOCK) {
            *v = strchr(*v, '\0');
        }
    }
}

static void acp_bufToData(char **v) {
    char *c;
    c = strchr(*v, ACP_DELIMITER_BLOCK);
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if (**v == ACP_DELIMITER_BLOCK) {
            *v = strchr(*v, '\0');
        }
    }
}

static int acp_sendBuf(const char *buf, size_t buf_size, Peer *peer) {
    size_t sz = acp_packlen(buf, buf_size);
#ifdef MODE_VERBOSE
    fprintf(stdout, "%s(): we will send: %u bytes to %s at %s %d\n", __func__, sz, peer->id, peer->addr_str, peer->port);
#endif
    return sendBuf((void *) buf, sz, *(peer->fd), (struct sockaddr *) (&peer->addr), peer->addr_size);
}

int acp_responseStrCat(ACPResponse *item, const char *str) {
    if (strlen(item->data) + strlen(str) + 1 >= item->data_size) {
        putsde("buffer overflow\n");
        return 0;
    }
    strcat(item->data, str);
    return 1;
}

int acp_requestStrCat(ACPRequest *item, const char *str) {
    if (strlen(item->data) + strlen(str) + 1 >= item->data_size) {
        putsde("buffer overflow\n");
        return 0;
    }
    strcat(item->data, str);
    return 1;
}

void acp_responseInit(ACPResponse *item) {
    memset(item->buf, 0, sizeof item->buf);
    memset(item->data, 0, sizeof item->data);
    item->data_size = ACP_DATA_MAX_SIZE;
    item->buf_size = ACP_BUFFER_MAX_SIZE;
    item->crc = 0;
    item->is_not_last = 0;
    item->last_is_ok = 1;
    item->seq = 0;
}

void acp_requestInit(ACPRequest *item) {
    memset(item->buf, 0, sizeof item->buf);
    memset(item->data, 0, sizeof item->data);
    memset(item->cmd, 0, sizeof item->cmd);
    item->cmd_size = ACP_COMMAND_MAX_SIZE;
    item->data_size = ACP_DATA_MAX_SIZE;
    item->buf_size = ACP_BUFFER_MAX_SIZE;
    item->crc = 0;
}

int acp_responseCoopRequest(ACPResponse *response, ACPRequest *request) {
    response->id = request->id;
    return 0;
}

int acp_requestRead(ACPRequest *item, Peer *peer) {
    if (!acp_read(item->buf, item->buf_size, peer)) {
        return 0;
    }
    return acp_requestParse(item);
}

int acp_responseRead(ACPResponse *item, Peer *peer) {
    if (!acp_read(item->buf, item->buf_size, peer)) {
        return 0;
    }
    acp_responseParse(item);
    return 1;
}

int acp_requestCheck(ACPRequest *item) {
    return 1;
}

int acp_responseCheck(ACPResponse *response, ACPRequest *request) {
    if (response->id != request->id) {
        printde("unexpected id (response_id:%u, request_id:%u\n", response->id, request->id);
        return 0;
    }
    return 1;
}

static int acp_dataToI(char *buf, int *item) {
    if (sscanf(buf, "%d", item) != 1) {
        return 0;
    }
    return 1;
}

static int acp_dataToF(char *buf, float *item) {
    if (sscanf(buf, "%f", item) != 1) {
        return 0;
    }
    return 1;
}

static void acp_dataToI1List(char *buf, I1List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        int p0;
        if (sscanf(buff, "%d", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(I1List)

static void acp_dataToI2List(char *buf, I2List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        int p0, p1;
        if (sscanf(buff, "%d" ACP_DELIMITER_COLUMN_STR "%d", &p0, &p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(I2List)

static void acp_dataToI3List(char *buf, I3List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        int p0, p1, p2;
        if (sscanf(buff, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d", &p0, &p1, &p2) != 3) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->item[list->length].p2 = p2;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(I3List)

static void acp_dataToF1List(char *buf, F1List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        float p0;
        if (sscanf(buff, "%f", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(F1List)

static void acp_dataToI1F1List(char *buf, I1F1List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        int p0;
        float p1;
        if (sscanf(buff, "%d" ACP_DELIMITER_COLUMN_STR "%f", &p0, &p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(I1F1List)

static void acp_dataToS1List(char *buf, S1List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        char p0[LINE_SIZE];
        memset(p0, 0, sizeof p0);
        size_t i = 0;
        size_t j = 0;
        int f = 0;
        for (i = 0; i < LINE_SIZE; i++) {
            if (buff[0] == '\0') {
                f = 1;
                return;
            }
            if (buff[0] == ACP_DELIMITER_ROW) {
                break;
            }
            p0[j] = buff[0];
            j++;
            buff = &buff[1];
        }
        strcpy(&list->item[list->length * LINE_SIZE], p0);
        list->length++;
        if (f) {
            return;
        }
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(S1List)

static void acp_dataToI1S1List(char *buf, I1S1List *list) {
    char *buff = buf;
    list->length = 0;
    char format[LINE_SIZE];
    int n = sprintf(format, "%%d%c%%%lus", ACP_DELIMITER_COLUMN, LINE_SIZE);
    if (n <= 0) {
        return;
    }
    while (list->length < list->max_length) {
        int p0;
        char p1[LINE_SIZE];
        memset(p1, 0, sizeof p1);
        if (sscanf(buff, format, &p0, p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        strcpy(list->item[list->length].p1, p1);
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(I1S1List)

static void acp_dataToFTSList(char *buf, FTSList *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        int id, state;
        float temp;
        struct timespec tm;
        if (sscanf(buff, "%d" ACP_DELIMITER_COLUMN_STR "%f" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d", &id, &temp, &tm.tv_sec, &tm.tv_nsec, &state) != 5) {
            break;
        }
        list->item[list->length].id = id;
        list->item[list->length].value = temp;
        list->item[list->length].tm.tv_sec = tm.tv_sec;
        list->item[list->length].tm.tv_nsec = tm.tv_nsec;
        list->item[list->length].state = state;
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(FTSList)

static void acp_dataToS2List(char *buf, S2List *list) {
    char *buff = buf;
    list->length = 0;
    while (list->length < list->max_length) {
        char p0[LINE_SIZE];
        char p1[LINE_SIZE];
        memset(p0, 0, sizeof p0);
        memset(p1, 0, sizeof p1);
        size_t i = 0;
        size_t j = 0;
        int f = 0;
        for (i = 0; i < LINE_SIZE; i++) {
            if (buff[0] == ACP_DELIMITER_COLUMN) {
                buff = &buff[1];
                break;
            }
            if (buff[0] == '\0' || buff[0] == ACP_DELIMITER_ROW) {
                f = 1;
                break;
            }
            p0[j] = buff[0];
            j++;
            buff = &buff[1];
        }
        if (f) {
            break;
        }
        j = 0;
        for (i = 0; i < LINE_SIZE; i++) {
            if (buff[0] == ACP_DELIMITER_COLUMN) {
                buff = &buff[1];
                break;
            }
            if (buff[0] == '\0' || buff[0] == ACP_DELIMITER_ROW) {
                break;
            }
            p1[j] = buff[0];
            j++;
            buff = &buff[1];
        }
        strcpy(list->item[list->length].p0, p0);
        strcpy(list->item[list->length].p1, p1);
        list->length++;
        acp_bufnrow(&buff);
    }
}

FUN_ACP_REQUEST_DATA_TO(S2List)

void acp_requestSetCmd(ACPRequest * item, const char *cmd) {
    strncpy(item->cmd, cmd, item->cmd_size);
}

void acp_responsePack(ACPResponse *item) {
    snprintf(item->buf, item->buf_size, "%u" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_BLOCK_STR "%s" ACP_DELIMITER_BLOCK_STR "%u" ACP_DELIMITER_BLOCK_STR, item->seq, item->is_not_last, item->data, item->id);
    crc_update_by_str(&item->crc, item->buf);
    char crc_str[] = {item->crc, '\0'};
    strcat(item->buf, crc_str);
#ifdef MODE_VERBOSE
    fprintf(stdout, "response is: \n %u_%d_BLOCK_%s_BLOCK_%u_BLOCK_%hhu\n", item->seq, item->is_not_last, item->data, item->id, item->crc);
#endif
}

void acp_requestPack(ACPRequest *item) {
    acp_requestSetNewId(item);
    snprintf(item->buf, item->buf_size, "%s" ACP_DELIMITER_BLOCK_STR "%s" ACP_DELIMITER_BLOCK_STR "%u" ACP_DELIMITER_BLOCK_STR, item->cmd, item->data, item->id);
    crc_update_by_str(&item->crc, item->buf);
    char crc_str[] = {item->crc, '\0'};
    strcat(item->buf, crc_str);
#ifdef MODE_VERBOSE
    fprintf(stdout, "request is: \n %s_BLOCK_%s_BLOCK_%u_BLOCK_%hhu\n", item->cmd, item->data, item->id, item->crc);
#endif
}

int acp_responseSend(ACPResponse *response, Peer *peer) {
    acp_responsePack(response);
    return acp_sendBuf(response->buf, response->buf_size, peer);
}

int acp_requestSend(ACPRequest *request, Peer *peer) {
    acp_requestPack(request);
    return acp_sendBuf(request->buf, request->buf_size, peer);
}

int acp_requestSendCmd(const char *cmd, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    return acp_requestSend(request, peer);
}

int acp_requestSendUnrequitedCmd(const char *cmd, Peer *peer) {
    ACPRequest request;
    return acp_requestSendCmd(cmd, &request, peer);
}

int acp_requestSendI1List(char *cmd, const I1List *data, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    for (int i = 0; i < data->length; i++) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_ROW_STR, data->item[i]);
        if (!acp_requestStrCat(request, q)) {
            return 0;
        }
    }
    if (acp_requestSend(request, peer) < 0) {
        return 0;
    }
    return 1;
}

int acp_requestSendI1F1List(char *cmd, const I1F1List *data, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    for (int i = 0; i < data->length; i++) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR ACP_FLOAT_FORMAT ACP_DELIMITER_ROW_STR, data->item[i].p0, data->item[i].p1);
        if (!acp_requestStrCat(request, q)) {
            return 0;
        }
    }
    if (acp_requestSend(request, peer) < 0) {
        return 0;
    }
    return 1;
}

int acp_requestSendI2List(char *cmd, const I2List *data, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    for (int i = 0; i < data->length; i++) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, data->item[i].p0, data->item[i].p1);
        if (!acp_requestStrCat(request, q)) {
            return 0;
        }
    }
    if (acp_requestSend(request, peer) < 0) {
        return 0;
    }
    return 1;
}

int acp_requestSendS2List(char *cmd, const S2List *data, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    for (int i = 0; i < data->length; i++) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_ROW_STR, data->item[i].p0, data->item[i].p1);
        if (!acp_requestStrCat(request, q)) {
            return 0;
        }
    }
    if (acp_requestSend(request, peer) < 0) {
        return 0;
    }
    return 1;
}

int acp_requestSendS1List(char *cmd, const S1List *data, ACPRequest *request, Peer *peer) {
    acp_requestInit(request);
    acp_requestSetCmd(request, cmd);
    for (int i = 0; i < data->length; i++) {
        char q[LINE_SIZE];
        snprintf(q, sizeof q, "%s" ACP_DELIMITER_ROW_STR, &data->item[i * LINE_SIZE]);
        if (!acp_requestStrCat(request, q)) {
            return 0;
        }
    }
    if (acp_requestSend(request, peer) < 0) {
        return 0;
    }
    return 1;
}

int acp_requestSendUnrequitedI1List(char *cmd, const I1List *data, Peer *peer) {
    ACPRequest request;
    return acp_requestSendI1List(cmd, data, &request, peer);
}

int acp_requestSendUnrequitedI1F1List(char *cmd, const I1F1List *data, Peer *peer) {
    ACPRequest request;
    return acp_requestSendI1F1List(cmd, data, &request, peer);
}

int acp_requestSendUnrequitedI2List(char *cmd, const I2List *data, Peer *peer) {
    ACPRequest request;
    return acp_requestSendI2List(cmd, data, &request, peer);
}

int acp_requestSendUnrequitedS2List(char *cmd, const S2List *data, Peer *peer) {
    ACPRequest request;
    return acp_requestSendS2List(cmd, data, &request, peer);
}

int acp_requestSendUnrequitedS1List(char *cmd, const S1List *data, Peer *peer) {
    ACPRequest request;
    return acp_requestSendS1List(cmd, data, &request, peer);
}

void acp_responseSendStr(const char *s, int is_not_last, ACPResponse *response, Peer *peer) {
    if (!response->last_is_ok) {
        return;
    }
    response->is_not_last = is_not_last;
    strncpy(response->data, s, response->data_size);
    response->crc = 0;
    int n = acp_responseSend(response, peer);
    if (n > 0) {
        response->seq++;
        response->last_is_ok = 1;
    } else {
        response->last_is_ok = 0;
    }
}

FUN_ACP_RESPONSE_READ(I1List)
FUN_ACP_RESPONSE_READ(I2List)
FUN_ACP_RESPONSE_READ(I1F1List)
FUN_ACP_RESPONSE_READ(FTSList)
int acp_setEMOutput(EM *em, int output) {
    if (output == ((int) em->last_output)) {
        return 0;
    }
    I2 di[1];
    di[0].p0 = em->remote_id;
    di[0].p1 = output;
    I2List data = {di, 1, 1};
    if (!acp_requestSendUnrequitedI2List(ACP_CMD_SET_INT, &data, &em->peer)) {
        printde("failed to send request where em.id = %d\n", em->id);
        return 0;
    }
    em->last_output = (float) output;
    return 1;
}

int acp_setEMDutyCycle(EM *em, float output) {
    if (output == em->last_output) {
        return 0;
    }
    I2 di[1];
    di[0].p0 = em->remote_id;
    di[0].p1 = (int) output;
    I2List data = {di, 1, 1};
    if (!acp_requestSendUnrequitedI2List(ACP_CMD_SET_PWM_DUTY_CYCLE, &data, &em->peer)) {
        printde("failed to send request where em.id = %d\n", em->id);
        return 0;
    }
    em->last_output = output;
    return 1;

}

int acp_setEMOutputR(EM *em, int output) {
    I2 di[1];
    di[0].p0 = em->remote_id;
    di[0].p1 = output;
    I2List data = {di, 1, 1};
    if (!acp_requestSendUnrequitedI2List(ACP_CMD_SET_INT, &data, &em->peer)) {
        printde("failed to send request where em.id = %d\n", em->id);
        return 0;
    }
    em->last_output = (float) output;
    return 1;
}

int acp_setEMDutyCycleR(EM *em, float output) {
    I2 di[1];
    di[0].p0 = em->remote_id;
    di[0].p1 = (int) output;
    I2List data = {di, 1, 1};
    if (!acp_requestSendUnrequitedI2List(ACP_CMD_SET_PWM_DUTY_CYCLE, &data, &em->peer)) {
        printde("failed to send request where em.id = %d\n", em->id);
        return 0;
    }
    em->last_output = output;
    return 1;
}

int acp_readSensorInt(SensorInt *s) {
    struct timespec now = getCurrentTime();
    /*
                if (!timeHasPassed(s->interval_min, s->last_read_time, now)) {
                         
                        
                    return s->last_return;
                }
     */

    s->peer.active = 0;
    s->peer.time1 = now;
    s->last_read_time = now;
    s->last_return = 0;

    int di[1];
    di[0] = s->remote_id;
    I1List data = {di, 1, 1};
    ACPRequest request;
    if (!acp_requestSendI1List(ACP_CMD_GET_INT, &data, &request, &s->peer)) {
        printde("acp_requestSendI1List failed where sensor.id = %d\n", s->id);
        return 0;
    }

    //waiting for response...
    I2 td[1];
    I2List tl = {td, 0, 1};

    memset(&td, 0, sizeof tl);
    tl.length = 0;
    if (!acp_responseReadI2List(&tl, &request, &s->peer)) {
        printde("acp_responseReadI2List() error where sensor.id = %d\n", s->id);
        return 0;
    }
    s->peer.active = 1;
    if (tl.length != 1) {
        printde("response: number of items = %d but 1 expected\n", tl.length);
        return 0;
    }
    if (tl.item[0].p0 == s->remote_id) {
        printde("response:  peer returned id=%d but requested one was %d\n", tl.item[0].p0, s->remote_id);
        return 0;
    }
    s->peer.active = 1;
    s->value = tl.item[0].p1;
    s->last_return = 1;
    return 1;
}

int acp_readSensorFTS(SensorFTS *s) {
    struct timespec now = getCurrentTime();
    /*
                if (!timeHasPassed(s->interval_min, s->last_read_time, now)) {
                    return s->last_return;
                }
     */

    s->peer.active = 0;
    s->peer.time1 = now;
    s->last_read_time = now;
    s->last_return = 0;
    s->value.state = 0;

    int di[1];
    di[0] = s->remote_id;
    I1List data = {di, 1, 1};
    ACPRequest request;
    if (!acp_requestSendI1List(ACP_CMD_GET_FTS, &data, &request, &s->peer)) {
        printde("acp_requestSendI1List failed where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
        return 0;
    }

    //waiting for response...
    FTS td[1];
    FTSList tl = {td, 0, 1};

    memset(&td, 0, sizeof tl);
    tl.length = 0;
    if (!acp_responseReadFTSList(&tl, &request, &s->peer)) {
        printde("acp_responseReadFTSList() error where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
        return 0;
    }
    s->peer.active = 1;
    if (tl.length != 1) {
        printde("response: number of items = %d but 1 expected\n", tl.length);
        return 0;
    }
    if (tl.item[0].id != s->remote_id) {
        printde("response: peer returned id=%d but requested one was %d\n", tl.item[0].id, s->remote_id);
        return 0;
    }
    if (tl.item[0].state != 1) {
        printde("response: FTS state is bad where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
        return 0;
    }
    s->value = tl.item[0];
    s->last_return = 1;
    return 1;
}

int acp_getFTS(FTS *output, Peer *peer, int remote_id) {

    struct timespec now = getCurrentTime();
    peer->active = 0;
    peer->time1 = now;

    int di[1];
    di[0] = remote_id;
    I1List data = {di, 1, 1};
    ACPRequest request;
    if (!acp_requestSendI1List(ACP_CMD_GET_FTS, &data, &request, peer)) {
        printde("send failed where remote_id=%d\n", remote_id);
        return 0;
    }

    //waiting for response...
    FTS td[1];
    FTSList tl = {td, 0, 1};

    memset(&td, 0, sizeof tl);
    tl.length = 0;
    if (!acp_responseReadFTSList(&tl, &request, peer)) {
        printde("read failed where remote_id=%d\n", remote_id);
        return 0;
    }
    peer->active = 1;
    if (tl.length != 1) {
        printde("response: number of items = %d but 1 expected\n", tl.length != 1);
        return 0;
    }
    if (tl.item[0].id != remote_id) {
        printde("response: peer returned id=%d but requested one was %d\n", tl.item[0].id, remote_id);
        return 0;
    }
    if (tl.item[0].state != 1) {
        printde("response: FTS state is bad where remote_id=%d\n", remote_id);
        peer->active = 1;
        return 0;
    }
    peer->active = 1;
    *output = tl.item[0];

    return 1;
}

void acp_pingPeer(Peer *item) {

    item->active = 0;
    item->time1 = getCurrentTime();
    ACPRequest request;
    if (!acp_requestSendCmd(ACP_CMD_APP_PING, &request, item)) {
        putsde("acp_requestSendCmd failed\n");
        return;
    }
    //waiting for response...
    ACP_RESPONSE_CREATE
    if (!acp_responseRead(&response, item)) {
        return;
    }
    if (!acp_responseCheck(&response, &request)) {
        return;
    }
    char *b = response.data;
    char resp[] = {'\0', '\0'};
    resp[0] = b[0];
    if (strncmp(resp, ACP_RESP_APP_BUSY, 1) != 0) {
        putsde("peer is not busy\n");
        return;
    }
    item->active = 1;


}

void acp_pingPeerList(PeerList *list, struct timespec interval, struct timespec now) {
    int i;
    FORL{
        if (timeHasPassed(interval, LIi.time1, now)) {
            acp_pingPeer(&LIi);
        }
    }
}

int acp_responseSendCurTime(ACPResponse *item, Peer *peer) {
    struct tm *current;
    time_t now;
    time(&now);
    current = localtime(&now);
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            current->tm_year,
            current->tm_mon,
            current->tm_mday,
            current->tm_hour,
            current->tm_min,
            current->tm_sec
            );
    if (!acp_responseStrCat(item, q)) {
        return 0;
    }
    if (!acp_responseSend(item, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendCmdGetInt(Peer *peer, char* cmd, int *output) {

    peer->active = 0;
    peer->time1 = getCurrentTime();
    ACPRequest request;
    if (!acp_requestSendCmd(cmd, &request, peer)) {
        putsde("acp_requestSendCmd failed\n");
        return 0;
    }
    //waiting for response...
    ACP_RESPONSE_CREATE
    if (!acp_responseRead(&response, peer)) {

        return 0;
    }
    peer->active = 1;

    if (!acp_responseCheck(&response, &request)) {
        return 0;
    }

    if (!acp_dataToI(response.data, output)) {
        putsde("acp_dataToI() failed\n");
        return 0;
    }
    return 1;
}

int acp_sendCmdGetFloat(Peer *peer, char* cmd, float *output) {

    peer->active = 0;
    peer->time1 = getCurrentTime();
    ACPRequest request;
    if (!acp_requestSendCmd(cmd, &request, peer)) {
        putsde("acp_requestSendCmd failed\n");
        return 0;
    }
    //waiting for response...
    ACP_RESPONSE_CREATE
    if (!acp_responseRead(&response, peer)) {

        return 0;
    }


    if (!acp_responseCheck(&response, &request)) {
        return 0;
    }
    peer->active = 1;
    if (!acp_dataToF(response.data, output)) {
        putsde("acp_dataToI() failed\n");
        return 0;
    }
    return 1;


}

int acp_responseFTSCat(int id, float value, struct timespec tm, int state, ACPResponse *response) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR ACP_FLOAT_FORMAT ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, id, value, tm.tv_sec, tm.tv_nsec, state);
    return acp_responseStrCat(response, q);
}

int acp_responseITSCat(int id, int value, struct timespec tm, int state, ACPResponse *response) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, id, value, tm.tv_sec, tm.tv_nsec, state);
    return acp_responseStrCat(response, q);
}

void freePeer(PeerList *list) {
    free(list->item);
    list->item = NULL;
    list->length = 0;
}

int acp_cmdcmp(ACPRequest *request, char * cmd) {
    int i = 0;
    size_t l = strlen(cmd);
    while (i < l && i < request->buf_size) {
        if (request->buf[i] != cmd[i]) {
            return 0;
        }
        i++;
    }
    if (i == 0) {
        return 0;
    }
    if (request->buf[i] == ACP_DELIMITER_BLOCK) {
        return 1;
    }
    return 0;
}

void acp_printI1(I1List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I1List dump");
    puts("+-----------+");
    puts("|   item    |");
    puts("+-----------+");
    for (i = 0; i < list->length; i++) {

        snprintf(q, sizeof q, "|%11d|", list->item[i]);
        puts(q);
    }
    puts("+-----------+");
}

void acp_printI2(I2List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I2List dump");
    puts("+-----------+-----------+");
    puts("|     p0    |     p1    |");
    puts("+-----------+-----------+");
    for (i = 0; i < list->length; i++) {

        snprintf(q, sizeof q, "|%11d|%11d|", list->item[i].p0, list->item[i].p1);
        puts(q);
    }
    puts("+-----------+-----------+");
}

void acp_printI3(I3List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I3List dump");
    puts("+-----------+-----------+-----------+");
    puts("|     p0    |     p1    |     p2    |");
    puts("+-----------+-----------+-----------+");
    for (i = 0; i < list->length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%11d|", list->item[i].p0, list->item[i].p1, list->item[i].p2);
        puts(q);
    }
    puts("+-----------+-----------+-----------+");
}

void acp_sendPeerListInfo(PeerList *pl, ACPResponse *response, Peer *peer) {
    char q[LINE_SIZE];
    ACP_SEND_STR("+--------------------------------------------------------------------+\n")
    ACP_SEND_STR("|                               Peer                                 |\n")
    ACP_SEND_STR("+-----------+---------------+-----------+-----------+----------------+\n")
    ACP_SEND_STR("|    id     |    address    |   port    | sin_port  |     s_addr     |\n")
    ACP_SEND_STR("+-----------+---------------+-----------+-----------+----------------+\n")
    for (int i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%11s|%15s|%11d|%11u|%16u|\n",
                pl->item[i].id,
                pl->item[i].addr_str,
                pl->item[i].port,
                pl->item[i].addr.sin_port,
                pl->item[i].addr.sin_addr.s_addr
                );
        ACP_SEND_STR(q)
    }
    ACP_SEND_STR("+-----------+---------------+-----------+-----------+----------------+\n")
}



