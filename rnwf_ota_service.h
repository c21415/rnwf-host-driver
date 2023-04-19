/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */


/*! \page ota_service OTA Service
This page is for advanced users.
*/


// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef RNWF_OTA_SERVICE_H
#define	RNWF_OTA_SERVICE_H

#include <xc.h> // include processor files - each processor file is guarded.  

#define DBG_MSG_OTA(args, ...)    printf("[OTA]:"args, ##__VA_ARGS__)

#define OTA_BUF_LEN_MAX         128

/**
 @defgroup OTA_GRP OTA API
 @{
 */


#define OTA_SOCK_ID     gOta_CfgData.socket

/**
 @brief OTA modes
 
 */
typedef enum 
{
    RNWF_OTA_MODE_HTTP,     /**<FW file is from HTTP server */
    RNWF_OTA_MODE_USB,      /**<FW file is from Host USB */
    RNWF_OTA_MODE_UART,     /**<FW file is from Host UART*/    
}RNWF_OTA_MODES_t;


/**
 @brief OTA Service lists
 
 */
typedef enum
{
    RNWF_OTA_ENABLE,                /**<Connected to MQTT broker event */
    RNWF_OTA_DISABLE,               /**<Connected to MQTT broker event */
    RNWF_OTA_CONFIG,                /**<Connected to MQTT broker event */
    RNWF_OTA_SET_CALLBACK,          /**<Register OTA application callback*/
    RNWF_OTA_SET_SRVC_CALLBACK,
    RNWF_OTA_AUTO_MODE,             /**<OTA Auto mode, triggered reqularly*/  
}RNWF_OTA_SERVICE_t;

/**
 @brief OTA Service lists
 
 */
typedef enum
{
    RNWF_EVENT_MAKE_UART,   /**<Change to UART mode */     
     
}RNWF_OTA_EVENT_t;


/**
 @brief OTA GPIO Control Function
 
 */
typedef void (*RNWF_OTA_GPIO_CTRL_FUNC_t)(uint32_t pin, uint8_t output);

/**
 @brief OTA UART Tx Function
 
 */
typedef void (*RNWF_OTA_UART_TX_FUNC_t)(uint8_t *buffer, uint32_t length);

/**
 @brief OTA Flash Write Function
 
 */
typedef void (*RNWF_OTA_FLASH_WR_FUNC_t)(uint8_t *buffer, uint32_t length);

/**
 @brief OTA Flash Read Function
 
 */
typedef void (*RNWF_OTA_DLASH_RD_FUNC_t)(uint8_t *buffer, uint32_t length);

/**
 @brief MQTT Callback Function definition
 
 */
typedef void (*RNWF_OTA_CALLBACK_t)(RNWF_OTA_EVENT_t, uint8_t *);


/**
 @brief Network and Socket service List
 
 */
typedef struct 
{
    RNWF_NET_SOCKET_t socket;                /**<Socket handler for HTTP link*/
    RNWF_OTA_MODES_t mode;                  /**<Active OTA mode */ 
    const char* url;                        /**<HTTP server URL */
    uint16_t port;                          /**<HTTP server port */
    uint32_t        flash_addr;             /**<Flash/NVM address to store received FW*/
    uint32_t        flash_len;              /**<Flash/NVM length allocated for FW*/    
    RNWF_OTA_GPIO_CTRL_FUNC_t gpio_ctrl;    /**<Host function pointer to control PB0/PB1/MCLR */
    RNWF_OTA_UART_TX_FUNC_t   uart_tx;      /**<UART function to Transmit data */ 
    RNWF_OTA_FLASH_WR_FUNC_t  flash_wr;     /**<Flash function to write received FW */ 
    RNWF_OTA_FLASH_WR_FUNC_t  flash_rd;     /**<Flash function to read FW from flash */ 
}RNWF_OTA_CFG_t;


/**
 @brief MQTT Callback Function handler
 
 */
extern RNWF_OTA_CALLBACK_t gOta_CallBack_Handler;


extern uint32_t g_Ota_SocketId;

extern RNWF_OTA_CFG_t gOta_CfgData;


/**
 * @brief MQTT Service Layer API to handle system operations.
 * 
 *
 * @param[in] request       Requested service ::RNWF_OTA_SERVICE_t
 * @param[in] input         Input/Output data for the requested service 
 * 
 * @return RNWF_PASS Requested service is handled successfully
 * @return RNWF_PASS Requested service has failed
 */
RNWF_RESULT_t RNWF_OTA_SrvCtrl( RNWF_OTA_SERVICE_t request, void *input);

#endif	/* XC_HEADER_TEMPLATE_H */

/** @}*/