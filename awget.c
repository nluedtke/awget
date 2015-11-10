//==============================================================================
// Name        : awget.c
// Author      : Nicholas
// Version     : 1
// Copyright   : None
// Description : v1: handles the chaining and file request for awget
// Arguments   : This program takes in 3 arguments
//			   : arg1 = URL of file to request
//			   : arg2 = -c or -h (optional)
//                      -c specifies the chainfile
//                      -h prints the help msg
//             : arg3 = Name of chainfile (optional but required if -c is
//                  present)
//
//  This program was written for CS457 at CSU as apart of a CS degree program.
//
//==============================================================================


#include "awget.h"

void print_help(){
    printf("awget program:\n");
    printf("This program will request a file and traverse the stepping stones"
                   "\nspecified by the chainfile to download it.\n");
    printf("Usage: ./awget <URL> [options]\n");
    printf("\nREQUIRED ARGUMENTS\n");
    printf("<URL> Specifies the file to download.\n");
    printf("\nOPTIONAL FLAGS\n");
    printf("-h Prints the help message.\n");
    printf("-c <chainfile> Specifies the chainfile location\n");
    printf("\nUsing this program with no options will attempt to find the "
                   "chainfile\nin the working directory as 'chaingang.txt'\n");
}

int errorCheck(int ec) {
    switch (ec) { //Sets up switch to detect type of error.
        case 0:
            return 0;
        case -1: //File issue
            fprintf(stderr, "Invalid Chainfile\n");
            break;
        case -2: //Argument issue
            fprintf(stderr, "Arguments not correct\n\n");
            print_help();
            break;
        case -3:
            fprintf(stderr, "awget failed to connect\n");
            break;
        case -4:
            fprintf(stderr, "awget failed to send\n");
            break;
        case -10:
            fprintf(stderr, "File Requested invalid!\n");
            break;
        default:
            fprintf(stderr, "Unknown error.\n");
    }
    return -1;
}

const char* get_filename(const char* request){
    int last = 0;
    for(int i=0; i<strlen(request);i++) {
        if (request[i] == '/') last = i;
    }
    if (last == 0 || last == strlen(request)-1){
        return "index.html";
    } else {
        return request+last+1;
    }
}

int main(int argc, char* argv[]) {

    char *request;
    char *chainfile = "chaingang.txt";

    /* Check Arguments */
    if (argc == 2 && strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    } else if (argc == 2) {
        request = argv[1];
    } else if (argc == 4 && strcmp(argv[2], "-c") == 0) {
        request = argv[1];
        chainfile = argv[3];
    } else {
        return errorCheck(-2);
    }

    /* Check If file is able to be read */
    if (access(chainfile, R_OK) != 0) {
        return errorCheck(-1);
    }

    /* Open File */
    FILE *fp;
    fp = fopen(chainfile, "r");
    if (fp == NULL) return errorCheck(-1);

    /* Get Number of Stones */
    int num_step_stone;
    if (fscanf(fp, "%d", &num_step_stone) != 1) return errorCheck(-1);
    printf("<%d>\n", num_step_stone);
    StepStone stones[num_step_stone];

    /* Read File*/
    for (int x = 0; x < num_step_stone; x++) {
        if (fscanf(fp, "%s %d", stones[x].SSaddr, &stones[x].SSport) != 2)
            return errorCheck(-1);
        printf("<%s, %d>\n", stones[x].SSaddr, stones[x].SSport);
    }

    /* Init Random Seed */
    srand(time(NULL));

    /* Determine next SS */
    int next = rand() % num_step_stone;
    printf("next SS is  <%s, %d>\n", stones[next].SSaddr, stones[next].SSport);

    /* Create Socket */
    int sock;
    struct sockaddr_in servAddr;
    char buffer[BUFFERSIZE];
    int bytesRcvd;

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Socket failed\n");
        return errorCheck(-3);
    }

    /* Connect */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(stones[next].SSaddr);
    servAddr.sin_port = htons(stones[next].SSport);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        fprintf(stderr, "Connect failed\n");
        return errorCheck(-3);
    }

    /* Send request and confirm */
    if (send(sock, request, strlen(request), 0) != strlen(request)) {
        fprintf(stderr, "Sent failure");
        return errorCheck(-4);
    }
    memset(&buffer, '\0', BUFFERSIZE);
    if ((bytesRcvd = recv(sock, buffer, BUFFERSIZE, 0)) < 0) {
        fprintf(stderr, "Recv ACK. failed");
        return errorCheck(-3);
    }
    if (strcmp(buffer, "ACK.") != 0) {
        fprintf(stderr, "Request send failed");
        return errorCheck(-4);
    }

    /* Send stepping stones */
    for (int x = 0; x < num_step_stone; x++) {
        if (x != next) {
            if (send(sock, &stones[x], sizeof(StepStone), 0) !=
                sizeof(StepStone)) {
                fprintf(stderr, "Sent failure");
                return errorCheck(-4);
            }
            memset(&buffer, '\0', BUFFERSIZE);
            if ((bytesRcvd = recv(sock, buffer, BUFFERSIZE, 0)) < 0) {
                fprintf(stderr, "Recv ACK. failed");
                return errorCheck(-3);
            }
            if (strcmp(buffer, "ACK.") != 0) {
                fprintf(stderr, "Stone send failed");
                return errorCheck(-4);
            }
        }
    }

    /* Send End of Transmission */
    if (send(sock, "EOT.", sizeof("EOT."), 0) != sizeof("EOT.")) {
        fprintf(stderr, "Send EOT. failure");
        return errorCheck(-4);
    }

    /* Begin to wait for file to return */
    printf("Waiting for file...\n");
    fflush(stdout);

    /* Get Returned Packet, set size */
    bytesRcvd = 0;
    memset(&buffer, '\0', BUFFERSIZE);
    while (strcmp(buffer, "RET.")!=0) {
        if ((bytesRcvd = recv(sock, buffer, 4, 0)) < 0) {
            fprintf(stderr, "Recv on return. failed");
            return errorCheck(-5);
        }
    }

    memset(&buffer, '\0', BUFFERSIZE);
    if (send(sock, "ACK.", sizeof("ACK."), 0) != sizeof("ACK.")) {
        return errorCheck(-4);
    }
    if ((bytesRcvd = recv(sock, buffer, BUFFERSIZE, 0)) < 0) {
        fprintf(stderr, "Recv on return. failed");
        return errorCheck(-5);
    }

    int sz = atoi(buffer);
    if (sz == 0) return errorCheck(-10);
    char *file = malloc(sz);

    /* Set filename */
    const char* filename = get_filename(request);
    fp = fopen(filename, "w");
    if (fp == NULL) return errorCheck(-6);


    if (send(sock, "ACK.", sizeof("ACK."), 0) != sizeof("ACK.")) {
        fprintf(stderr, "Send ACK. on return failure");
        return errorCheck(-4);
    }

    int recv_b = 0;

    while(recv_b < sz) {
        recv_b = recv_b + recv(sock, file+recv_b, sz-recv_b, 0);
        if (recv_b < 0) return errorCheck(-5);
    }

    printf("Received File %s\n", filename);
    fflush(stdout);

    fwrite(file, sz, 1, fp);
    fclose(fp);
    printf("Goodbye!\n");
    fflush(stdout);
    free(file);

    return errorCheck(0);
}