/*
 * File:   rnwf_ota_service.c
 * Author: c21415
 *
 * Created on March 13, 2023, 10:44 AM
 */

/* This section lists the other files that are included in this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rnwf_interface.h"
#include "rnwf_net_service.h"
#include "rnwf_ota_service.h"
#include "rnwf_system_service.h"

#include "../../timer/delay.h"

#include "../../../SST25WF080B_functions.h"



#define HTTP_CONTENT_LEN    "Content-Length:"
/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */


RNWF_OTA_CALLBACK_t gOta_CallBack_Handler;

uint8_t ota_buf[OTA_BUF_LEN_MAX];

uint8_t gOta_Http_Req[] = "GET /test1.bin HTTP/1.1\r\n Connection: Keep-Alive\r\n\r\n";


RNWF_OTA_CFG_t gOta_CfgData;
/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

uint8_t prov_buf[OTA_BUF_LEN_MAX];

uint32_t total_rx = 0;

uint32_t otaFileSize = 0;

RNWF_RESULT_t RNWF_OTA_Process(uint32_t socket, uint16_t rx_len)
{
    volatile int32_t result = 0;
    char *tmpPtr;
    static uint32_t flash_addr = 0x0;
    //Parse the First HTTP Response
    if((total_rx > 0) && (total_rx == otaFileSize))
    {
        return RNWF_PASS;
    }

    while(rx_len > 0)
    {
        volatile uint16_t readCnt = (rx_len > OTA_BUF_LEN_MAX)?(OTA_BUF_LEN_MAX):rx_len;
        if((result = RNWF_NET_TCP_SOCK_Read(socket, readCnt, (uint8_t *)ota_buf)) > 0 )
        {
            if(!otaFileSize)
            {
                DBG_MSG_OTA("%s\r\n", ota_buf);
                if((tmpPtr = (uint8_t *)strstr(ota_buf, HTTP_CONTENT_LEN)) != NULL)
                {
                    volatile char *token = strtok(tmpPtr, "\r\n");
                    otaFileSize = strtol((token+sizeof(HTTP_CONTENT_LEN)), NULL, 10);
                    if(gOta_CallBack_Handler)
                        gOta_CallBack_Handler(RNWF_EVENT_DWLD_START, &otaFileSize);
                }
                break;
            }
            total_rx = total_rx + result;
            rx_len -= result;
            DBG_MSG_OTA("Received %lu bytes\r\n", total_rx);
            Sector_Program(flash_addr, ota_buf, result);
            flash_addr += result;
            if(total_rx == otaFileSize)
            {
                if(gOta_CallBack_Handler)
                    gOta_CallBack_Handler(RNWF_EVENT_DWLD_DONE, &total_rx);
                return RNWF_PASS;
            }
        }
        else
        {
                break;
        }
    }    
    return RNWF_PASS;
}

void RNWF_OTA_SOCKET_Callback(uint32_t sock, RNWF_NET_SOCK_EVENT_t event, uint8_t *p_str)
{
    switch(event)
    {
        case RNWF_NET_SOCK_EVENT_CONNECTED:
        {
            DBG_MSG_OTA("Connected to OTA server!\r\n");
            RNWF_NET_TCP_SOCK_Write(sock, strlen((char *)gOta_Http_Req), (uint8_t *)gOta_Http_Req);
        }
        break;

        case RNWF_NET_SOCK_EVENT_DISCONNECTED:
        {
            DBG_MSG_OTA("Close OTA Socket!\r\n");
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_CLOSE, &sock);
            break;
        }
        case RNWF_NET_SOCK_EVENT_READ:
        {
            uint16_t rx_len = *(uint16_t *)p_str;
            RNWF_OTA_Process(sock, rx_len);
            break;
        }
        default:
            break;
                    
    }    
    
}


RNWF_RESULT_t RNWF_OTA_SrvCtrl( RNWF_OTA_SERVICE_t request, void *input)  
{
    RNWF_RESULT_t result = RNWF_FAIL;

    switch(request)
    {
        case RNWF_OTA_ENABLE:
        {
            /* RNWF Application Callback register */
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_SET_SRVC_CALLBACK, RNWF_OTA_SOCKET_Callback);
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_TCP_OPEN, &gOta_CfgData.socket);
        }
        break;

        case RNWF_OTA_CONFIG:
        {
            RNWF_OTA_CFG_t *ota_cfg = (RNWF_OTA_CFG_t *)input;
            gOta_CfgData.socket.bind_type = RNWF_BIND_REMOTE;
            gOta_CfgData.socket.sock_addr = ota_cfg->url;
            gOta_CfgData.socket.sock_port = ota_cfg->port;
            gOta_CfgData.socket.sock_type = RNWF_SOCK_TCP;
            gOta_CfgData.socket.tls_conf  = 0;
            gOta_CfgData.flash_addr = ota_cfg->flash_addr;
            gOta_CfgData.flash_len = ota_cfg->flash_len;
            gOta_CfgData.flash_rd = ota_cfg->flash_rd;
            gOta_CfgData.flash_wr = ota_cfg->flash_wr;
        }
        break;

        case RNWF_OTA_SET_CALLBACK:
        if(input != NULL)
        {
            gOta_CallBack_Handler = (RNWF_OTA_CALLBACK_t)(input);            
            result = RNWF_PASS;
        }
        break;        
            
            
        default:
            break;
        
    }
    return result;
}
