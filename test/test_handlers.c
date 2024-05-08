#include "test_handlers.h"

int registration_sock_handler(int client_sock) {
    application_control_segment_t seg = { };

    // RECEIVE REQUEST HEADER
    ssize_t rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);
    
    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);
    
    printf("Received seg header has op = %u\n", seg.header.info.op);

    assert(seg.header.info.op == OP_DIR_REQUEST(APP_CONTROL_OP_REGISTER)); // we are testing registration only
    
    // RECEIVE REQUEST DATA (REGISTRATION SECTION)
    rv = recv(client_sock, seg.data, sizeof(application_registration_section_t), 0);

    printf("recv for data returned with %d\n", rv);
    printf("errno = %d\n", errno);

    application_registration_section_t* pRegistrationSection = (application_registration_section_t*)seg.data;
    //user_id is not null-terminated, so we need to print it differently
    printf("UID = %.*s\n", (int)MAX_USER_ID_LENGTH, pRegistrationSection->user_id);
    printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\n", pRegistrationSection->dev_mac[0], pRegistrationSection->dev_mac[1], pRegistrationSection->dev_mac[2], pRegistrationSection->dev_mac[3], pRegistrationSection->dev_mac[4], pRegistrationSection->dev_mac[5]);
    
    memcpy(pRegistrationSection->cid, "abracadabra_test", CID_LENGTH);

    seg.header.info.op = OP_DIR_RESPONSE(APP_CONTROL_OP_REGISTER);

    // SEND RESPONSE
    rv = send(client_sock, seg.raw, sizeof(application_control_segment_info_t) + sizeof(application_registration_section_t), 0);
    
    printf("send for response returned with %d\n", rv);
    printf("errno = %d\n", errno);

    // RECEIVE REQUEST HEADER
    rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf("Received seg header has op = %u\n", seg.header.info.op);

    assert(seg.header.info.op == OP_DIR_REQUEST(APP_CONTROL_OP_REGISTER)); // second stage of registration

    // RECEIVE REQUEST DATA (REGISTRATION SECTION at second stage - format is the same as for the first stage - this is confirmation of registration by the device)
    rv = recv(client_sock, seg.data, sizeof(application_registration_section_t), 0);

    printf("recv for data returned with %d\n", rv);
    printf("errno = %d\n", errno);

    pRegistrationSection = (application_registration_section_t*)seg.data;
    printf("UID = %.*s\n", (int)MAX_USER_ID_LENGTH, pRegistrationSection->user_id);
    printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\n", pRegistrationSection->dev_mac[0], pRegistrationSection->dev_mac[1], pRegistrationSection->dev_mac[2], pRegistrationSection->dev_mac[3], pRegistrationSection->dev_mac[4], pRegistrationSection->dev_mac[5]);
    printf("CID = %.*s\n", (int)CID_LENGTH, pRegistrationSection->cid);
    printf("CKEY = %.*s\n", (int)CKEY_LENGTH, pRegistrationSection->ckey);

    seg.header.info.op = OP_DIR_RESPONSE(APP_CONTROL_OP_REGISTER);
    // SEND RESPONSE (final step - confirmation of receiving the CKEY and guarantee completion of registration) - return complete segment as received from the device, but with the response bit set
    rv = send(client_sock, seg.raw, sizeof(application_control_segment_info_t) + sizeof(application_registration_section_t), 0);

    printf("send for response returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf ("END OF REGISTRATION TEST\n");

    return 0;
}
int initcomm_sock_handler(int client_sock) {
    application_control_segment_t seg = { };

    // RECEIVE REQUEST HEADER
    ssize_t rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);
    
    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);
    
    printf("Received seg header has op = %u\n", seg.header.info.op);

    assert(seg.header.info.op == OP_DIR_REQUEST(APP_CONTROL_OP_INITCOMM)); // we are testing initcomm and expecting initcomm only
    
    // RECEIVE REQUEST DATA (INITCOMM SECTION)
    rv = recv(client_sock, seg.data, sizeof(application_initcomm_section_t), 0);

    printf("recv for data returned with %d\n", rv);
    printf("errno = %d\n", errno);

    application_initcomm_section_t* pInitcommSection = (application_initcomm_section_t*)seg.data;
    //printf("CID = %.*s\n", (int)CID_LENGTH, pInitcommSection->cid);
    //printf("CKEY = %.*s\n", (int)CKEY_LENGTH, pInitcommSection->ckey);
    printf("CID = "); print_hex(pInitcommSection->cid, CID_LENGTH); printf("\n");
    printf("CKEY = "); print_hex(pInitcommSection->ckey, CKEY_LENGTH); printf("\n");

    seg.header.info.op = OP_DIR_RESPONSE(APP_CONTROL_OP_INITCOMM);
    memcpy(seg.header.info.csid, "initcommCsidTest", COMM_CSID_LENGTH);
    //printf("Set csid to %.*s\n", (int)COMM_CSID_LENGTH, seg.header.info.csid);
    printf("Set CSID to "); print_hex(seg.header.info.csid, COMM_CSID_LENGTH); printf("\n");
    // SEND RESPONSE
    rv = send(client_sock, seg.raw, sizeof(application_control_segment_info_t) + sizeof(application_initcomm_section_t), 0);


    printf("send for response returned with %d\n", rv);
    printf("errno = %d\n", errno);
    
    printf ("END OF INITCOMM TEST\n");
    

    return 0;
}
int start_unconditional_stream_sock_handler(int client_sock) {
    application_control_segment_t seg = {
        .header = {
            .info = {
                .op = OP_DIR_REQUEST(APP_CONTROL_OP_CAM_START_UNCONDITIONAL_STREAM),
                .csid = {0},
                .data_length = 0
            }
        }
    };
    
    // Send request header (no need to send data for this operation, because it is a request to start a stream, which has no data section)
    ssize_t rv = send(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("send for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    // Receive response header
    rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    assert(seg.header.info.op == OP_DIR_RESPONSE(APP_CONTROL_OP_CAM_START_UNCONDITIONAL_STREAM));

    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf ("END OF START_UNCONDITIONAL_STREAM TEST\n");

    return 0;
}
int stop_unconditional_stream_sock_handler(int client_sock) {
    application_control_segment_t seg = {
        .header = {
            .info = {
                .op = OP_DIR_REQUEST(APP_CONTROL_OP_CAM_STOP_UNCONDITIONAL_STREAM),
                .csid = {0},
                .data_length = 0
            }
        }
    };
    
    // Send request header (no need to send data for this operation, because it is a request to stop a stream, which has no data section)
    ssize_t rv = send(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("send for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    // Receive response header
    rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    assert(seg.header.info.op == OP_DIR_RESPONSE(APP_CONTROL_OP_CAM_STOP_UNCONDITIONAL_STREAM));

    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf ("END OF STOP_UNCONDITIONAL_STREAM TEST\n");

    return 0;
}
int nop_sock_handler(int client_sock) {
    application_control_segment_t seg = {
        .header = {
            .info = {
                .op = OP_DIR_REQUEST(APP_CONTROL_OP_NOP),
                .csid = {0},
                .data_length = 0
            }
        }
    };

    ssize_t rv = send(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("send for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    rv = recv(client_sock, seg.header.raw, sizeof(application_control_segment_info_t), 0);

    printf("(seg.header.info.op is %u)", seg.header.info.op);
    assert(seg.header.info.op == OP_DIR_RESPONSE(APP_CONTROL_OP_NOP));

    printf("recv for header returned with %d\n", rv);
    printf("errno = %d\n", errno);

    printf ("END OF NOP TEST\n");

    return 0;
}