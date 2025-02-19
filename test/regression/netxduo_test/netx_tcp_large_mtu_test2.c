/* This NetX test concentrates initial cwnd size with large MTU.  */

#include   "nx_api.h"
extern void    test_control_return(UINT status);

#if !defined(NX_DISABLE_IPV4)
#define     DEMO_STACK_SIZE         2048


/* Define the ThreadX and NetX object control blocks...  */

static TX_THREAD               thread_0;
static TX_THREAD               thread_1;

static NX_PACKET_POOL          pool_0;
static NX_IP                   ip_0;
static NX_IP                   ip_1;
static NX_TCP_SOCKET           client_socket;
static NX_TCP_SOCKET           server_socket;



/* Define the counters used in the test application...  */

static ULONG                   error_counter;


/* Define thread prototypes.  */

static void    thread_0_entry(ULONG thread_input);
static void    thread_1_entry(ULONG thread_input);
extern void    _nx_ram_network_driver_1500(struct NX_IP_DRIVER_STRUCT *driver_req);
extern void    _nx_ram_network_driver_3000(struct NX_IP_DRIVER_STRUCT *driver_req);


/* Define what the initial system looks like.  */

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_large_mtu_test_2_application_define(void *first_unused_memory)
#endif
{

CHAR   *pointer;
UINT    status;

    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;


    /* Create the main thread.  */
    tx_thread_create(&thread_1, "thread 1", thread_1_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;


    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a packet pool.  */
    status =  nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", 256, pointer, 2048);
    pointer = pointer + 2048;

    if (status)
        error_counter++;

    /* Create an IP instance.  */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(1, 2, 3, 4), 0xFFFF0000UL, &pool_0, _nx_ram_network_driver_3000,
                    pointer, 2048, 1);
    pointer =  pointer + 2048;

    /* Create another IP instance.  */
    status += nx_ip_create(&ip_1, "NetX IP Instance 1", IP_ADDRESS(1, 2, 3, 5), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_1500,
                    pointer, 2048, 2);
    pointer =  pointer + 2048;
    if (status)
        error_counter++;

    /* Enable ARP and supply ARP cache memory for IP Instance 0.  */
    status =  nx_arp_enable(&ip_0, (void *) pointer, 1024);
    pointer = pointer + 1024;
    if (status)
        error_counter++;

    /* Enable ARP and supply ARP cache memory for IP Instance 1.  */
    status  =  nx_arp_enable(&ip_1, (void *) pointer, 1024);
    pointer = pointer + 1024;
    if (status)
        error_counter++;

    /* Enable TCP processing for both IP instances.  */
    status =  nx_tcp_enable(&ip_0);
    status += nx_tcp_enable(&ip_1);

    /* Check TCP enable status.  */
    if (status)
        error_counter++;
}


/* Define the test threads.  */

static void    thread_0_entry(ULONG thread_input)
{
UINT    status;
ULONG   tcp_transmit_window;
ULONG   mss;

    /* Print out some test information banners.  */
    printf("NetX Test:   TCP Large MTU Test 2......................................");

    /* Check for earlier error.  */
    if (error_counter)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }


    /* Create the client socket. */
    status = nx_tcp_socket_create(&ip_0, &client_socket, "Client Socket", 
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 65535,
                                  NX_NULL, NX_NULL);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Bind the socket.  */
    status = nx_tcp_client_socket_bind(&client_socket, 12, 5 * NX_IP_PERIODIC_RATE);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Connect to server. */
    status = nx_tcp_client_socket_connect(&client_socket, IP_ADDRESS(1, 2, 3, 5), 12, 5 * NX_IP_PERIODIC_RATE);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }

    /* Get TCP transmit window size. */
    status = nx_tcp_socket_info_get(&client_socket, NX_NULL, NX_NULL,
                                    NX_NULL, NX_NULL,
                                    NX_NULL, NX_NULL,
                                    NX_NULL, NX_NULL,
                                    NX_NULL, &tcp_transmit_window,
                                    NX_NULL);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }

    /* Verify the cwnd is 3MSS. */
    if ((tcp_transmit_window != client_socket.nx_tcp_socket_connect_mss * 3) ||
        (tcp_transmit_window != client_socket.nx_tcp_socket_tx_window_congestion))
    {
        error_counter++;
    }

    /* Get client MSS. */
    status = nx_tcp_socket_mss_get(&client_socket, &mss);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }

    /* Verify connection MSS. */
    if (mss != 1460)
    {
        error_counter++;
    }
    
    /* Disconnect from server. */
    status =  nx_tcp_socket_disconnect(&client_socket, 5 * NX_IP_PERIODIC_RATE);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Unbind the client socket. */
    status =  nx_tcp_client_socket_unbind(&client_socket);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Delete the client socket. */
    status =  nx_tcp_socket_delete(&client_socket);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    if (error_counter)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }
    else
    {

        printf("SUCCESS!\n");
        test_control_return(0);
    }
}


static void    thread_1_entry(ULONG thread_input)
{
UINT    status;
ULONG   mss;

    /* Create the server socket. */
    status = nx_tcp_socket_create(&ip_1, &server_socket, "Server Socket", 
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 65535,
                                  NX_NULL, NX_NULL);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Listen the socket.  */
    status = nx_tcp_server_socket_listen(&ip_1, 12, &server_socket, 5, NX_NULL);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Accept connection from client. */
    status = nx_tcp_server_socket_accept(&server_socket, 5 * NX_IP_PERIODIC_RATE);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }

    /* Get client MSS. */
    status = nx_tcp_socket_mss_get(&server_socket, &mss);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }

    /* Verify connection MSS. */
    if (mss != 1460)
    {
        error_counter++;
    }

    /* Verify the cwnd is 3MSS. */
    if (server_socket.nx_tcp_socket_tx_window_congestion != server_socket.nx_tcp_socket_connect_mss * 3)
    {
        error_counter++;
    }
    
    /* Disconnect from client. */
    status = nx_tcp_socket_disconnect(&server_socket, 5);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Unaccept the server socket. */
    status = nx_tcp_server_socket_unaccept(&server_socket);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Setup server socket for listening again. */
    status = nx_tcp_server_socket_relisten(&ip_1, 12, &server_socket);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Unlisten on the server port. */
    status =  nx_tcp_server_socket_unlisten(&ip_1, 12);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
    
    /* Delete the client socket. */
    status = nx_tcp_socket_delete(&server_socket);
    
    /* Check for error */
    if (status)
    {
        error_counter++;
    }
}
#else
#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_large_mtu_test_2_application_define(void *first_unused_memory)
#endif
{

    /* Print out test information banner.  */
    printf("NetX Test:   TCP Large MTU Test 2......................................N/A\n");

    test_control_return(3);

}
#endif /* __PRODUCT_NETXDUO__ */
