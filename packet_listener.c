#include "packet_listener.h"

/* TODO:
    1. TCP/UDP header parser

*/

extern RingBuffer output_buf; // buffer for output in main.c

static int packet_count = 0;
static running_state state = IDLE;
pcap_t *handle;

int mac2str(char str[], const uint8_t *src, size_t str_len){
    /* Convert MAC address to String */
    if(str == NULL || str_len < MAX_MAC_STR_SIZE){
        printf("Error when getting MAC address\n");
        return 1;
    }
    snprintf(str, MAX_MAC_STR_SIZE, "%2x:%2x:%2x:%2x:%2x:%2x", src[0], src[1], src[2], src[3], src[4], src[5]);
    return 0;
}
int ip2str(char *str, const void *ip, const uint16_t ip_type, size_t str_len){
    /* str must have at least 46 (INET6_ADDRSTRLEN) bytes of memory */
    if(str == NULL || str_len < INET6_ADDRSTRLEN){
        printf("Error when getting IP address\n");
        return 1;
    }
    switch(ip_type){
        case ETHERTYPE_IPV6:
            inet_ntop(AF_INET6, (struct in6_addr *)ip, str, INET6_ADDRSTRLEN);
            break;
        case ETHERTYPE_IP:
            inet_ntop(AF_INET, (in_addr_t *)ip, str, INET_ADDRSTRLEN);
            break;
        default:
            sprintf(str, "Unknown IP type");
            break;
    }
    return 0;
}

int ethtype2str(char *str, const uint16_t eth_type, size_t str_len){
    if(str == NULL || str_len < MAX_ETHTYPE_STR_SIZE){
        printf("Error when getting Ethertype\n");
        return 1;
    }
    switch(eth_type){
        case ETHERTYPE_IP:
            strcpy(str, "IPv4");
            break;
        case ETHERTYPE_IPV6:
            strcpy(str, "IPv6");
            break;
    }
    return 0;
}

const struct ether_header *parse_ETH(const u_char *packet){
    return (const struct ether_header *)packet;
}

const struct ip6_hdr *parse_IPv6(const u_char *packet){
    /* return ipv6 header given the packet which is already being offseted */
    return (const struct ip6_hdr *)packet;
}

const struct iphdr *parse_IPv4(const u_char *packet){
    /* return ip4 header given the packet which is already being offseted */
    return (const struct iphdr*)packet;
}

// void print_IPv4_info(){

// }

// void print_IPv6_info(){
    // src_ip = (char *)malloc(INET6_ADDRSTRLEN);
    // dst_ip = (char *)malloc(INET6_ADDRSTRLEN);
    // if(inet_ntop(AF_INET6, &(ip_hdr.ip6_hdr->ip6_src), src_ip, sizeof(src_ip)) == 0) {
    //     printf("[Packet Handler] Error when converting src IP\n");
    // }
    // if(inet_ntop(AF_INET6, &(ip_hdr.ip6_hdr->ip6_dst), dst_ip, sizeof(dst_ip)) == 0) {
    //     printf("[Packet Handler] Error when converting dst IP\n");
    // }
    // printf("[Packet Handler] Source IP: %s\n", src_ip);
    // printf("[Packet Handler] Destination IP: %s\n", dst_ip);
// }

// void print_IP_header(const struct *ip_header){
    // printf("[IP header]: IP type: %s\n", ip_header->ip_type);
    // switch(ip_header->ip_type){
    //     case "IPv4":
    //         break;
    //     case "IPv6":
    //         break;
    //     default:
    //         printf("[Packet Handler] Unknown IP type\n");
    // }
    // return;
// }

int getDeviceName(char* errbuf, char** allDevName, int devSize){
    // *allDevName = malloc(sizeof(char*) * devSize);
    if(allDevName == NULL){
        printf("Can't allocate when getting devices\n");
        return 0;
    }
    for(int i=0; i<devSize; i++){
        allDevName[i] = (char*)malloc(MAX_DEVICENAME_SIZE);
        if(allDevName[i] == NULL){
            printf("Can't allocate when getting devices\n");
            return 0;
        }
        allDevName[i][0] = '\0'; // initailize to empty string
    }
    // Get all avaliable devices
    pcap_if_t *allDev;
    if(pcap_findalldevs(&allDev, errbuf) == -1 || allDev == NULL){
        printf("error: %s\n", errbuf);
        return 0;
    }
    int device_count = 0;
    for(pcap_if_t *iter = allDev; iter != NULL; iter = iter->next){
        if(device_count >= MAX_DEVICES) break;
        // (*allDevName)[device_count] = (char*)malloc(sizeof(char) * strlen(iter->name)+1);
        strcpy(allDevName[device_count], iter->name);
        device_count++;
        // printf("%s\n", (*allDevName)[device_count]);
    }
    pcap_freealldevs(allDev);
    return device_count;
}

