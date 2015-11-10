//==============================================================================
// Name        : ss.c
// Author      : Nicholas
// Version     : 1
// Copyright   : None
// Description : v1: Stepping Stone for awget
// Arguments   : This program takes in 2 optional arguments
//			   : arg1 = flag either -p or -h (optional)
//                  -p indicates that a specified port will be used.
//                  -h will print help message.
//			   : arg2 = Port number (optional, but required when -p flag is
//                                  used)
//
//  This program was written for CS457 at CSU as apart of a CS degree program.
//
//==============================================================================


#include "awget.h"
#include <pthread.h>

struct req_thread_args{
    char request[1024];
    int sock;
};
typedef struct req_thread_args req_T;

struct rel_thread_args{
    int num_of_stones;
    StepStone* stones;
    char request[1024];
    int return_sock;
};
typedef struct rel_thread_args rel_T;


void print_help(){
    printf("ss program:\n");
    printf("This program will create a Stepping Stone for the awget program\n");
    printf("Usage: ./ss [options]\n");
    printf("\nOPTIONAL FLAGS\n");
    printf("-p <Port Number> Specifies the port to listen on\n");
    printf("-h Prints the help message.\n");
    printf("\nUsing this program with no option will create a Stepping Stone "
                   "on the default port (21789).\n");
}

int errorCheck(int ec) {
    switch (ec) { //Sets up switch to detect type of error.
        case 0:
            return 0;
        case -1: //Port issue
            fprintf(stderr, "Invalid Port Number\n");
            break;
        case -2: //Argument issue
            fprintf(stderr, "Arguments not correct\n");
            print_help();
            break;
        case -3:
            fprintf(stderr, "SS failed to start\n");
            break;
        case -4:
            fprintf(stderr, "SS failed to send\n");
            break;
        case -5:
            fprintf(stderr, "SS received invalid chainfile info.\n");
            break;
        case -6:
            fprintf(stderr, "SS unable to create thread.\n");
            break;
        default:
            fprintf(stderr, "Unknown error.\n");
    }
    return -1;
}

unsigned int gettid() {
    pthread_t ptid = pthread_self();
    uint64_t threadId = 0;
    memcpy(&threadId, &ptid, sizeof(threadId));
    return (unsigned int)threadId;
}

int send_ACK(int sock){
    if (send(sock, "ACK.", sizeof("ACK."), 0) != sizeof("ACK.")) return 1;
    return 0;
}

int wait_ACK(int sock){
    char buffer[BUFFERSIZE];
    int bytesRcvd = 0;
    for(;;){
        memset(&buffer, '\0', BUFFERSIZE);
        if ((bytesRcvd = recv(sock, buffer, 4, 0)) < 0) {
            fprintf(stderr, "Recv ACK. return failed\n");
            return 1;
        }
        if (strcmp(buffer, "ACK.")==0) {
            break;
        }
    }
    return 0;
}

int send_start_return_req(int sock, int sz){
    char buffer[BUFFERSIZE];

    if (send(sock, "RET.", strlen("RET."), 0) != strlen("RET.")) {
        fprintf(stderr, "Send file size failure");
        return 1;
    }

    wait_ACK(sock);
    memset(&buffer, '\0', BUFFERSIZE);
    snprintf(buffer, sizeof(buffer), "%d", sz);

    if (send(sock, buffer, strlen(buffer), 0) != strlen(buffer)) {
        fprintf(stderr, "Send file size failure\n");
        return 1;
    }

    wait_ACK(sock);
    return 0;
}

int send_file(int sock, int sz, char* file){
    int sent_b = 0;
    while(sent_b < sz) sent_b = sent_b + send(sock, file+sent_b, sz-sent_b, 0);
    return 0;
}

