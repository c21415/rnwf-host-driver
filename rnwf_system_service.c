/*
 * File:   rnwf_system_service.c
 * Author: c21415
 *
 * Created on March 13, 2023, 10:44 AM
 */

/* This section lists the other files that are included in this file.
 */
#include <stdio.h>
#include <string.h>

#include "rnwf_interface.h"
#include "rnwf_system_service.h"

/* TODO:  Include other files here if needed. */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

RNWF_RESULT_t RNWF_SYSTEM_SrvCtrl(RNWF_SYSTEM_SERVICE_t request, uint8_t *input) {
    switch(request)
    {
        case RNWF_SYSTEM_RESET:
        {
            uint8_t init_msg[128];
            RNWF_CMD_SEND_OK_WAIT(NULL, init_msg, RNWF_RESET_TARGET);
        }   
            break;
        case RNWF_SYSTEM_GET_MAN_ID:
            RNWF_CMD_SEND_OK_WAIT("+GMI:", input, RNWF_GET_MAN_ID);            
            break;
        case RNWF_SYSTEM_GET_CERT_LIST:
            RNWF_CMD_SEND_OK_WAIT("+FS:", input, RNWF_GET_CERT_LIST); 
            break;
        case RNWF_SYSTEM_GET_KEY_LIST:
            RNWF_CMD_SEND_OK_WAIT("+FS:", input, RNWF_GET_KEY_LIST); 
            break;              
        case RNWF_SYSTEM_GET_TIME:
            RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_GET_SYS_TIME); 
            break;            
        case RNWF_SYSTEM_SET_TIME_UNIX:
            RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_SET_SYS_TIME_UNIX, *(uint32_t*)input); 
            break;
        case RNWF_SYSTEM_SET_TIME_NTP:
            RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_SET_SYS_TIME_NTP, *(uint32_t*)input); 
            break;
        case RNWF_SYSTEM_SET_TIME_STRING:
            RNWF_CMD_SEND_OK_WAIT(NULL, NULL, RNWF_SET_SYS_TIME_STRING, (uint8_t*)input); 
            break;            
        default:
            break;
    }
    return RNWF_PASS;
}


/* *****************************************************************************
 End of File
 */
