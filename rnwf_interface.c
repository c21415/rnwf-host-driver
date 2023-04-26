/* ************************************************************************** */
/*! \mainpage RNWF02 Service API User Guide

The RNWF02 AT Command service layer wraps the ASCII
command implementation in a service architecture. The
service layer API's are documented here can enable 
easy and quick applciation development

- \ref SERVICE_GRP "System Service"
- \ref WIFI_GRP "Wi-Fi Service"
- \ref MQTT_GRP "MQTT Service"
- \ref NETSOCK_GRP "Network Socket Service"
*/

/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../timer/delay.h"

#include "rnwf_interface.h"
#include "rnwf_wifi_service.h"
#include "rnwf_net_service.h"
#include "rnwf_mqtt_service.h"
/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */
RNWF_INTERFACE_STATE_t   g_interface_state = RNWF_INTERFACE_FREE;

uint32_t g_interface_timeout = RNWF_INTERFACE_TIMEOUT;

uint8_t g_if_buffer[RNWF_INTERFACE_LEN_MAX];

uint8_t g_async_buffer[RNWF_IF_ASYCN_BUF_MAX];


/* Queue used by the interface bus to full the incoming frames*/
IF_QUEUE_t g_if_free_q;
/* Queue used push the read bytes to process incoming frames*/
IF_QUEUE_t g_if_rx_q;

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */


/* Circular Queue implementation */

/*
 * 
 * @param if_q -> queue pointer
 * 
 */
void if_q_init(IF_QUEUE_t *if_q)
{
    if_q->tail = -1;
    if_q->head = 0;
    if_q->size = 0;
}

/*
 * if_q_full() -> Queue full API
 * @param if_q -> queue pointer
 * 
 * @return true/false
 */
bool if_q_full(IF_QUEUE_t *if_q)
{
    if(if_q->size >= RNWF_IF_BUF_MAX)
        return true;

    return false;
}

/*
 * if_q_empty() -> Queue empty API
 * @param if_q -> queue pointer
 * 
 * @return true/false
 */
bool if_q_empty(IF_QUEUE_t *if_q)
{
    if(if_q->size)
        return false;

    return true;
}

/*
 * if_q_enqueue() -> Queue Enqueue API
 * @param if_q -> queue pointer
 * 
 * @return true/false
 */
bool if_q_enqueue(IF_QUEUE_t *if_q, uint8_t *mem_idx)
{
    
    if(if_q_full(if_q))
    {        
        return false;
    }
        
    if_q->tail = ((if_q->tail + 1) % RNWF_IF_BUF_MAX);                   
    if_q->queue[if_q->tail] = mem_idx;
    if_q->size++;    
    
    return true;
}

/*
 * if_q_dequeue() -> Queue Dequeue API
 * @param if_q -> queue pointer
 * 
 * @return true/false
 */
bool if_q_dequeue(IF_QUEUE_t *if_q, uint8_t **mem_idx)
{        
    if(if_q_empty(if_q))
    {
        return false;
    }       
    
    *mem_idx = if_q->queue[if_q->head]; 
    if_q->head = ((if_q->head + 1) % RNWF_IF_BUF_MAX);  
    if_q->size--;
                                                           
    return true;    
}

/** 
  @Function
    int ExampleLocalFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Description
    Full description, explaining the purpose and usage of the function.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

  @Precondition
    List and describe any required preconditions. If there are no preconditions,
    enter "None."

  @Parameters
    @param param1 Describe the first parameter to the function.
    
    @param param2 Describe the second parameter to the function.

  @Returns
    List (if feasible) and describe the return values of the function.
    <ul>
      <li>1   Indicates an error occurred
      <li>0   Indicates an error did not occur
    </ul>

  @Remarks
    Describe any special behavior not described above.
    <p>
    Any additional remarks.

  @Example
    @code
    if(ExampleFunctionName(1, 2) == 0)
    {
        return 3;
    }
 */

RNWF_RESULT_t RNWF_IF_SW_Reset(void)
{    
    return RNWF_CMD_RSP_Send(NULL, NULL, NULL, "\r\n", NULL);    
}


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