void* thread_request(void* param){
    printf("chainlist is empty\n");
    char buffer[BUFFERSIZE];
    char tmpfile[BUFFERSIZE];
    snprintf(tmpfile, sizeof(tmpfile), "tmpf.t_%d", gettid());
    char* file = tmpfile;

    /* Issue wget command */
    printf("issuing wget for file\n");
    req_T *args = (req_T*)param;
    memset(&buffer, '\0', BUFFERSIZE);
    snprintf(buffer, sizeof(buffer),
             "wget --output-document=%s %s > /dev/null 2>&1",
             file, args->request);
    if(system(buffer)==-1){
        printf("Failed to run command\n" );
        pthread_exit(NULL);
    }

    printf("File received.\n");

    /*Open file*/
    FILE* op;
    op = fopen(file, "r");
    if (op == NULL) {
        printf("Error opening file");
        fflush(stdout);
        pthread_exit(NULL);
    }
    fseek(op, 0L, SEEK_END);
    int sz = ftell(op);
    fseek(op, 0L, SEEK_SET);

    /* Store file */
    char *file_b = malloc(sz + 1);
    fread(file_b, sz, 1, op);
    fclose(op);
    file_b[sz] = 0;

    printf("Relaying File...\n");

    /* Send File */
    if (send_start_return_req(args->sock, sz)!=0) {
        printf("Error sending return request");
        fflush(stdout);
        pthread_exit(NULL);
    }
    if (send_file(args->sock, sz, file_b)!=0) {
        printf("Error sending return file");
        fflush(stdout);
        pthread_exit(NULL);
    }
    printf("File Sent!\n");
    fflush(stdout);

    /*Close Socket*/
    shutdown(args->sock,0);
    printf("Goodbye!\n");
    fflush(stdout);

    /* Clean Up */
    free(file_b);
    memset(&buffer, '\0', BUFFERSIZE);
    snprintf(buffer, sizeof(buffer), "rm -f %s", file);
    system(buffer);
    pthread_exit(NULL);
}

void* thread_relay(void* param){
    srand(time(NULL));
    rel_T *args = (rel_T*)param;
    int next = rand() % args->num_of_stones;
    printf("next SS is  <%s, %d>\n",
           args->stones[next].SSaddr, args->stones[next].SSport);

    /* Create Socket */
    int sock;
    struct sockaddr_in servAddr;
    char buffer[BUFFERSIZE];
    int bytesRcvd;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fprintf(stderr, "Socket failed\n");
        pthread_exit(NULL);
    }

    /* Connect */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(args->stones[next].SSaddr);
    servAddr.sin_port = htons(args->stones[next].SSport);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        fprintf(stderr, "Connect failed\n");
        pthread_exit(NULL);
    }

    /* Send request and confirm */
    if (send(sock, args->request, strlen(args->request), 0)
        != strlen(args->request)) {
        fprintf(stderr, "Sent failure");
        pthread_exit(NULL);
    }
    wait_ACK(sock);

    /* Send stepping stones */
    for(int x = 0; x<args->num_of_stones; x++) {
        if (x != next){
            if (send(sock, &args->stones[x], sizeof(StepStone), 0)
                != sizeof(StepStone)) {
                fprintf(stderr, "Sent failure");
                pthread_exit(NULL);
            }
            wait_ACK(sock);
        }
    }

    /* Send End of Transmission */
    if (send(sock, "EOT.", sizeof("EOT."), 0) != sizeof("EOT.")) {
        fprintf(stderr, "Send EOT. failure");
        pthread_exit(NULL);
    }

    /* Begin to wait for file to return */
    printf("Waiting for file...\n");
    /* Get Returned Packet, set size */
    bytesRcvd=0;
    memset(&buffer, '\0', BUFFERSIZE);
    while (strcmp(buffer, "RET.")!=0) {
        if ((bytesRcvd = recv(sock, buffer, 4, 0)) < 0) {
            fprintf(stderr, "Recv on return. failed");
            pthread_exit(NULL);
        }
    }
    memset(&buffer, '\0', BUFFERSIZE);
    if (send_ACK(sock)!=0){
        fprintf(stderr, "Sending ACK in relay return failed");
        pthread_exit(NULL);
    }

    if ((bytesRcvd = recv(sock, buffer, BUFFERSIZE, 0)) < 0) {
        fprintf(stderr, "Recv on return. failed");
        pthread_exit(NULL);
    }

    int sz = atoi(buffer);
    char *file = malloc(sz + 1);
    file[sz] = '\0';

    if (send_ACK(sock)!=0){
        fprintf(stderr, "Sending ACK in relay return failed");
        pthread_exit(NULL);
    }
    int recv_b = 0;
    while(recv_b < sz) {
        recv_b = recv_b + recv(sock, file+recv_b, sz-recv_b, 0);
        if (recv_b < 0) pthread_exit(NULL);
    }
    printf("Relaying file...\n");
    fflush(stdout);

    /* Relay */
    if (send_start_return_req(args->return_sock, sz)!=0) {
        printf("Error relaying return request");
        fflush(stdout);
        pthread_exit(NULL);
    }
    if (send_file(args->return_sock, sz, file)!=0){
        printf("Error relaying return request file");
        fflush(stdout);
        pthread_exit(NULL);
    }
    printf("Goodbye!\n");
    fflush(stdout);

    /* Shutdown */
    shutdown(args->return_sock, 0);
    fflush(stdout);
    free(file);
    pthread_exit(NULL);
}