void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet){
    /*  u_char *user: the user data.
        struct pcap_pkthr *header include:
            1. ts: timestamp
            2. cpalen: total length of the packet we already captured
            3. len: total length of the packet
        uchar *packet: content of the packet
    */
    /*
        This Function parse the packet into:
            1. Etherent header (v)
            2. IP header
            3. TCP header
    */
    /* 1. Output Basic Information */
    struct simple_info simple_packet;
    const char *timestamp = ctime((const time_t *)&header->ts);
    size_t packet_index = ++packet_count;
    // printf("[Packet Handler] Packet captured #%d\n", packet_count);
    // printf("[Packet Handler] Total length: %d, Receive length: %d\n", header->len, header->caplen);
    // printf("[Packet Handler] TimeStamp: %s", timestamp);
    
    // 2. Parse Ethernet header
    const struct ether_header *eth = parse_ETH(packet);
    const uint8_t *src_mac = eth->ether_shost;
    const uint8_t *dst_mac = eth->ether_dhost;
    // because struct ether_header is not aligned and cause 
    const uint16_t ether_type = ntohs(eth->ether_type);
    
    // printf("[ETH parser] Source MAC: %x:%x:%x:%x:%x:%x\n", src_mac[0], src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
    // printf("[ETH parser] Destination MAC: %x:%x:%x:%x:%x:%x\n", dst_mac[0], dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]);

    

    /* ETHERTYPE(uint16_t) is exactly 2 bytes, so 
       use ntohs to convert from NBO(Network Byte order) to HBO(Host Byte Order)
        IPv4: 0x0800
        ARP: 0x0806
        IPv6: 0x86dd
    */
    struct ip_header ip_hdr;
    const void *src_ip_raw, *dst_ip_raw;
    switch(ether_type){
        case ETHERTYPE_IP:
            /* parse IPv4 packet */
            // printf("[Packet Handler] IPv4 packet\n");
            ip_hdr.ip4_hdr = parse_IPv4(packet+sizeof(struct ether_header));
            // ip_hdr.ip4_hdr = (struct iphdr*)(packet+sizeof(struct ether_header));
            ip_hdr.ip_type = "IPv4";
            src_ip_raw = &ip_hdr.ip4_hdr->saddr;
            dst_ip_raw = &ip_hdr.ip4_hdr->daddr;
            break;
        case ETHERTYPE_ARP:
            // printf("[Packet Handler] ARP packet\n");
            break;
        case ETHERTYPE_IPV6:
            /* parse IPv6 packet */
            // printf("[Packet Handler] IPv6 packet\n");
            ip_hdr.ip6_hdr = parse_IPv6(packet+sizeof(struct ether_header));
            // ip_hdr.ip6_hdr = (struct ip6_hdr*)(packet+sizeof(struct ether_header));
            ip_hdr.ip_type = "IPv6";
            /* Output IP address */
            src_ip_raw = &ip_hdr.ip6_hdr->ip6_src;
            dst_ip_raw = &ip_hdr.ip6_hdr->ip6_dst;
            // src_ip = ip2str(ip_hdr.ip6_hdr->ip6_src, *ether_type);
            // dst_ip = ip2str(ip_hdr.ip4_hdr->ip6_dst, *ether_type);
            break;
        default:
            // printf("[Packet Handler] Unknown packet\n");
            break;
    }
        /* Output IP information */

    /* write to simple information */
    /* Write Ethernet information */
    simple_packet.index = packet_index;
    simple_packet.arr_time = header->ts.tv_sec;
    simple_packet.length = header->len;
    mac2str(simple_packet.src_mac, src_mac, sizeof(simple_packet.src_mac)/sizeof(src_mac[0]));
    mac2str(simple_packet.dst_mac, dst_mac, sizeof(simple_packet.dst_mac)/sizeof(dst_mac[0]));
    ethtype2str(simple_packet.eth_type_str, ether_type, sizeof(simple_packet.eth_type_str)/sizeof(simple_packet.eth_type_str[0]));
    // Note: this line will cause synchronization problem in multithread situation (ctime return a static memory space).
    strcpy(simple_packet.arr_time_str, ctime((const time_t *)&header->ts.tv_sec)); 
    if(strlen(simple_packet.arr_time_str) > 0)
        simple_packet.arr_time_str[strlen(simple_packet.arr_time_str)-1] = '\0';
    /* Write IP information */
    ip2str(simple_packet.src_ip, src_ip_raw, ether_type, sizeof(simple_packet.src_ip)/sizeof(simple_packet.src_ip[0]));
    ip2str(simple_packet.dst_ip, dst_ip_raw, ether_type, sizeof(simple_packet.dst_ip)/sizeof(simple_packet.dst_ip[0]));


    /* push into output_buffer */
    char msg[MAX_MSG_SIZE];
    sprintf(msg, "# %zu", simple_packet.index);
    rb_push_overlapped(&output_buf, msg);
    sprintf(msg, "\tLength: %zu \t Time Stamp: %s", simple_packet.length, simple_packet.arr_time_str);
    rb_push_overlapped(&output_buf, msg);
    sprintf(msg, "\tMAC address: src: %s \tdst: %s", simple_packet.src_mac, simple_packet.dst_mac);
    rb_push_overlapped(&output_buf, msg);
    sprintf(msg, "\tEther type: %s", simple_packet.eth_type_str);
    rb_push_overlapped(&output_buf, msg);
    sprintf(msg, "\tIP address: src: %s \tdst: %s", simple_packet.src_ip, simple_packet.dst_ip);
    rb_push_overlapped(&output_buf, msg);



    // printf("[Debug] Packet length: %ld\n", simple_packet.length);
    // printf("[Debug] Packet arrive second: %ld\n", simple_packet.arr_time);
    // printf("[Debug] Packet index: %ld\n", simple_packet.index);
    // printf("[Debug] Arrive time: %s\n", simple_packet.arr_time_str);
    // printf("[Debug] Source MAC: %s\n", simple_packet.src_mac);
    // printf("[Debug] Destination MAC: %s\n", simple_packet.dst_mac);
    // printf("[Debug] Ethertype: %s\n", simple_packet.eth_type_str);
    // printf("[Debug] Source IP: %s\n", simple_packet.src_ip);
    // printf("[Debug] Destination IP: %s\n", simple_packet.dst_ip);

    // fprintf(stdout, "\n");
}