RNWF_RESULT_t RNWF_IF_ASYNC_Handler(uint8_t *p_msg)
{    
    uint16_t arg_len = 0;
    RNWF_RESULT_t result = RNWF_COTN;     
    uint8_t *p_arg = (uint8_t *)(strstr((char *)p_msg, RNWF_ARG_DELIMETER) + 1);
    while(p_arg[arg_len] != '\0')
    {
        (p_arg[arg_len] == ',') ? p_arg[arg_len++] = ' ':arg_len++;        
    }
    RNWF_RESPONSE_Trim(p_arg);
    
#ifdef RNWF_INTERFACE_DEBUG    
    DBG_MSG_IF("Async Message -> %s\n", p_msg);
    DBG_MSG_IF("Async Arguments-> %s\n", p_arg);
#endif
    
    switch(p_msg[0])                        
    {
        /** Wi-Fi DHCP/DNS or Net based Async Events*/
        case 'W':
        case 'D':
        case 'N':
        {            
            for(int i = 0; i < RNWF_WIFI_SERVICE_CB_MAX; i++)
            {
                if(gWifi_CallBack_Handler[i] == 0)
                    continue;
                
                RNWF_WIFI_CALLBACK_t wifi_cb_func = gWifi_CallBack_Handler[i];

                if(strstr((char *)p_msg, RNWF_EVENT_STA_AUTO_IP) || strstr((char *)p_msg, RNWF_EVENT_AP_AUTO_IP))
                {                                
                    wifi_cb_func(RNWF_DHCP_DONE, p_arg);        
                }    
                else if(strstr((char *)p_msg, RNWF_EVENT_LINK_LOSS) || strstr((char *)p_msg, RNWF_EVENT_ERROR))
                {        
                    wifi_cb_func(RNWF_DISCONNECTED, p_arg);         
                }
                else if(strstr((char *)p_msg, RNWF_EVENT_LINK_UP))
                {                
                    wifi_cb_func(RNWF_CONNECTED, p_arg);                     
                }
                else if(strstr((char *)p_msg, RNWF_EVENT_ERROR))
                {                
                    wifi_cb_func(RNWF_CONNECT_FAILED, p_arg);   
                }
                if(strstr((char *)p_msg, RNWF_EVENT_SCAN_IND))
                {
                    wifi_cb_func(RNWF_SCAN_INDICATION, p_arg);   
                }
                if(strstr((char *)p_msg, RNWF_EVENT_SCAN_DONE))
                {                     
                    wifi_cb_func(RNWF_SCAN_DONE, p_arg); 
                    result = RNWF_PASS;
                }    
            }
        }
        break;
        /** Socket based Async Events*/
        case 'S':
        {
            uint32_t socket_id;
            RNWF_NET_SOCK_EVENT_t event;            
                        
            /** Service Layer Handler */
            for(uint8_t i = 0; i < RNWF_NET_SOCK_SERVICE_CB_MAX; i++)
            {
                RNWF_NET_SOCK_CALLBACK_t netSock_cb_func = gSocket_CallBack_Handler[i];
                
                if(netSock_cb_func == NULL)
                    continue;
                
                sscanf((char *)p_arg, "%lu %*s", &socket_id);
                                                            
                if(strstr((char *)p_msg, RNWF_EVENT_SOCK_TLS_DONE))
                {          
                    event = RNWF_NET_SOCK_EVENT_TLS_DONE;
                }
                else if(strstr((char *)p_msg, RNWF_EVENT_SOCK_CONNECTED))
                {                 
                    event = RNWF_NET_SOCK_EVENT_CONNECTED;                    
                }
                else if(strstr((char *)p_msg, RNWF_EVENT_SOCK_CLOSE))
                {                          
                    event = RNWF_NET_SOCK_EVENT_DISCONNECTED;                    
                } 
                else if(strstr((char *)p_msg, RNWF_EVENT_SOCK_TCP_RECV))
                {
                    uint16_t rx_len = 0;                       
                    sscanf((char *)p_arg, "%*lu %u", &rx_len);  
                    p_arg = (uint16_t *)&rx_len;                    
                    event = RNWF_NET_SOCK_EVENT_READ;

                }    

                netSock_cb_func(socket_id, event, p_arg);                    
            }
            
        }
        break;
        /** MQTT based Async Events*/
        case 'M':
        {
            uint32_t status;
            /**No call back then just return */
            if(gMqtt_CallBack_Handler == NULL)
                break;
            
            sscanf((char *)p_arg, "%lu %*s", &status); 
            if(strstr((char *)p_msg, RNWF_EVENT_MQTT_CONNECTED))
            {                
                if(status)
                    gMqtt_CallBack_Handler(RNWF_MQTT_CONNECTED, p_arg);
                else
                    gMqtt_CallBack_Handler(RNWF_MQTT_DISCONNECTED, p_arg);                
            }                        
        }
        break;
        default:
            break;                
        
    }
    
    return result;
}


