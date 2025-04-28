#ifndef PACKET_HANDLER_H
#define PACKET_HANDLER_H

#include <pcap.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <pthread.h>
#include "ringbuffer.h"

#define MAX_DEVICENAME_SIZE 50
#define MAX_DEVICES 10
#define MAX_DATE_STR_SIZE 30
#define MAX_MAC_STR_SIZE 18
#define MAX_ETHTYPE_STR_SIZE 10
#define MAX_PTC_STR_SIZE 20

/*TODO:
    1. Loop改成能中斷依照需求中斷 -> ok
    2. 將得到的結果轉成字串加入output_buffer
    3. 加入filter篩選對應封包
    4. 整理struct aligment
*/

/* 功能:
    1. start_packet_listener: 啟動packet listener
    2. stop_packet_listener: 停止packet listener
    3. packet_listener: 利用pcap_loop blocking的抓取封包
    4. packet_handler: 解析處理抓到的封包並轉化成字串傳給ui的output buffer
*/

typedef enum{   // 定義整體輸出狀態
    IDLE = 0,   // 停止輸出
    RUNNING, // 顯示所有封包資訊
} running_state;

// Need to be align
struct simple_info{
    /* use to print otuline of each packet on monitor */
    size_t index, length;
    time_t arr_time;
    char arr_time_str[MAX_DATE_STR_SIZE];
    char src_mac[MAX_MAC_STR_SIZE], dst_mac[MAX_MAC_STR_SIZE];
    char src_ip[INET6_ADDRSTRLEN], dst_ip[INET6_ADDRSTRLEN]; // can also store IPv4 address
    // uint_16_t eth_type //  2 bytes
    // char ptc_type; //  1 byte
    char eth_type_str[MAX_ETHTYPE_STR_SIZE], ptc_type_str[MAX_PTC_STR_SIZE];
};

struct ip_header{
    union {
        const struct iphdr *ip4_hdr; // IPv4
        const struct ip6_hdr *ip6_hdr; // IPv6
    };
    const char *ip_type;
};

void start_packet_listener();
void stop_packet_listener();
void packet_handler(u_char *user, const struct pcap_pkthdr *pkthdr, const u_char *bytes);
/* Start Listening packet */
void packet_listener();

#endif