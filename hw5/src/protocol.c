#include "csapp.h"
#include "protocol.h"
#include "debug.h"

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload){
    //validate input
    if(fd<0 || hdr==NULL){
        debug("Invalid arguments: fd = %d, hdr = %p", fd, hdr);
        return -1;
    }
    
    //set header fields in network byte order
    hdr->payload_length = htonl(hdr->payload_length);
    hdr->msgid = htonl(hdr->msgid);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    //send the packet header 
    if(rio_writen(fd, hdr, sizeof(CHLA_PACKET_HEADER))!= sizeof(CHLA_PACKET_HEADER)){
        debug("Failed to send packet header");
        return -1;
    }
    debug("sent packet header: fd = %d, size = %zu", fd, sizeof(CHLA_PACKET_HEADER));

    //send the payload
    if(payload!=NULL && ntohl(hdr->payload_length)>0){
        if(rio_writen(fd,payload,ntohl(hdr->payload_length))!= ntohl(hdr->payload_length)){
            debug("Failed to send payload");
            return -1;
        }
        debug("Sent payload: fd= %d, size = %u", fd, ntohl(hdr->payload_length));
    }
    return 0;

}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload){
    //validate input
    if(fd<0||hdr==NULL||payload==NULL){
        debug("Invalid arguments: fd=%d,hdr=%p,payload=%p", fd,hdr,payload);
        return -1;
    }

    //Read the packet header
    if(rio_readn(fd,hdr,sizeof(CHLA_PACKET_HEADER))!=sizeof(CHLA_PACKET_HEADER)){
        debug("Failed to read packet header");
        return -1;
    }
    debug("Recieved packet header: fd=%d, size=%zu", fd, sizeof(CHLA_PACKET_HEADER));

    //convert header fields from network to host byte order
    hdr->payload_length = ntohl(hdr->payload_length);
    hdr->msgid = ntohl(hdr->msgid);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    //Read payload if it exists 
    if(hdr->payload_length>0){
        char* payload_buffer = Malloc(hdr->payload_length);
        if(rio_readn(fd,payload_buffer,hdr->payload_length)!=hdr->payload_length){
            debug("Failed to read payload");
            Free(payload_buffer);
            return -1;
        }
        *payload = payload_buffer;
        debug("Recieved payload: fd=%d, size=%u",fd,hdr->payload_length);
    }
    else{
        *payload = NULL;
    }
    return 0;
}