RNWF_RESULT_t RNWF_RESPONSE_Trim(uint8_t *buffer)
{   
    //if(strchr((char *)buffer, "\n") != NULL)
    {
        int16_t idx = strlen((char *)buffer);
  
        while(idx-- >= 0)
        {
            if(buffer[idx] == '\r')
            {
                buffer[idx] = '\0';
                return RNWF_PASS;    
            }                                 
        }        
        return RNWF_FAIL; 
    }
       
}



RNWF_RESULT_t RNWF_RAW_Write(uint8_t *buffer, uint16_t len)
{        
    while(len > 0)
    {        
        if(UART2.IsTxReady()) 
        {                       
            UART2.Write(*buffer++); 
            while(!UART2.IsTxDone());            
            len--;
        }
        if((len % 450) == 0)
                DELAY_milliseconds(100);
    }    
    return RNWF_PASS;
}


RNWF_RESULT_t RNWF_EVENT_Handler(void)
{   
    uint8_t *ptr_async;         
    
    while(IF_RX_Q_DEQUEUE(&ptr_async))
    {   
        //DBG_MSG_IF("Dequeued Msg = %s\r\n", ptr_async);
        char *token = strtok((char *)ptr_async, "\r+");        
        while(token != NULL)
        {
            DBG_MSG_IF("Async token %s\n", token);
            RNWF_IF_ASYNC_Handler((uint8_t *)token);            
            token = strtok(NULL, "\r+");            
        }
        //RNWF_IF_ASYNC_Handler(ptr_async);
        memset(ptr_async, 0, RNWF_IF_ASYCN_MSG_MAX);
        IF_BUF_Q_ENQUEUE(ptr_async);  
    }
    RNWF_CMD_RSP_Send(NULL, NULL, NULL, NULL); 
    return RNWF_PASS;
}
/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
RNWF_RESULT_t RNWF_CMD_RSP_Send(const char *cmd_complete, const char *delimeter, uint8_t *response, const char *format, ...)
{
    uint8_t *p_frame = g_if_buffer;
    RNWF_RESULT_t result = RNWF_PASS;
    size_t cmd_len, rsp_len = 0;
    uint16_t offset =  0;
    va_list args;   
    //memset(g_interface_send_buffer, 0, RNWF_INTERFACE_LEN_MAX);
    /* Get the variable arguments in va_list */
    //memset(g_if_buffer, 0, RNWF_INTERFACE_LEN_MAX);
    if(format != NULL)
    {
        RNWF_IS_INTERFACE_BUSY();

        va_start( args, format );                  
        cmd_len = vsnprintf((char *)g_if_buffer, RNWF_INTERFACE_LEN_MAX, format, args);        
        va_end(args); 
#ifdef RNWF_INTERFACE_DEBUG        
        DBG_MSG_IF("\nDBG:cmd[%d] -> %s", cmd_len, p_frame);
#endif /* RNWF_INTERFACE_DEBUG */        

        while(*p_frame != '\0')
        {
            if(UART2.IsTxReady()) 
            {
                UART2.Write(*p_frame++); 
                while(!UART2.IsTxDone());
            }
        }
        memset(g_if_buffer, 0, cmd_len);
        if(response != NULL)
            response[0] = '\0';
        
    }
    
    g_interface_timeout = RNWF_INTERFACE_TIMEOUT;
    while(g_interface_timeout--) //later make it timeout
    {        
        if(UART2.IsRxReady())
        {      
            if(rsp_len < RNWF_INTERFACE_LEN_MAX)
            {
                g_if_buffer[rsp_len++] = UART2.Read();                  
            }
            else
            {
                //truncate the buffer till '\r\n'            
                g_if_buffer[rsp_len-3] = g_if_buffer[rsp_len-2];
                g_if_buffer[rsp_len-2] = g_if_buffer[rsp_len-1];
                rsp_len = rsp_len-1;    //
            }
            
            if(strstr((char *)g_if_buffer, "\r\n#")) //Raw mode socket 
            {
                result = RNWF_RAW;
                g_if_buffer[rsp_len-3] = '\0';
                g_if_buffer[rsp_len-2] = '\0';  
                g_if_buffer[rsp_len-1] = '\0';
                break;
            }
            if(strstr((char *)g_if_buffer, "\r\n>"))
            {           
#ifdef RNWF_INTERFACE_DEBUG       
                DBG_MSG_IF("DBG:rsp[%d] <- %.*s\n", rsp_len, rsp_len, g_if_buffer);
#endif /* RNWF_INTERFACE_DEBUG */                     
                g_if_buffer[rsp_len-3] = '\0';
                g_if_buffer[rsp_len-2] = '\0';  
                g_if_buffer[rsp_len-1] = '\0';                  
                if((g_if_buffer[0] == '\r') && (g_if_buffer[1] == '+'))
                {    
                    uint8_t *async_buf;
                    //RNWF_SET_INTERFACE_FREE();
                    //result = RNWF_IF_ASYNC_Handler(g_if_buffer);
                    //RNWF_SET_INTERFACE_BUSY();  
                    if(IF_BUF_Q_DEQUEUE(&async_buf))                        
                    {                                       
                        strcpy((char *)async_buf, (char *)g_if_buffer);
                        IF_RX_Q_ENQUEUE(async_buf);                         
                    }
                    if(response != NULL || format != NULL)
                    {
                        rsp_len = 0;                        
                        continue;
                    }                   
                }                 
                else if(strstr((char *)g_if_buffer, RNWF_AT_ERROR))
                {
                    if(response != NULL)
                    {                    
                        strcpy((char *)response, (char *)g_if_buffer+strlen(RNWF_AT_ERROR));
                    }
                    break;
                    result = RNWF_FAIL;                    
                }                                                                             
                else if(response != NULL)
                {
                    if(delimeter != NULL)
                    {                                        
                        volatile uint8_t *token, *rsp_ptr;

                        rsp_ptr = (uint8_t *)strstr((char *)g_if_buffer, delimeter);                            
                        offset =  strlen(delimeter);
                        token = (uint8_t *)strtok((char *)rsp_ptr, "\r\n");

                        /* walk through other tokens */
                        while( token != NULL ) {                           
                            if(strstr((char *)token, delimeter))
                                strcat((char *)response, (char *)token+offset); 
                                if(strstr((char *)token, "\r\nOK"))
                                {
                                    break;
                                }
                            token = (uint8_t *)strtok(NULL, "\r\n");
                        }
                    }   
                    else
                    {          
                        uint8_t *end_ptr = NULL;
                        if((end_ptr = (uint8_t *)strstr((char *)g_if_buffer, "\r\nOK")) != NULL)
                        {
                            *end_ptr = '\0';                              
                        } 
                        strcat((char *)response, (char *)g_if_buffer);
                    }
                }
                break;
                rsp_len = 0;
            }                                   
        }
    }
    RNWF_SET_INTERFACE_FREE();
    
    return result;
    
    
}



RNWF_RESULT_t RNWF_IF_Init(void)
{    
    if_q_init(&g_if_free_q);
    if_q_init(&g_if_rx_q);
    
    /* Fill up the free queue will all the buffer entries */
    for(uint8_t *pMem_addr = g_async_buffer; pMem_addr < (g_async_buffer + RNWF_IF_ASYCN_BUF_MAX); pMem_addr+=RNWF_IF_ASYCN_MSG_MAX)
    {                
        IF_BUF_Q_ENQUEUE(pMem_addr);        
    }
    
    RNWF_IF_SW_Reset();
    
    return RNWF_PASS;
}

/* *****************************************************************************
 End of File
 */
