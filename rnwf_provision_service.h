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

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef HTTP_H
#define	HTTP_H

#include <xc.h> // include processor files - each processor file is guarded.  


#define DBG_MSG_PROV(args, ...)    printf("[PROV]:"args, ##__VA_ARGS__)

#ifdef PROV_WEB_SERVER  

#define HTTP_HDR_OFFSET     128

#define HTTP_BUFFER_LEN     1400

//#define HTTP_RESPONSE_HDR   "HTTP/1.1 200 OK\nServer: RNWF/1.0\nContent-Type: text/html\nContent-Length: %d\nConnection: keep-alive\n\n"

#define HTTP_RESPONSE_NOT_FOUND "HTTP/1.1 404 OK\nConnection: Close\n\n"

#define HTTP_RESPONSE_HDR   "HTTP/1.1 200 OK\nServer: RNWF-02/1.0\nContent-Type: %s\nContent-Length: %d\nConnection: Keep-Alive\n\n"


extern uint8_t http_buffer[HTTP_BUFFER_LEN];  

typedef void (*WEB_REQUEST_HANDLER_t)(uint32_t socket, uint8_t *input);


typedef struct {    
    WEB_REQUEST_HANDLER_t   handler;     
    const char              *url;
}WEB_PAGES_t;


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */
 
    
void HTTP_REQ_Parser(uint32_t socket, uint16_t rx_len);
uint16_t HTTP_RESP_Send(uint32_t socket, const char * resp_hdr, const char *webpage, const char *page_type, uint16_t page_len);

void HTTP_CB_Index(uint32_t socket, uint8_t *input);
void HTTP_CB_Logo(uint32_t socket, uint8_t *input);
void HTTP_CB_Icon(uint32_t socket, uint8_t *input);
void HTTP_CB_Scan(uint32_t socket, uint8_t *input);
void HTTP_CB_Provision(uint32_t socket, uint8_t *input);
    
#endif

#define PROV_BUF_LEN_MAX     128

/* Wi-Fi Configuration */
#define SOFT_AP_SSID        "RNWF02-SoftAP"
#define SOFT_AP_PASSPHRASE  "12345678"
#define SOFT_AP_SECURITY    RNWF_OPEN//RNWF_WPA2_MIXED

#define APP_WIFI_PROV_CONFIG_ID     "apply"
#define APP_WIFI_PROV_DONE_ID       "finish"

/**
 @brief Wi-Fi Provision service List
 
 */
typedef enum 
{
    RNWF_PROV_ENABLE,               /**<Configure the MQTT Broker parameters*/            
    RNWF_PROV_DISABLE,
    RNWF_PROV_SET_CALLBACK,     /**<Set the Application callback for Provisioning Events*/
            
}RNWF_MQTT_SERVICE_t;


/**
 @brief Provisioning events
 
 */
typedef enum {
    RNWF_PROV_COMPLTE,                  /**<Provisionging complete*/
    RNWF_PROV_FAILURE,                  /**<Provisionging Failure*/    
}RNWF_PROV_EVENT_t;


/**
 @brief Wi-Fi Provision modes
 
 */
typedef enum 
{
    RNWF_PROV_MOBILE_APP,               /**<Use the mobile app*/
    RNWF_PROV_WEB_SERVER,               /**<Use the web server*/
            
}RNWF_PROV_MODE_t;


/**
 * @brief       Wi-Fi operation modes
 * 
 */
typedef struct {
    RNWF_PROV_MODE_t mode;          /**<Wi-Fi operation mode ::RNWF_WIFI_MODE_t either STA or SoftAP */    
}RNWF_PROV_PARAM_t;



// TODO Insert appropriate #include <>

// TODO Insert C++ class definitions if appropriate

// TODO Insert declarations

// Comment a function and leverage automatic documentation with slash star star
/**
    <p><b>Function prototype:</b></p>
  
    <p><b>Summary:</b></p>

    <p><b>Description:</b></p>

    <p><b>Precondition:</b></p>

    <p><b>Parameters:</b></p>

    <p><b>Returns:</b></p>

    <p><b>Example:</b></p>
    <code>
 
    </code>

    <p><b>Remarks:</b></p>
 */
// TODO Insert declarations or function prototypes (right here) to leverage 
// live documentation

/**
 * @brief       Wi-Fi callback function type
 * 
 * @param[out]  event One of the ::RNWF_WIFI_EVENT_t event   
 * @param[out]  msg Received data related to the passed event   
 * 
 */
typedef void (*RNWF_PROV_CALLBACK_t)(RNWF_PROV_EVENT_t event, uint8_t *msg);


RNWF_RESULT_t RNWF_PROV_SrvCtrl( RNWF_MQTT_SERVICE_t request, void *input);
        
    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* HTTP_H */

