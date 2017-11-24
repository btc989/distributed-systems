#include "clientServerThreads.h"

void *client_thread (void *args)
{
    int socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *hp;
    char host_name [MAX_LINE_SIZE];
    int host_port_no;
    struct server_address server_table [MAX_SERVERS];
    int server_count;
    char send_line [MAX_LINE_SIZE];
    char work_c_string [MAX_LINE_SIZE];
    char caller [MAX_LINE_SIZE];
    int i;
    int n;

    gethostname (host_name, sizeof (host_name));
    sprintf (caller, "    Client [%s]", host_name);
    printf ("    Client [HOST=%s PID=%d]: ready\n", host_name, getpid ());

    /* Don't start until server is ready */
    while (SERVER_NOT_READY)
    {
        mutex_lock (caller);
        if (*my_server_ready)
        {
            break;
        }
        mutex_unlock (caller);
    }
    /* Don't start until client is ready */
    server_count = get_server_addresses (server_table);
    for (i = 0; i < server_count; i ++)
    {
        if (strcmp (host_name, server_table [i].host_name) == 0)
        {
            host_port_no = server_table [i].port_no;
            break;
        }
    }
    mutex_unlock (caller);

    srand (host_port_no);

    while (LOOP_FOREVER)
    {
        printf ("    Client [HOST=%s PID=%d]: ENTERING NON-CRITICAL SECTION\n", host_name, getpid ());
        usleep (2000 * (1 + rand () % 1000));
        printf ("    Client [HOST=%s PID=%d]: LEAVING NON-CRITICAL SECTION\n", host_name, getpid ());

        /* THE REQUEST PART OF YOUR PRE-PROTOCOL CODE GOES HERE! */
        /*ADDDED */
        mutex_lock(caller);
            *my_highest_ticket_no = *(int*)my_highest_ticket_no +1;
            *my_ticket_no = my_highest_ticket_no;
        mutex_unlock(caller);

        //create request to be sent
        strcpy(send_line, "request ");
        strcat(send_line, host_name);
        strcat(send_line, " ");
        
        sprintf (work_c_string, "%d", host_port_no);
        strcat(send_line, work_c_string);

        sprintf (work_c_string, "%d", my_ticket_no);
        strcat(send_line, work_c_string);
        strcat(send_line, "\0");

        printf("TEST::CLIENT:: aafter request creation %s \n",send_line);
        //loop through servers sending out request to all
        //except own server
        mutex_lock (caller);
        printf("TEST::CLIENT:: before for-loop %d \n",server_count);
        for (i = 0; i < server_count; i++)
        {
            printf("TEST::CLIENT:: in for-loop %d \n",server_count);
            printf("TEST::CLIENT:: in for '%s'  '%s' \n",host_name,server_table [i].host_name );
            if (strcmp (host_name, server_table[i].host_name) != 0)
            {
                
                //open socket
                //connect to remote server

                if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    printf("socket ERROR in main");
                    exit(1);
                }

                memset( & server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                hp =gethostbyname( server_table [i].host_name);
                if (hp == (struct hostent * ) NULL) {
                    printf("gethostbyname ERROR in main: %s does not exist", server_table [i].host_name);
                    exit(1);
                }
                
                memcpy( & server_addr.sin_addr, hp -> h_addr, hp -> h_length);
                server_addr.sin_port =  htons(TCP_PORTNO);

                

                if (connect(socket_fd, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                    printf("connect ERROR in main");
                    exit(1);
                }

                //send request message
                n = strlen(send_line);
                if ((i = write_n(socket_fd, send_line, n)) != n) {
                    printf("ERROR: could not send to server");
                    exit(1);
                }
                else
                    printf("Request Sent to server %s   \n",server_table [i].host_name);


                //close socket
                close(socket_fd);
            }
        }
        mutex_unlock (caller);
        printf("TEST::CLIENT:: unlock ");
        /*END OF ADDED*/

        printf ("    Client [HOST=%s PID=%d]: WAITING TO ENTER CRITICAL SECTION\n", host_name, getpid ());
        while (AWAIT_REPLIES)
        {

            /* THE AWAIT REPLIES PART OF YOUR PRE-PROTOCOL CODE GOES HERE! */
            /*ADDED*/
            mutex_lock (caller); 
            //check shared variable
            if(*my_replies==server_count-1){
                 //AWAIT_REPLIES=1;
                *my_replies=0;
                break;
            }
                
            mutex_unlock (caller);
            /*END OF ADDED */

        }
        printf ("    Client [HOST=%s PID=%d]: ENTERING CRITICAL SECTION\n", host_name, getpid ());
        write_to_history_file (host_name, host_port_no, my_ticket_no);
        printf ("    Client [HOST=%s PID=%d]: LEAVING CRITICAL SECTION\n", host_name, getpid ());

        /* THE DEFERRED REPLIES OF YOUR POST-PROTOCOL CODE GOES HERE! */
        /*ADDED */
//create request to be sent
        mutex_lock (caller);
        printf("TEST::CLIENT:: got lock in critical section");
        if(my_deferred_count != -1)
            *my_deferred_count = get_server_addresses (my_deferred_table);
        printf("TEST::CLIENT:: after deferred count");
        for (i = 0; i < my_deferred_count; i ++)
        {
            strcpy(send_line, "reply ");
            strcat(send_line, host_name);
            strcat(send_line, " ");
            
            sprintf (work_c_string, "%d", my_deferred_table[i].port_no);
            strcat(send_line, work_c_string);
            strcat(send_line, "\0");

            //open socket
                //connect to remote server

                if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    printf("socket ERROR in main");
                    exit(1);
                }

                memset( & server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                hp = my_deferred_table[i].host_name;
                if (hp == (struct hostent * ) NULL) {
                    printf("gethostbyname ERROR in main: %s does not exist", server_table [i].host_name);
                    exit(1);
                }
                memcpy( & server_addr.sin_addr, hp -> h_addr, hp -> h_length);
                server_addr.sin_port = my_deferred_table[i].port_no;

                if (connect(socket_fd, (struct sockaddr * ) & server_addr, sizeof(server_addr)) < 0) {
                    printf("connect ERROR in main");
                    exit(1);
                }

                //send request message
                n = strlen(send_line);
                if ((i = write_n(socket_fd, send_line, n)) != n) {
                    printf("ERROR: could not send to server");
                    exit(1);
                }
                else
                    printf("Reply Sent to  defered server %s",my_deferred_table[i].host_name);
                    
                //close socket
                close(socket_fd);
        }
        my_deferred_count = 0;
        mutex_unlock (caller);
        
        /*END OF ADDED*/
    }

    exit (0);
}

int get_server_addresses (struct server_address server_table [])
{
    FILE *server_address_file;
    char char_port_no [MAX_LINE_SIZE];
    int int_port_no;
    int server_count = 0;
    int lock;
    struct flock key;

    key.l_type = F_WRLCK;
    key.l_whence = SEEK_SET;
    key.l_start = 0;
    key.l_len = 0;
    key.l_pid = getpid ();
    lock = open ("server_address_file_lock", O_WRONLY);
    fcntl (lock, F_SETLKW, &key);

    server_address_file = fopen ("serverAddressFile", "r");
    if (server_address_file == (FILE *) NULL)
    {
        printf ("    Client: fopen failed for serverAddressFile read");
        exit (1);
    }

    while (fscanf (server_address_file, "%s", server_table [server_count].host_name) == 1)
    {
        fscanf (server_address_file, "%s", char_port_no);
        int_port_no = atoi (char_port_no);
        server_table [server_count].port_no = int_port_no;
        server_count ++;
    }

    fclose (server_address_file);

    key.l_type = F_UNLCK;
    fcntl (lock, F_SETLK, &key);
    close (lock);

    return (server_count);
}

void write_to_history_file (char host_name [], int host_port_no, int *my_ticket_no)
{
    FILE *history_file;

    history_file = fopen ("historyFile", "a");
    if (history_file == (FILE *) NULL)
    {
        printf ("    Client: fopen failed for historyFile append");
        exit (1);
    }

    fprintf (history_file, "%s\n", host_name);
    fprintf (history_file, "%d\n", host_port_no);
    fprintf (history_file, "%d\n\n", *my_ticket_no);

    fclose (history_file);

    return;
}
