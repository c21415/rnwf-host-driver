/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    http.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../../timer/delay.h"

#include "rnwf_interface.h"
#include "rnwf_net_service.h"
#include "rnwf_wifi_service.h"
#include "rnwf_provision_service.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */
#ifdef PROV_MOBILE_APP 

RNWF_PROV_CALLBACK_t gProv_CallBack_Handler; 

/* TCP Socket */
RNWF_NET_SOCKET_t provision_socket = {
        .bind_type = RNWF_BIND_LOCAL,
        .sock_port = 80,
        .sock_type = RNWF_SOCK_TCP,
        };


/* Parse Wi-Fi configuration file */
/* Format is APP_WIFI_PROV_WIFI_CONFIG_ID,<SSID>,<AUTH>,<PASSPHRASE>*/
RNWF_RESULT_t RNWF_PROV_Parse(uint8_t *wifiCofnig, RNWF_WIFI_PARAM_t *wifi_config)
{
    char* p;    
    RNWF_RESULT_t ret = RNWF_PASS;
    
    p = strtok((char *)wifiCofnig, ",");
    if (p != NULL && !strncmp(p, APP_WIFI_PROV_CONFIG_ID, strlen(APP_WIFI_PROV_CONFIG_ID))) 
    {
        p = strtok(NULL, ",");
        if (p)
            wifi_config->ssid = p;

        p = strtok(NULL, ",");
        if (p) 
        {
//            uint8_t security = (RNWF_WIFI_SECURITY_t)atoi(p);
#if 0            
            switch(security)
            {
                case 0:
                case 1:
                case 2:    
                case 3:
            }
#endif            
            wifi_config->security = (RNWF_WIFI_SECURITY_t)atoi(p);
            
            if (RNWF_OPEN < wifi_config->security &&  wifi_config->security < RNWF_WPA3)
            {
                p = strtok(NULL, ",");
                if (p) 
                    wifi_config->passphrase =  p;
                else
                    ret = RNWF_FAIL;
            } 
            else if (wifi_config->security == RNWF_OPEN)
                wifi_config->passphrase = NULL;
            else
                ret = RNWF_FAIL;
        }
        else
            ret = RNWF_FAIL;

        DBG_MSG_PROV("SSID:%s - PASSPHRASE:%s - AUTH:%d\r\n", 
                            wifi_config->ssid, 
                            wifi_config->passphrase, 
                            wifi_config->security);
    }
    else if(p != NULL && !strncmp(p, APP_WIFI_PROV_DONE_ID, strlen(APP_WIFI_PROV_DONE_ID)))
    {
        
        
    }
    return ret;
}

RNWF_RESULT_t RNWF_PROV_Process(uint32_t socket, uint16_t rx_len) {
        
    RNWF_WIFI_PARAM_t wifiConfig;
    uint8_t prov_buf[PROV_BUF_LEN_MAX];
    
    if(RNWF_NET_TCP_SOCK_Read(socket, rx_len, (uint8_t *)prov_buf) > 0)
    {
        if(RNWF_PROV_Parse(prov_buf, &wifiConfig) == RNWF_PASS)
        {
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_CLOSE, &socket);
            wifiConfig.mode = RNWF_WIFI_MODE_STA;
            wifiConfig.autoconnect = 1;
            if(gProv_CallBack_Handler)
                gProv_CallBack_Handler(RNWF_PROV_COMPLTE, (uint8_t *)&wifiConfig);
            return RNWF_PASS;
        }
        else
        {
            if(gProv_CallBack_Handler)
                gProv_CallBack_Handler(RNWF_PROV_FAILURE, NULL);
        }
    }
    return RNWF_FAIL;
}


void RNWF_PROV_SOCKET_Callback(uint32_t sock, RNWF_NET_SOCK_EVENT_t event, uint8_t *p_str)
{        
    switch(event)
    {
        case RNWF_NET_SOCK_EVENT_CONNECTED:  
        {            
            break;
        }
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
        RNWF_PROV_Process(sock, rx_len);
        
    
#endif            
            
        }
        default:
            break;
                    
    }    
    
}

