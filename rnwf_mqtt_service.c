/*
 * File:   rnwf_mqtt_service.c
 * Author: c21415
 *
 * Created on March 13, 2023, 10:44 AM
 */

/* This section lists the other files that are included in this file.
 */
#include <stdio.h>
#include <string.h>

#include "rnwf_interface.h"
#include "rnwf_mqtt_service.h"
#include "rnwf_net_service.h"
#include "rnwf_system_service.h"




/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
RNWF_MQTT_CALLBACK_t gMqtt_CallBack_Handler;

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

RNWF_RESULT_t RNWF_MQTT_SrvCtrl( RNWF_MQTT_SERVICE_t request, void *input)  
{
RNWF_RESULT_t result = RNWF_FAIL;

    switch(request)
    {
        case RNWF_MQTT_CONFIG:
        {
            RNWF_MQTT_CFG_t *mqtt_cfg = (RNWF_MQTT_CFG_t *)input;  
            if(mqtt_cfg->tls_idx != 0)
            {
                result = RNWF_NET_SOCK_SrvCtrl(mqtt_cfg->tls_idx, mqtt_cfg->tls_conf);                                     
                result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_SET_TLS_CONF, mqtt_cfg->tls_idx);                             
            }
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_SET_BROKER_URL, mqtt_cfg->url);
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_SET_BROKER_PORT, mqtt_cfg->port);
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_SET_CLIENT_ID, mqtt_cfg->clientid);
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_SET_USERNAME, mqtt_cfg->username);
            break;
        }           
        case RNWF_MQTT_CONNECT:
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_CONNECT);     
            break;
        case RNWF_MQTT_RECONNECT:
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_RECONNECT);     
            break;            
        case RNWF_MQTT_DISCONNECT:
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_DISCONNECT);     
            break;
        
        case RNWF_MQTT_PUBLISH:
        {
            RNWF_MQTT_FRAME_t *mqtt_frame = (RNWF_MQTT_FRAME_t *)input;
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_PUBLISH, mqtt_frame->isNew, mqtt_frame->qos, mqtt_frame->isRetain, mqtt_frame->topic, mqtt_frame->message);     
        }
        break;            
        
        case RNWF_MQTT_SUBSCRIBE_QOS0:
        {
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_SUBSCRIBE_QOS0, (const char *)input);
        }
        break;
        case RNWF_MQTT_SUBSCRIBE_QOS1:
        {
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_SUBSCRIBE_QOS1, (const char *)input);
        }
        break;
        case RNWF_MQTT_SUBSCRIBE_QOS2:
        {            
            result = RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_MQTT_CMD_SUBSCRIBE_QOS2, (const char *)input);     
        }        
        break;            
        
        case RNWF_MQTT_SET_CALLBACK:
            if(input != NULL)
            {
                gMqtt_CallBack_Handler = (RNWF_MQTT_CALLBACK_t)(input);            
                result = RNWF_PASS;
            }
            break;
            
            
        default:
            break;
        
    }
    return result;
}