int run_ss(int port){
    /* Declare Variables */
    char hostname[BUFFERSIZE];
    memset(&hostname, '\0', BUFFERSIZE);
    gethostname(hostname, sizeof(hostname));
    int servSock, clntSock;
    unsigned short portno = port;
    unsigned int clntLen;
    struct sockaddr_in servAddr, clntAddr;
    int bytesRcvd;
    char buffer[BUFFERSIZE];

    /* Create socket */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Socket Failed\n");
        return -3;
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(portno);

    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        fprintf(stderr, "Bind Failed\n");
        return -3;
    }

    /* Listen for Connection */
    if (listen(servSock, 2) < 0) {
        fprintf(stderr, "Listen Failed\n");
        return -3;
    }
    printf("Running on host: %s\n", hostname);
    printf("Listening on port: %d\n", port);

    clntLen=sizeof(clntAddr);
    for(;;){
        if ((clntSock = accept(servSock,
                               (struct sockaddr *) &clntAddr, &clntLen)) < 0) {
            fprintf(stderr, "Accept Failed\n");
            return -3;
        }

        /* Connection Accepted! Begin recv of data */
        printf("Request from: %s \n", inet_ntoa(clntAddr.sin_addr));

        /* Get and store request */
        memset(&buffer, '\0', BUFFERSIZE);
        if ((bytesRcvd = recv(clntSock, buffer, BUFFERSIZE, 0)) < 0) {
            fprintf(stderr, "Recv failed");
            return -3;
        }
        send_ACK(clntSock);
        char request[strlen(buffer)];
        strcpy(request, buffer);
        printf("Request: %s \n", request);

        /* Get number of Stones and Stone info */
        int num_of_stones = 0;
        StepStone* stones;
        stones = (StepStone*)malloc(MAX_STONES*sizeof(StepStone));
        for(;;){
            if ((bytesRcvd = recv(clntSock, &stones[num_of_stones],
                                  sizeof(StepStone), 0)) < 0) {
                fprintf(stderr, "Recv failed");
                return -3;
            }
            send_ACK(clntSock);

            if (bytesRcvd == sizeof("EOT.")) break;
            num_of_stones++;
            /* Grow Max Stones */
            if (num_of_stones == MAX_STONES){
                stones = (StepStone*)realloc(stones,
                                             MAX_STONES*sizeof(StepStone));
                MAX_STONES = MAX_STONES*2;
            }
        }

        /* Determine next step, Are we the requester or relayer? */
        if (num_of_stones == 0) {
            pthread_t req_thread;
            req_T *args;
            args=(req_T *)malloc(sizeof(req_T));
            strcpy(args->request, request);
            args->sock = clntSock;
            if (pthread_create(&req_thread, NULL, &thread_request,
                               (void*)args) != 0){
                fprintf(stderr, "Can't create thread");
                return -6;
            }
        } else if ( num_of_stones > 0){
            printf("chainlist is \n");
            for (int x = 0; x < num_of_stones; x++){
                printf("<%s, %d>\n", stones[x].SSaddr, stones[x].SSport);
            }
            pthread_t rel_thread;
            rel_T *args;
            args=(rel_T *)malloc(sizeof(rel_T));
            args->num_of_stones = num_of_stones;
            args->stones = stones;
            args->return_sock = clntSock;
            strcpy(args->request, request);
            if (pthread_create(&rel_thread, NULL, &thread_relay,
                               (void*)args) != 0){
                fprintf(stderr, "Can't create thread");
                return -6;
            }
        } else {
            fprintf(stderr, "Error In Chainfile received");
            return -5;
        }

    }
}

int check_int( char* int_to_check){
    for (unsigned int x = 0; x < strlen(int_to_check); x++)
        if (!isdigit(int_to_check[x])) return 1;
    return 0;
}


int main(int argc, char* argv[]) {
    int ec = -1;
    if (argc == 1) /* argc should be 1, 2, or 3 */
    {
        ec = run_ss(DEFAULT_PORT);
    } else if (argc == 2 && strcmp(argv[1], "-h")==0) {
        print_help();
        return 0;
    } else if (argc == 3 && strcmp(argv[1], "-p")==0) {
        if (check_int(argv[2]) == 0){
            int port = atoi(argv[2]);
            if (port > 0 && port < 65535){
                ec = run_ss(port);
            } else {
                ec = -1;
            }
        } else{
            ec = -1;
        }
    } else {
        ec = -2;
    }

    return errorCheck(ec);
}