void close_listener(pcap_t *handler, char** all_device, int max_device_number){
    if(handler != NULL) 
        pcap_close(handler);
    if(all_device != NULL){
        for(int i=0; i<max_device_number; i++){
            free(all_device[i]);
        }
    }
    // fprintf(stdout, "[Close Listener] Listener closed\n");
}

void packet_listener(){
    char errbuf[PCAP_ERRBUF_SIZE];
    char *allDevices[MAX_DEVICES];
    int device_number = 0;
    
    // fprintf(stdout, "Starting...\n");

    // Getting devices
    device_number = getDeviceName(errbuf, allDevices, MAX_DEVICES);
    const char *device = allDevices[0];
    // printf("[Get Devices] %d devices\n", device_number);
    // printf("[Get Devices] Using %s ", device);

    // Create pcap object
    handle = pcap_create(device, errbuf);
    if(handle == NULL){
        // fprintf(stderr, "error when creating pcap handler: %s\n", errbuf);
        close_listener(handle, allDevices, MAX_DEVICES);
        return;
    }
    pcap_set_snaplen(handle, 65535); // man length of package
    pcap_set_promisc(handle, 1); // promiscuous mode
    pcap_set_timeout(handle, 1000); // timeout in ms
    pcap_set_buffer_size(handle, 1 << 20); // 1MB buffer

    // Activate pcap
    if(pcap_activate(handle) != 0){
        // fprintf(stderr, "error when activating pcap handler: %s\n", pcap_geterr(handle));
        return;
    }
    // loop over each packet
    int loop_ret = pcap_loop(handle, -1, packet_handler, NULL);
    if(loop_ret == -1){
        // fprintf(stderr, "error when looping: %s\n", pcap_geterr(handle));
        close_listener(handle, allDevices, MAX_DEVICES);
        return;
    }

    close_listener(handle, allDevices, MAX_DEVICES);
}

void start_packet_listener(){
    /* Start listening package on "pcap_t *handle" */
    state = RUNNING;
    packet_listener();
}

void stop_packet_listener(){
    /* pcap_loop 為blocking, 但可以利用pcap_breakloop()來強制中斷 */
    /* pcap也有non-blocking寫法(pcap_dispatch + pcap_setnonblock) */
    state = IDLE;
    pcap_breakloop(handle);
}