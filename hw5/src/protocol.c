#include "csapp.h"
#include "protocol.h"
#include "debug.h"
#include <time.h>

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload){
    //validate input
    if(fd<0 || hdr==NULL){
        debug("Invalid arguments: fd = %d, hdr = %p", fd, hdr);
        return -1;
    }
    uint16_t payload_size = 0;
    if(payload!=NULL){
        payload_size = hdr->payload_length;
    }
    else{
        payload_size = 0;
    }

    struct timespec time;

    clock_gettime(CLOCK_REALTIME,&time);
    
    //set header fields in network byte order
    hdr->timestamp_sec = time.tv_sec;
    hdr->timestamp_nsec = time.tv_nsec;
    hdr->payload_length = htonl(hdr->payload_length);
    hdr->msgid = htonl(hdr->msgid);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    int size = sizeof(CHLA_PACKET_HEADER);
    int size1 = size;
    if((rio_writen(fd, hdr, size))< 0){
        return -1;
        size = size - size1;
    }
    debug("send package: fd number is %d, header size is %d", fd, size);

    size1 = payload_size;
    size = size1;
    if(payload_size != 0){
        if((rio_writen(fd, payload, size)) < 0 ){
            return -1;
            payload_size = payload_size - size1;
        }
    }

    debug("send package: fd number is %d, payload size is %d", fd, size);


    return 0;

}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload){
    //validate input
    if(fd<0||hdr==NULL||payload==NULL){
        debug("Invalid arguments: fd=%d,hdr=%p,payload=%p", fd,hdr,payload);
        return -1;
    }

    int size = sizeof(CHLA_PACKET_HEADER);
    int size1 = size;

    if((size1 = Rio_readn(fd, hdr, size))<=0){
        return -1;
         size = size -size1;
    }
    
    //convert header fields from network to host byte order
    hdr->payload_length = ntohl(hdr->payload_length);
    hdr->msgid = ntohl(hdr->msgid);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    debug("receive package: fd number is %d, header size is %d", fd, size);

     if(hdr->payload_length != 0){
        char *payloadp1 = malloc(hdr->payload_length);
        memset(payloadp1, 0, hdr->payload_length);
        size = hdr->payload_length;
        size1 = size;
        if((Rio_readn(fd, payloadp1, size))<=0){

            free(payloadp1);

            return -1;

            size = size - size1;
        }

        debug("send package: fd number is %d, payload size is %d", fd, size);


        *payload = payloadp1;
        return 0;
    }

    return 0;
}
