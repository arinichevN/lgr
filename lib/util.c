#include "util.h"
void dumpStr(const char *buf) {
    int i;
    for (i = 0; i < strlen(buf); i++) {
        printf("%hhu.", buf[i]);
    }
    putchar('\n');
}

void strnline(char **v) {
    char *c;
    c = strchr(*v, '\n');
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if(**v=='\n'){
            *v = strchr(*v, '\0');
        }
    }
}

char * bufCat(char * buf, const char *str, size_t size) {
    if (strlen(buf) + strlen(str) + 1 >= size) {
#ifdef MODE_DEBUG
        fputs("bufCat: buffer overflow\n", stderr);
#endif
        return NULL;
    }
    return strcat(buf, str);
}

char * estostr(char *s) {
    int i,j=0;
    int f = 0;
    char ss[strlen(s) + 1];
    memset(ss, 0, sizeof ss);
    for (i = 0; i < strlen(s)+1; i++) {
        
        if (s[i] == '\\') {
            f = 1;
            continue;
        }
        if (f) {
            switch (s[i]) {
                case 'n':
                    ss[j] = '\n';
                    break;
                case 'r':
                    ss[j] = '\r';
                    break;
                case 't':
                    ss[j] = '\t';
                    break;
                case '0':
                    ss[j] = '\0';
                    break;
                case '\\':
                    ss[j] = '\\';
                    break;
                default:
                    ss[j] = '\\';
                    j++;
                    ss[j] = s[i];
                    break;
            }
            j++;
            f = 0;
            continue;
        }
        ss[j] = s[i];
        j++;
    }
    return strcpy(s, ss);
}
