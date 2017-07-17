
#include "main.h"

FUN_LIST_GET_BY_IDSTR(Peer)

FUN_LIST_GET_BY_ID(SensorFTS)

FUN_LIST_GET_BY_ID(EM)

void acp_bufnrow(char **v) {
    char *c;
    c = strchr(*v, ACP_DELIMITER_ROW);
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if (**v == ACP_DELIMITER_CRC) {
            *v = strchr(*v, '\0');
        }
    }
}

void acp_bufToData(char **v) {
    char *c;
    c = strchr(*v, ACP_DELIMITER_CMD);
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if (**v == ACP_DELIMITER_CRC) {
            *v = strchr(*v, '\0');
        }
    }
}

int acp_initBuf(char *buf, size_t buf_size) {
    if (buf_size < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
        fputs("acp_initBuf: not enough space buf\n", stderr);
#endif
        return 0;
    }
    memset(buf, 0, buf_size);
    buf[0] = ACP_CMD_APP_NO;
    buf[1] = ACP_CMD_APP_NO;
    buf[2] = ACP_DELIMITER_CMD;
    return 1;
}

void acp_parsePackI1(char *buf, I1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
        int p0;
        if (sscanf(buff, "%d", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        acp_bufnrow(&buff);
    }
}

void acp_parsePackI2(char *buf, I2List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

void acp_parsePackI3(char *buf, I3List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

void acp_parsePackF1(char *buf, F1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
        float p0;
        if (sscanf(buff, "%f", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        acp_bufnrow(&buff);
    }
}

void acp_parsePackI1F1(char *buf, I1F1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

void acp_parsePackS1(char *buf, S1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

void acp_parsePackI1S1(char *buf, I1S1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    char format[LINE_SIZE];
    int n = sprintf(format, "%%d%c%%%lus", ACP_DELIMITER_COLUMN, LINE_SIZE);
    if (n <= 0) {
        return;
    }
    while (list->length < list_max_size) {
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

void acp_parsePackFTS(char *buf, FTSList *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

void acp_parsePackS2(char *buf, S2List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    acp_bufToData(&buff);
    while (list->length < list_max_size) {
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

size_t acp_packlen(char *buf, size_t buf_size) {
    int i, state = 0;
    size_t n = 0;
    for (i = 0; i < buf_size; i++) {
        switch (state) {
            case 0:
                if (buf[i] == ACP_DELIMITER_CRC) {
                    state = 1;
                }
                n++;
                break;
            case 1://crc
                n++;
                state = 2;
                break;
            case 2:
                n++;
                if (buf[i] == ACP_DELIMITER_PACKET) {
                    return n;
                }
                return 0;
        }

    }
    return 0;
}

//requires buf after acp_initBuf()

int acp_bufAddHeader(char *buf, char qnf, char *cmd_str, size_t buf_size) {
    if (buf_size < ACP_HEADER_LENGTH) {
        return 0;
    }
    buf[0] = qnf;
    buf[1] = cmd_str[0];
    buf[2] = ACP_DELIMITER_CMD;
    return 1;
}

int acp_crc_check(const char * buf, size_t buf_size) {
    uint8_t crc, crc_fact = 0;
    char state = 0;
    int i, found = 0;
    for (i = 0; i < buf_size; i++) {
        switch (state) {
            case 0:
                if (buf[i] == ACP_DELIMITER_CRC) {
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
#ifdef MODE_DEBUG
        fprintf(stderr, "crc_found: %d, crc: %hhd, crc_fact: %hhd\n", found, crc, crc_fact);
#endif
        return 0;
    }
}

int acp_bufAddFooter(char *buf, size_t buf_size) {
    uint8_t crc = 0;
    size_t pack_len;
    pack_len = strlen(buf) + 3;
    if (pack_len > buf_size) {
#ifdef MODE_DEBUG
        fputs("acp_bufAddFooter: not enough space for two chars in buf\n", stderr);
#endif
        return 0;
    }
    buf[pack_len - 3] = ACP_DELIMITER_CRC;
    crc_update_by_str(&crc, buf);
    buf[pack_len - 2] = crc;
    buf[pack_len - 1] = ACP_DELIMITER_PACKET;
    return 1;
}

int acp_sendBuf(char *buf, Peer *peer) {
    size_t sz;
    sz = acp_packlen(buf, peer->sock_buf_size);
    return sendBuf((void *) buf, sz, *(peer->fd), (struct sockaddr *) (&peer->addr), peer->addr_size);
}

void acp_dumpBuf(const char *buf, size_t buf_size) {
    int i, state = 0;
    for (i = 0; i < buf_size; i++) {
        printf("%hhu.", buf[i]);
        switch (state) {
            case 0:
                if (buf[i] == ACP_DELIMITER_CRC) {
                    state = 1;
                }
                break;
            case 1:
                state = 2;
                break;
            case 2:
                putchar('\n');
                return;
        }

    }
}

int acp_sendBufArrPackI1List(char cmd, const I1List *data, Peer *peer) {
    char q[LINE_SIZE], buf[peer->sock_buf_size];
    int i;
    if (!acp_initBuf(buf, peer->sock_buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, peer->sock_buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_ROW_STR, data->item[i]);
        if (bufCat(buf, q, peer->sock_buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendBufArrPackI2List(char cmd, const I2List *data, Peer *peer) {
    char q[LINE_SIZE], buf[peer->sock_buf_size];
    int i;
    if (!acp_initBuf(buf, peer->sock_buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, peer->sock_buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, data->item[i].p0, data->item[i].p1);
        if (bufCat(buf, q, peer->sock_buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendBufArrPackS2List(char cmd, const S2List *data, Peer *peer) {
    char q[LINE_SIZE], buf[peer->sock_buf_size];
    int i;
    if (!acp_initBuf(buf, peer->sock_buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, peer->sock_buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_ROW_STR, data->item[i].p0, data->item[i].p1);
        if (bufCat(buf, q, peer->sock_buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendBufArrPackS1List(char cmd, const S1List *data, Peer *peer) {
    char q[LINE_SIZE], buf[peer->sock_buf_size];
    int i;
    if (!acp_initBuf(buf, peer->sock_buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, peer->sock_buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%s" ACP_DELIMITER_ROW_STR, &data->item[i * LINE_SIZE]);
        if (bufCat(buf, q, peer->sock_buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

void acp_sendStr(const char *s, uint8_t *crc, Peer *peer) {
    if (sendto(*(peer->fd), s, strlen(s), 0, (struct sockaddr*) (&peer->addr), peer->addr_size) < 0) {
#ifdef MODE_DEBUG

        perror("sendStr: error writing to socket");
#endif
    }
    crc_update_by_str(crc, s);
}

void acp_sendFooter(int8_t crc, Peer *peer) {
    char s[] = "\n\n \n";
    s[2] = crc;
    if (sendto(*(peer->fd), s, strlen(s), 0, (struct sockaddr*) (&peer->addr), peer->addr_size) < 0) {
#ifdef MODE_DEBUG

        perror("sendFooter: error writing to socket");
#endif
    }
}

int acp_sendBufPack(char *buf, char qnf, char *cmd_str, Peer *peer) {
    if (!acp_bufAddHeader(buf, qnf, cmd_str, peer->sock_buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendBufPack: acp_bufAddHeader() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendBufPack: acp_bufAddFooter() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendStrPack(char qnf, char *cmd, Peer *peer) {
    char buf[peer->sock_buf_size];
    memset(buf, 0, sizeof buf);
    if (!acp_bufAddHeader(buf, qnf, cmd, peer->sock_buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendStrPack: acp_bufAddHeader() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_bufAddFooter(buf, peer->sock_buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendStrPack: acp_bufAddFooter() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_sendBuf(buf, peer)) {
        return 0;
    }
    return 1;
}

int acp_bufCatDate(struct tm *date, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            date->tm_year,
            date->tm_mon,
            date->tm_mday,
            date->tm_hour,
            date->tm_min,
            date->tm_sec
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int acp_recvOK(Peer *peer) {
    char buf[peer->sock_buf_size];
    if (!clientRead(*(peer->fd), buf, sizeof buf)) {
        return 0;
    }
    if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvOK: acp_crc_check() failed\n", stderr);
#endif
        return 0;
    }
    if (strncmp(buf + 1, ACP_RESP_REQUEST_SUCCEEDED, sizeof ACP_RESP_REQUEST_SUCCEEDED - 1) != 0) {
#ifdef MODE_DEBUG

        fprintf(stderr, "acp_recvOK: response is not OK but is: %c\n", buf[0]);
#endif
        return 0;
    }
    return 1;
}

char acp_recvPing(Peer *peer) {
    char buf[peer->sock_buf_size];
    if (!clientRead(*(peer->fd), buf, sizeof buf)) {
        return '\0';
    }
    if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvPing: acp_crc_check() failed\n", stderr);
#endif
        return '\0';
    }
    return buf[1];
}

char acp_recvInt(Peer *peer) {
    char buf[peer->sock_buf_size];
    if (!clientRead(*(peer->fd), buf, sizeof buf)) {
        return '\0';
    }
    if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvPing: acp_crc_check() failed\n", stderr);
#endif
        return '\0';
    }
    return buf[1];
}

int acp_recvFTS(FTSList *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd) {
    char buf[buf_size];
    if (!clientRead(fd, buf, sizeof buf)) {
        return 0;
    }
    if (strlen(buf) < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: not enough data\n", stderr);
#endif
        return 0;
    }
    if (buf[0] != qnf) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: bad quantifier\n", stderr);
#endif
        return 0;
    }
    if (strncmp(&buf[1], cmd, 1) != 0) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: bad command\n", stderr);
#endif
        return 0;
    }
    if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: acp_crc_check() failed\n", stderr);
#endif
        return 0;
    }
    acp_parsePackFTS(buf, list, list_max_size);
    return 1;
}

int acp_recvI2(I2List *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd) {
    char buf[buf_size];
    if (!clientRead(fd, buf, sizeof buf)) {
        return 0;
    }
    if (strlen(buf) < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
        fputs("acp_recvI2: not enough data\n", stderr);
#endif
        return 0;
    }
    if (buf[0] != qnf) {
#ifdef MODE_DEBUG
        fputs("acp_recvI2: bad quantifier\n", stderr);
#endif
        return 0;
    }
    if (strncmp(&buf[1], cmd, 1) != 0) {
#ifdef MODE_DEBUG
        fputs("acp_recvI2: bad command\n", stderr);
#endif
        return 0;
    }
    if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvI2: acp_crc_check() failed\n", stderr);
#endif
        return 0;
    }
    acp_parsePackI2(buf, list, list_max_size);
    return 1;
}

FUN_LOCK(SensorInt)

FUN_LOCK(SensorFTS)

FUN_LOCK(Peer)

FUN_LOCK(EM)

FUN_UNLOCK(SensorInt)

FUN_UNLOCK(SensorFTS)

FUN_UNLOCK(Peer)

FUN_UNLOCK(EM)

int acp_setEMOutput(EM *em, int output) {
    if (lockEM(em)) {
        if (lockPeer(em->source)) {
            if (output == ((int) em->last_output)) {
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            I2 di[1];
            di[0].p0 = em->remote_id;
            di[0].p1 = output;
            I2List data = {di, 1};
            if (!acp_sendBufArrPackI2List(ACP_CMD_SET_INT, &data, em->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_setEMOutput: failed to send request where em.id = %d\n", em->id);
#endif
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            em->last_output = (float) output;
            unlockPeer(em->source);
            unlockEM(em);
            return 1;
        }
    }
}

int acp_setEMDutyCycle(EM *em, float output) {
    if (lockEM(em)) {
        if (lockPeer(em->source)) {
            if (output == em->last_output) {
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            I2 di[1];
            di[0].p0 = em->remote_id;
            di[0].p1 = (int) output;
            I2List data = {di, 1};
            if (!acp_sendBufArrPackI2List(ACP_CMD_SET_DUTY_CYCLE_PWM, &data, em->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_setEMDutyCycle: failed to send request where em.id = %d\n", em->id);
#endif
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            em->last_output = output;
            unlockPeer(em->source);
            unlockEM(em);
            return 1;
        }
    }
}

int acp_setEMOutputR(EM *em, int output) {
    if (lockEM(em)) {
        if (lockPeer(em->source)) {
            I2 di[1];
            di[0].p0 = em->remote_id;
            di[0].p1 = output;
            I2List data = {di, 1};
            if (!acp_sendBufArrPackI2List(ACP_CMD_SET_INT, &data, em->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_setEMOutput: failed to send request where em.id = %d\n", em->id);
#endif
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            em->last_output = (float) output;
            unlockPeer(em->source);
            unlockEM(em);
            return 1;
        }
    }
}

int acp_setEMDutyCycleR(EM *em, float output) {
    if (lockEM(em)) {
        if (lockPeer(em->source)) {
            I2 di[1];
            di[0].p0 = em->remote_id;
            di[0].p1 = (int) output;
            I2List data = {di, 1};
            if (!acp_sendBufArrPackI2List(ACP_CMD_SET_DUTY_CYCLE_PWM, &data, em->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_setEMDutyCycle: failed to send request where em.id = %d\n", em->id);
#endif
                unlockPeer(em->source);
                unlockEM(em);
                return 0;
            }
            em->last_output = output;
            unlockPeer(em->source);
            unlockEM(em);
            return 1;
        }
    }
}

int acp_readSensorInt(SensorInt *s) {
    if (lockSensorInt(s)) {
        if (lockPeer(s->source)) {
            struct timespec now = getCurrentTime();
            /*
                        if (!timeHasPassed(s->interval_min, s->last_read_time, now)) {
                            unlockPeer(s->source);
                            unlockSensorInt(s);
                            return s->last_return;
                        }
             */

            s->source->active = 0;
            s->source->time1 = now;
            s->last_read_time = now;
            s->last_return = 0;

            int di[1];
            di[0] = s->remote_id;
            I1List data = {di, 1};
            if (!acp_sendBufArrPackI1List(ACP_CMD_GET_INT, &data, s->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_readSensorInt: acp_sendBufArrPackI1List failed where sensor.id = %d\n", s->id);
#endif
                unlockPeer(s->source);
                unlockSensorInt(s);
                return 0;
            }

            //waiting for response...
            I2 td[1];
            I2List tl = {td, 0};

            int i;
            int done = 0;
            for (i = 0; i < ACP_RETRY_NUM; i++) {
                memset(&td, 0, sizeof tl);
                tl.length = 0;
                if (!acp_recvI2(&tl, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED, s->source->sock_buf_size, 1, *(s->source->fd))) {
#ifdef MODE_DEBUG
                    fprintf(stderr, "ERROR: acp_readSensorInt: acp_recvI2() error where sensor.id = %d\n", s->id);
#endif
                    unlockPeer(s->source);
                    unlockSensorInt(s);
                    return 0;
                }
                s->source->active = 1;
                if (tl.item[0].p0 == s->remote_id) {
                    done = 1;
                    break;
                }
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_readSensorInt: response:  peer returned id=%d but requested one was %d\n", tl.item[0].p0, s->remote_id);
#endif
            }
            if (!done) {
                unlockPeer(s->source);
                unlockSensorInt(s);
                return 0;
            }
            s->source->active = 1;
            s->value = tl.item[0].p1;
            s->last_return = 1;
            unlockPeer(s->source);
            unlockSensorInt(s);
            return 1;
        }
        unlockSensorInt(s);
    }
    return 0;
}

int acp_readSensorFTS(SensorFTS *s) {
    if (lockSensorFTS(s)) {
        if (lockPeer(s->source)) {
            struct timespec now = getCurrentTime();
            /*
                        if (!timeHasPassed(s->interval_min, s->last_read_time, now)) {
                            unlockPeer(s->source);
                            unlockSensorFTS(s);
                            return s->last_return;
                        }
             */

            s->source->active = 0;
            s->source->time1 = now;
            s->last_read_time = now;
            s->last_return = 0;
            s->value.state = 0;

            int di[1];
            di[0] = s->remote_id;
            I1List data = {di, 1};
            if (!acp_sendBufArrPackI1List(ACP_CMD_GET_FTS, &data, s->source)) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: sensorRead: acp_sendBufArrPackI1List failed where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
#endif
                unlockPeer(s->source);
                unlockSensorFTS(s);
                return 0;
            }

            //waiting for response...
            FTS td[1];
            FTSList tl = {td, 0};

            memset(&td, 0, sizeof tl);
            tl.length = 0;
            if (!acp_recvFTS(&tl, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED, s->source->sock_buf_size, 1, *(s->source->fd))) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: acp_readSensorFTS: acp_recvFTS() error where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
#endif
                unlockPeer(s->source);
                unlockSensorFTS(s);
                return 0;
            }
            s->source->active = 1;
            if (tl.item[0].id != s->remote_id) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: sensorRead: response: peer returned id=%d but requested one was %d\n", tl.item[0].id, s->remote_id);
#endif
                readAll(*(s->source->fd));
                unlockPeer(s->source);
                unlockSensorFTS(s);
                return 0;
            }
            if (tl.length != 1) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: sensorRead: response: number of items = %d but 1 expected\n", tl.length != 1);
#endif
                unlockPeer(s->source);
                unlockSensorFTS(s);
                return 0;
            }
            if (tl.item[0].state != 1) {
#ifdef MODE_DEBUG
                fprintf(stderr, "ERROR: sensorRead: response: temperature sensor state is bad where sensor.id = %d and remote_id=%d\n", s->id, s->remote_id);
#endif
                s->source->active = 1;
                unlockPeer(s->source);
                unlockSensorFTS(s);
                return 0;
            }
            s->source->active = 1;
            s->value = tl.item[0];
            s->last_return = 1;
            unlockPeer(s->source);
            unlockSensorFTS(s);
            return 1;
        }
        unlockSensorFTS(s);
    }
    return 0;
}

void acp_pingPeer(Peer *item) {
    if (lockPeer(item)) {
        readAll(*(item->fd));
        item->active = 0;
        char cmd_str[1] = {ACP_CMD_APP_PING};
        if (!acp_sendStrPack(ACP_QUANTIFIER_BROADCAST, cmd_str, item)) {
#ifdef MODE_DEBUG
            fputs("WARNING: acp_pingPeer: acp_sendStrPack failed\n", stderr);
#endif
            item->time1 = getCurrentTime();
            unlockPeer(item);
            return;
        }
        //waiting for response...
        char resp[2] = {'\0', '\0'};
        resp[0] = acp_recvPing(item);
        if (strncmp(resp, ACP_RESP_APP_BUSY, 1) != 0) {
#ifdef MODE_DEBUG
            fputs("WARNING: acp_pingPeer: acp_recvPing() peer is not busy\n", stderr);
#endif
            item->time1 = getCurrentTime();
            unlockPeer(item);
            return;
        }
        item->active = 1;
        item->time1 = getCurrentTime();
        unlockPeer(item);
    }
}

void acp_pingPeerList(PeerList *list, struct timespec interval, struct timespec now) {
    size_t i;
    FORL{
        if (timeHasPassed(interval, LIi.time1, now)) {
            acp_pingPeer(&LIi);
        }
    }
}

int acp_sendCmdGetInt(Peer *item, char* cmd, int *output) {
    if (lockPeer(item)) {
        item->active = 0;
        if (!acp_sendStrPack(ACP_QUANTIFIER_BROADCAST, cmd, item)) {
#ifdef MODE_DEBUG
            fputs("WARNING: acp_sendCmdGetInt: acp_sendStrPack failed\n", stderr);
#endif
            item->time1 = getCurrentTime();
            unlockPeer(item);
            return 0;
        }
        //waiting for response...
        char buf[item->sock_buf_size];
        if (!clientRead(*(item->fd), buf, sizeof buf)) {
            item->time1 = getCurrentTime();
            unlockPeer(item);
            return 0;
        }
        item->active = 1;
        item->time1 = getCurrentTime();
        unlockPeer(item);

        if (!acp_crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
            fputs("acp_sendCmdGetInt: acp_crc_check() failed\n", stderr);
#endif
            return 0;
        }
        char *buff = buf;
        if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
            fputs("acp_sendCmdGetInt: ERROR: bad buff length\n", stderr);
#endif
            return 0;
        }
        acp_bufToData(&buff);
        int p0;
        if (sscanf(buff, "%d", &p0) != 1) {
#ifdef MODE_DEBUG
            fputs("acp_sendCmdGetInt: ERROR: failed to scan\n", stderr);
#endif
            return 0;
        }
        *output = p0;
        return 1;
    }
#ifdef MODE_DEBUG
    fputs("acp_sendCmdGetInt: ERROR: lock failed\n", stderr);
#endif
    return 0;
}



int acp_sendSMS(Peer *peer, char *phone, char *message) {
    if (lockPeer(peer)) {
        S2 di[1];
        memcpy(di[0].p0, phone, LINE_SIZE);
        memcpy(di[0].p1, message, LINE_SIZE);
        S2List data = {di, 1};
        if (!acp_sendBufArrPackS2List(ACP_CMD_GWU59_SEND_SMS, &data, peer)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "ERROR: acp_sendSMS: failed to send request where peer.id = %s\n", peer->id);
#endif
            unlockPeer(peer);
            return 0;
        }
        unlockPeer(peer);
        return 1;
    }

}

int acp_makeCall(Peer *peer, char *phone) {
    if (lockPeer(peer)) {
        S1List data = {phone, 1};
        if (!acp_sendBufArrPackS1List(ACP_CMD_GWU59_RING, &data, peer)) {
#ifdef MODE_DEBUG
            fprintf(stderr, "ERROR: acp_makeCall: failed to send request where peer.id = %s\n", peer->id);
#endif
            unlockPeer(peer);
            return 0;
        }
        unlockPeer(peer);
        return 1;
    }

}
int acp_catFTS(int id, float value, struct timespec tm, int state, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR ACP_FLOAT_FORMAT ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR, id, value,tm.tv_sec,tm.tv_nsec, state);
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}
void freePeer(PeerList *list) {
    free(list->item);
    list->item = NULL;
    list->length = 0;
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