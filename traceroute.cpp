#include <iostream>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <cstring>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp6.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
using namespace std;

int main(){
    int udp = socket(AF_INET,SOCK_DGRAM,0);
    if(udp<0){
        cout<<"udp socket err"<<endl;
        return -1;
    }
    string host;
    cout<<"Enter host : ";cin>>host;
    addrinfo hints,*res;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if(getaddrinfo(host.c_str(),"37916",&hints,&res)){
        cout<<"getaddrinfo err"<<endl;
        return -1;
    }

    int s = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(s<0){
        cout<<"icmp socket err"<<endl;
        return -1;
    }
    int recvbuf = 1024*60;
    if(setsockopt(s,SOL_SOCKET,SO_RCVBUF,&recvbuf,sizeof(recvbuf))){
        cout<<"setting recv buf err"<<endl;
        return -1;
    }

    sockaddr_in hop;
    socklen_t hopl = sizeof(hop);
    char RecvBuf[1024];
    char SendBuf[64];
    ushort seq=0;
    srand(time(0));
    for(int ttl=1;ttl<256;ttl++){
        ((sockaddr_in*)res->ai_addr)->sin_port = htons(16000+(unsigned short)rand()%65000);
        if(setsockopt(udp,IPPROTO_IP,IP_TTL,&ttl,sizeof(ttl))){
            cout<<"ttl sockopt err"<<endl;
            return -1;
        }
        memset(SendBuf,0,sizeof(SendBuf));
        int bytes = sendto(udp,"a",1,0,res->ai_addr,res->ai_addrlen);
        if(ttl == 255){
            for(int j=0;j<100;j++)
                sendto(udp,"1",1,0,res->ai_addr,res->ai_addrlen);
        }
    }
    while(true){
        int rbytes = recvfrom(s,RecvBuf,1024,0,(sockaddr*)&hop,&hopl);
        ip* iph = (ip*)RecvBuf;
        icmp* icmph = (icmp*)(RecvBuf+sizeof(ip));
        char host[256];
        getnameinfo((sockaddr*)&hop,hopl,host,256,nullptr,0,0);
        if(icmph->icmp_type == ICMP_UNREACH_PORT ||
                (icmph->icmp_type == ICMP_TIMXCEED&&
                 icmph->icmp_code==ICMP_TIMXCEED_INTRANS)){
            if(icmph->icmp_code == (uint8_t)-1)
                break;
            cout<<host<<"("<<inet_ntoa(iph->ip_src)<<")"<<endl;
            if(iph->ip_src.s_addr ==
                    ((sockaddr_in*)res->ai_addr)->sin_addr.s_addr)
                break;
            continue;
        }else if(icmph->icmp_type == ICMP_UNREACH){
            cout<<inet_ntoa(iph->ip_src)<<endl;
            break;
        }else{
            cout<<"from "<<inet_ntoa(iph->ip_src)<<
                  " received icmp type "<<icmph->icmp_type<<endl;
        }
    }
    return 0;
}