void RNWF_PROV_WIFI_Callback(RNWF_WIFI_EVENT_t event, uint8_t *p_str)
{            
    switch(event)
    {
        case RNWF_DHCP_DONE:
            printf("DHCP IP:%s\n", &p_str[2]);
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_TCP_OPEN, &provision_socket);
            
            break;
        case RNWF_SCAN_INDICATION:
        {
#ifdef PROV_WEB_SERVER              
            uint16_t resp_len = strlen((char *)p_str);
            uint16_t buff_len = strlen((char *)&http_buffer[HTTP_HDR_OFFSET])+HTTP_HDR_OFFSET;
            // reuse the http buffer with a offset to hold header info 
            // sanity check for buffer over flow            
            if((buff_len + resp_len + 2) < HTTP_BUFFER_LEN)
            {
                strcat((char *)&http_buffer[HTTP_HDR_OFFSET], (char *)p_str);                
                buff_len += resp_len;                
                http_buffer[buff_len] = '|';
                http_buffer[buff_len+1] = '\0';            
            }
#endif            
            break;            
        }
        case RNWF_SCAN_DONE:            
#ifdef PROV_WEB_SERVER  
            if(g_scan_socket)             
                HTTP_RESP_Send(g_scan_socket, HTTP_RESPONSE_HDR, (char *)&http_buffer[HTTP_HDR_OFFSET], "text/html", strlen((char *)&http_buffer[HTTP_HDR_OFFSET]));                
#endif
#ifdef PROV_MOBILE_APP               

#endif            
            
            break;
        default:
            break;
                    
    }    
}

RNWF_RESULT_t RNWF_PROV_SrvCtrl( RNWF_MQTT_SERVICE_t request, void *input)  {
    
    switch(request)
    {
        case RNWF_PROV_ENABLE:
        {                   
            /* RNWF Application Callback register */
            RNWF_WIFI_SrvCtrl(RNWF_WIFI_SET_SRVC_CALLBACK, RNWF_PROV_WIFI_Callback);
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_SET_SRVC_CALLBACK, RNWF_PROV_SOCKET_Callback);

            const char *dhcps_cfg[] = {"192.168.1.1/24", "192.168.1.10"};
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_DHCP_SERVER_ENABLE, dhcps_cfg);  

            /* Wi-Fii Connectivity */
            RNWF_WIFI_PARAM_t wifi_ap_cfg = {RNWF_WIFI_MODE_AP, SOFT_AP_SSID, "", SOFT_AP_SECURITY, 1};    
            RNWF_WIFI_SrvCtrl(RNWF_SET_WIFI_PARAMS, &wifi_ap_cfg);
        }
        break;
        
        case RNWF_PROV_DISABLE:
        {
            /* RNWF Application Callback register */
            RNWF_WIFI_SrvCtrl(RNWF_WIFI_SET_SRVC_CALLBACK, NULL);
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_SET_SRVC_CALLBACK, NULL);
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_DHCP_SERVER_DISABLE, NULL);
        }
        break;
        
        case RNWF_PROV_SET_CALLBACK:
        {
            if(input != NULL)
                gProv_CallBack_Handler = (RNWF_PROV_CALLBACK_t)input;
        }    
        break;
            
        default:
            break;
    }
    
    return RNWF_PASS;
    
}



RNWF_RESULT_t RNWF_PROV_SrvInit(RNWF_PROV_MODE_t provMode)  {
        
    
    
#ifdef PROV_MOBILE_APP                   
    if(provMode == RNWF_PROV_MOBILE_APP)
    {
        

        
    }
#endif
    
    return RNWF_PASS;
           
}
#endif
/* *****************************************************************************
 End of File
 */
