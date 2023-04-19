/*
 * File:   rnwf_ota_service.c
 * Author: c21415
 *
 * Created on March 13, 2023, 10:44 AM
 */

/* This section lists the other files that are included in this file.
 */
#include <stdio.h>
#include <string.h>

#include "rnwf_interface.h"
#include "rnwf_net_service.h"
#include "rnwf_ota_service.h"
#include "rnwf_system_service.h"




/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
RNWF_OTA_CALLBACK_t gOta_CallBack_Handler;

uint8_t gOta_Http_Req[] = "GET /ota_config.xml HTTP/1.1\r\n Connection: close\r\n\r\n";


RNWF_OTA_CFG_t gOta_CfgData;
/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

RNWF_RESULT_t RNWF_OTA_Process(uint32_t socket, uint16_t rx_len) {
            
    uint8_t prov_buf[OTA_BUF_LEN_MAX];
    
    
    if(RNWF_NET_SOCK_Read(socket, OTA_BUF_LEN_MAX, (uint8_t *)prov_buf, RNWF_BINARY_MODE) == RNWF_PASS)
    {
        
        
    }        
    return RNWF_PASS;
}

void RNWF_OTA_SOCKET_Callback(uint32_t sock, RNWF_NET_SOCK_EVENT_t event, uint8_t *p_str)
{        
    switch(event)
    {
        case RNWF_NET_SOCK_EVENT_CONNECTED:  
        {                                    
            RNWF_NET_TCP_SOCK_Write(sock, strlen((char *)gOta_Http_Req), (uint8_t *)gOta_Http_Req);            
        }
        break;
        
        case RNWF_NET_SOCK_EVENT_DISCONNECTED:
        {
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_CLOSE, &sock);
            break;
        }
        case RNWF_NET_SOCK_EVENT_READ:
        {        
#ifdef PROV_WEB_SERVER               
            HTTP_REQ_Parser(sock, *(uint16_t *)p_str);
#endif
#ifdef PROV_MOBILE_APP  
            
        uint16_t rx_len = *(uint16_t *)p_str;                    
        RNWF_OTA_Process(sock, rx_len);
        
    
#endif            
            
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