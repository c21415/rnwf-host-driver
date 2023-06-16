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




#define HTTP_CONTENT_LEN    "Content-Length:"
/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */


RNWF_OTA_CALLBACK_t gOta_CallBack_Handler;

uint8_t ota_buf[OTA_BUF_LEN_MAX];

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

volatile uint32_t total_rx = 0;

uint32_t otaFileSize = 0;

RNWF_RESULT_t RNWF_OTA_CONF_Process(uint32_t socket, uint16_t rx_len)
{
    int32_t read_size, result;
    uint8_t *tmpPtr, *buffer = &ota_buf[OTA_BUF_LEN_MAX/2];
    RNWF_OTA_CFG_t otaCfg;
    while(rx_len > 0)
    {
        if((result = RNWF_NET_TCP_SOCK_Read(socket, rx_len, buffer)) > 0 )
        {
            rx_len -= result;
            read_size += result;
        }
        else
        {
            break;
        }
    }
    if(strncmp(buffer, OTA_CONF_FW_HDR, strlen(OTA_CONF_FW_HDR)) || strncmp(buffer, OTA_CONF_FS_HDR, strlen(OTA_CONF_FS_HDR)))
    {
        char *args[OTA_CFG_PARAM_MAX] = {0, 0, 0, 0}, *token;
        uint8_t idx = 0;
        tmpPtr = buffer + strlen(OTA_CONF_FW_HDR);
        token = (char *)strtok(tmpPtr, ", ");
        while(token != NULL)
        {
            printf("%s\r\n", token);
            args[idx++] = token;
            token = (char *)strtok(NULL, ", ");
        }
        otaCfg.socket.tls_conf = 0;
        otaCfg.socket.bind_type = RNWF_BIND_REMOTE;
        otaCfg.socket.sock_type = RNWF_SOCK_TCP;
        sscanf(args[OTA_CFG_PARAM_PORT], "%u", &otaCfg.socket.sock_port);
        otaCfg.socket.sock_addr = args[OTA_CFG_PARAM_SERVER];
        otaCfg.file = args[OTA_CFG_PARAM_FILE];
        otaCfg.type = strtol(args[OTA_CFG_PARAM_TYPE], NULL, 10);
        gOta_CallBack_Handler(RNWF_EVENT_IMAGE_INFO, (uint8_t *)&otaCfg);
        RNWF_OTA_SrvCtrl(RNWF_OTA_REQUEST, (void *)&otaCfg);
    }
}

RNWF_RESULT_t RNWF_OTA_DWNLD_Process(uint32_t socket, uint16_t rx_len)
{
    int32_t read_size;
    RNWF_OTA_CHUNK_t ota_chunk;
    static uint16_t result = 0;
    char *tmpPtr;
    //Parse the First HTTP Response
    if((total_rx > 0) && (total_rx == otaFileSize))
    {
        return RNWF_PASS;
    }

    while(rx_len > 0)
    {
        volatile uint16_t readCnt = ((rx_len + result) > OTA_BUF_LEN_MAX)?(OTA_BUF_LEN_MAX-result):rx_len;
        if((read_size = RNWF_NET_TCP_SOCK_Read(socket, readCnt, (uint8_t *)&ota_buf[result])) > 0 )
        {
            if(!otaFileSize)
            {
                DBG_MSG_OTA("%s\r\n", ota_buf);
                if((tmpPtr = (char *)strstr((char *)ota_buf, HTTP_CONTENT_LEN)) != NULL)
                {
                    char *token = strtok(tmpPtr, "\r\n");
                    otaFileSize = strtol((token+sizeof(HTTP_CONTENT_LEN)), NULL, 10);
                    gOta_CallBack_Handler(RNWF_EVENT_DWLD_START, (uint8_t *)&otaFileSize);
                }
                break;
            }
            result += read_size;
            total_rx = total_rx + read_size;
            rx_len -= read_size;
            if(result == OTA_BUF_LEN_MAX)
            {
                ota_chunk.chunk_size = OTA_BUF_LEN_MAX;
                ota_chunk.chunk_ptr = ota_buf;
                DBG_MSG_OTA("%lu\r\n", total_rx);
                gOta_CallBack_Handler(RNWF_EVENT_FILE_CHUNK, (uint8_t *)&ota_chunk);
                result = 0;
            }
            if(total_rx == otaFileSize)
            {
                ota_chunk.chunk_size = result;
                ota_chunk.chunk_ptr = ota_buf;
                gOta_CallBack_Handler(RNWF_EVENT_FILE_CHUNK, (uint8_t *)&ota_chunk);
                gOta_CallBack_Handler(RNWF_EVENT_DWLD_DONE, (uint8_t *)&total_rx);
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
    static uint32_t cfg_client_id = 0;
    switch(event)
    {
        case RNWF_NET_SOCK_EVENT_CONNECTED:
        {
            if(!cfg_client_id && strstr(p_str, TOSTR(OTA_CONF_PORT)))
            {
                cfg_client_id = sock;
                DBG_MSG_OTA("Connection received on configuration Tunnel %u\r\n");
                RNWF_SYSTEM_SrvCtrl(RNWF_SYSTEM_DEV_INFO, ota_buf);
            }
            RNWF_NET_TCP_SOCK_Write(sock, strlen((char *)ota_buf), (uint8_t *)ota_buf);
        }
        break;

        case RNWF_NET_SOCK_EVENT_DISCONNECTED:
        {
//            DBG_MSG_OTA("Close OTA Socket!\r\n");
            cfg_client_id = 0;
            RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_CLOSE, &sock);
            break;
        }
        case RNWF_NET_SOCK_EVENT_READ:
        {
            uint16_t rx_len = *(uint16_t *)p_str;
            if(cfg_client_id == sock)
                RNWF_OTA_CONF_Process(sock, rx_len);
            else
                RNWF_OTA_DWNLD_Process(sock, rx_len);
            break;
        }
        default:
            break;
    }
}


RNWF_RESULT_t RNWF_OTA_SrvCtrl( RNWF_OTA_SERVICE_t request, void *input)
{
    RNWF_RESULT_t result = RNWF_PASS;
    static uint32_t otaAddress;

    switch(request)
    {
        case RNWF_OTA_ENABLE:
        {
            // Bind a lock port to set the OTA configuration
            RNWF_NET_SOCKET_t ota_cfg_sock;
            ota_cfg_sock.bind_type = RNWF_BIND_LOCAL;
            ota_cfg_sock.sock_port = 6666;
            ota_cfg_sock.sock_type = RNWF_SOCK_TCP;
            ota_cfg_sock.tls_conf = 0;
            /* RNWF Application Callback register */
            if(gOta_CallBack_Handler != NULL)
            {
                RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_SET_SRVC_CALLBACK, RNWF_OTA_SOCKET_Callback);
                RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_TCP_OPEN, &ota_cfg_sock);
            }
        }
        break;

        case RNWF_OTA_REQUEST:
        {
            RNWF_OTA_CFG_t *otaCfg = (RNWF_OTA_CFG_t *)input;
            sprintf(ota_buf, OTA_HTTP_REQ, otaCfg->file);
            if(gOta_CallBack_Handler != NULL)
            {
                RNWF_NET_SOCK_SrvCtrl(RNWF_NET_SOCK_TCP_OPEN, &otaCfg->socket);
            }
        }
        break;

        case RNWF_OTA_SET_CALLBACK:
        if(input == NULL)
        {
            gOta_CallBack_Handler = (RNWF_OTA_CALLBACK_t)(input);
            result = RNWF_FAIL;
        }
        break;

        case RNWF_OTA_DFU_INIT:
        {
            uint8_t  peVersion = 0;
            uint32_t chipID = 0;

            otaAddress = (uint32_t)*(uint32_t *)input;
            DFU_PE_InjectTestPattern();

            DELAY_milliseconds(UART_DELAY_MSEC);

            /* Verify PE version */
            peVersion = DFU_PE_Version();

            if(peVersion != RIO0_PE_VERSION)
            {
                DBG_MSG_OTA("PE version didn't match");
                result = RNWF_FAIL;
            }

            /* Verify chip ID */
            chipID = DFU_PE_Chip_ID();

            if (chipID != RIO0_CHIP_ID)
            {
                DBG_MSG_OTA("Chip ID didn't match");
                result = RNWF_FAIL;
            }
        }
        break;

        case RNWF_OTA_DFU_ERASE:
        {
            /* Erase */
            if (DFU_PE_Erase(otaAddress, (uint32_t)*(uint32_t *)input) == false)
            {
                /* Error handling */
                DBG_MSG_OTA("PE erase reported error");
                result = RNWF_FAIL;
            }
        }
        break;

        case RNWF_OTA_DFU_WRITE:
        {
            RNWF_OTA_CHUNK_t *otaChunk = (RNWF_OTA_CHUNK_t *)input;
            if(DFU_PE_Write(otaAddress, otaChunk->chunk_size, otaChunk->chunk_size))
            {
                otaAddress += otaChunk->chunk_size;
                result = RNWF_FAIL;
            }
        }
        break;

        default:
            break;
    }
    return result;
}


/***********************RNWF02 DFU API's*************************/

#ifdef DFU_DEBUG
static uint32_t DFU_PE_htonl (uint32_t x)
{
    uint32_t ret = x;

    if (BYTE_ORDER == BIG_ENDIAN)
    {
    }
    else if (BYTE_ORDER == LITTLE_ENDIAN)
    {
      ret = __builtin_bswap32 (x);
    }
    else
    {
        DBG_MSG_OTA("ERROR: Unknown system\r\n");
        DFU_ASSERT(false);
    }

    return ret;
}
#endif

void DFU_PE_InjectTestPattern(void)
{
    /* Programming Executive (PE) test pattern 'MCHP' PGC->Tx PGC->Rx*/
    const char *PE_TP_MCLR = "100000000000000000000000000000000000000000000000000000000000000001";
    const char *PE_TP_PGC  = "001010101010101010101010101010101010101010101010101010101010101011";
    const char *PE_TP_PGD  = "000110000111100110011000000001111001100001100000000110011000000000";

    DBG_MSG_OTA("* Sending test pattern *\r\n%s\r\n%s\r\n%s\r\n", PE_TP_MCLR, PE_TP_PGC, PE_TP_PGD);
    UART2->Deinitialize();

    MCLR_SetDigitalOutput();
    PGC_SetDigitalOutput();
    PGD_SetDigitalOutput();

    DELAY_milliseconds(MSEC_TO_SEC);

    for (volatile int i=0; i<(int)strlen(PE_TP_MCLR); i++)
    {
        /* MCLR */
        if((PE_TP_MCLR[i] - '0'))
            MCLR_SetHigh();
        else
            MCLR_SetLow();
        /* PGC */
        if((PE_TP_PGC[i] - '0'))
            PGC_SetHigh();
        else
            PGC_SetLow();

        /* PGD */
        if((PE_TP_PGD[i] - '0'))
            PGD_SetHigh();
        else
            PGD_SetLow();

        DELAY_microseconds(TP_DELAY_USEC);
    }

    MCLR_SetDigitalInput();
    USART2->Initialize();
}

uint8_t DFU_PE_Version(void)
{
    uint32_t data = 0;
    uint8_t  peVersion = 0;
    uint8_t  byteResp[4];

    data = PE_CMD_EXEC_VERSION;
    data = (data << 16) | 0x1;

    DBG_MSG_OTA("Sending PE version request\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(data));
#endif

    RNWF_IF_Write((uint8_t *)&data, 4);

    if(RNWF_IF_Read(byteResp, 4) == 4)
    {
        peVersion = byteResp[0];
        DBG_MSG_OTA("PE version: %d\r\n\r\n", (unsigned int)peVersion);
        return peVersion;
    }
    return 0;
}

uint32_t DFU_PE_Chip_ID(void)
{
    uint32_t data = 0;
    uint32_t chipID = 0;
    uint8_t  byteResp[9];

    data = PE_CMD_GET_DEVICE_ID;
    data = (data << 16) | 0x01;

    DBG_MSG_OTA("Sending PE chip ID request\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(data));
#endif
    RNWF_IF_Write((uint8_t *)&data, 4);

    /* Response */
    RNWF_IF_Read(byteResp, 8);
    byteResp[9] = '\0';
    DBG_MSG_OTA("Chip ID Response = %02X %02X %02X %02X\r\n", byteResp[4], byteResp[6], byteResp[6], byteResp[7]);

    memcpy(&chipID, &byteResp[4], 4);
    DBG_MSG_OTA("Chip ID: %08X\r\n\r\n", chipID);
    return chipID;
}

bool DFU_PE_Erase(const uint32_t address, const uint32_t length)
{
    uint32_t data = 0;
    uint32_t pages = length / (uint32_t)PE_ERASE_PAGE_SIZE;
    uint8_t  byteResp[4];

    if (length % (uint32_t)PE_ERASE_PAGE_SIZE > (uint32_t)0)
    {
        pages += (uint32_t)1;
    }

    data = PE_CMD_PAGE_ERASE;
    data = data << 16;
    data |= (pages &= 0x0000ffff);


    DBG_MSG_OTA("Sending PE erase\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(data));
#endif
    RNWF_IF_Write((uint8_t *)&data, 4);

#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(address));
#endif
    RNWF_IF_Write((uint8_t *)&address, 4);

    /* Response */
    RNWF_IF_Read(byteResp, 4);

    if (((char)byteResp[2] != (char)PE_CMD_PAGE_ERASE) || ((char)byteResp[0] != (char)0) || ((char)byteResp[1] != (char)0))
    {
        DBG_MSG_OTA("Error: PE erase failed\r\n");
        return false;
    }

    DBG_MSG_OTA("\r\nErase done!\r\n");
    return true;
}

bool DFU_PE_Write(const uint32_t address, const uint32_t length, uint8_t *PE_writeBuffer)
{
    /* The address must be 32-bit aligned, and the number of bytes (length) must be a
    multiple of a 32-bit word. */
    uint32_t data = 0;
    uint32_t checksumValue = 0;
    uint8_t byteResp[4];

    if (length>(uint16_t)MAX_PE_WRITE_SIZE)
    {
        DBG_MSG_OTA("ERROR: Length exceeds MAX_PE_WRITE_SIZE\r\n");
        return false;
    }

    /* Length should be integer factor of 4096 and divisible by 4 */
    if ((((uint16_t)MAX_PE_WRITE_SIZE % length) != (uint16_t)0) || ((length % (uint16_t)4) != (uint16_t)0))
    {
        DBG_MSG_OTA("ERROR: Length should be integer factor of 4096 and divisible by 4\r\n");
        return false;
    }

    /* Assemble PE write command */
    data |= ((uint32_t)0x0000ffff & (uint32_t)PE_CMD_PGM_CLUSTER_VERIFY) << 16;
    data |= (CFGMethod & 0x0000ffff);
    DBG_MSG_OTA("ID:\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(data));
#endif
    RNWF_IF_Write((uint8_t *)&data, sizeof(data));

    /* Address */
    DBG_MSG_OTA("Address:\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(address));
#endif
    RNWF_IF_Write((uint8_t *)&address, sizeof(address));

    /* Length */
    DBG_MSG_OTA("Length: %d\r\n", (unsigned int)length);
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(length));
#endif
    RNWF_IF_Write((uint8_t *)&length, sizeof(length));

    /* Checksum */
    for (uint16_t i=0; i<length; i++)
    {
        checksumValue += PE_writeBuffer[i];
    }

    DBG_MSG_OTA("Checksum:\r\n");
#ifdef DFU_DEBUG
    DBG_MSG_OTA("%08x\r\n", (unsigned int)DFU_PE_htonl(checksumValue));
#endif
    RNWF_IF_Write((uint8_t *)&checksumValue, sizeof(checksumValue));

    /* Data */
    DBG_MSG_OTA("PE_writeBuffer:\r\n");
    for (uint16_t i=0; i<length; i++)
    {
        DBG_MSG_OTA("%02x ", PE_writeBuffer[i]);
        RNWF_IF_Write((uint8_t *)&PE_writeBuffer[i], 1);

        DELAY_microseconds(WRITE_DELAY_USEC);
    }
    DBG_MSG_OTA("\r\n");

    /* Response */
    RNWF_IF_Read(byteResp, 4);


    /* Verify response for errors */
    if (((char)byteResp[2] != (char)PE_CMD_PGM_CLUSTER_VERIFY) || ((char)byteResp[0] != (char)0) || ((char)byteResp[1] != (char)0))
    {
        DBG_MSG_OTA("Error: PE write failed\r\n");
        return false;
    }

    return true;
}

void DFU_Reset(void)
{
    const char *MCLR_RESET = "101";

    DBG_MSG_OTA("* Resetting NC * \r\n");

    MCLR_SetDigitalOutput();
    for (uint8_t i=0; i<(uint8_t)strlen(MCLR_RESET); i++)
    {
        if((MCLR_RESET[i] - '0') == '1')
            MCLR_SetHigh();
        else
            MCLR_SetLow();

        DELAY_microseconds(TP_DELAY_USEC);
    }
    MCLR_SetDigitalInput();
}

RNWF_RESULT_t DFU_PROGRAM_Task(uint32_t otaBinSize)
{
    uint8_t  peVersion = 0;
    uint32_t chipID = 0;
    uint32_t startAddress = RIO0_FLASH_BASE;

    uint32_t n = sizeof(otaBinSize) / (uint32_t)DFU_PE_WRITE_SIZE;
    uint32_t r = sizeof(otaBinSize) % (uint32_t)DFU_PE_WRITE_SIZE;

    /* DFU mode */
    DFU_PE_InjectTestPattern();

    DELAY_milliseconds(UART_DELAY_MSEC);

    /* Verify PE version */
    peVersion = DFU_PE_Version();

    if(peVersion != RIO0_PE_VERSION)
    {
        DBG_MSG_OTA("PE version didn't match");
    }

    /* Verify chip ID */
    chipID = DFU_PE_Chip_ID();

    if (chipID != RIO0_CHIP_ID)
    {
        DBG_MSG_OTA("Chip ID didn't match");
        return RNWF_FAIL;
    }

    /* Erase */
    if (DFU_PE_Erase(startAddress, otaBinSize) == false)
    {
        /* Error handling */
        DBG_MSG_OTA("PE erase reported error");
        return RNWF_FAIL;
    }

    /* Write */
    /* 512 byte image header, therefore image size is not always
     * divisible by 4096. */
    for (uint32_t i=0 ; i<n; i++)
    {
        HighSpeed_Read_Cont(startAddress + ((uint32_t)DFU_PE_WRITE_SIZE * i), DFU_PE_WRITE_SIZE, (char *)ota_buf); 
        if (DFU_PE_Write(startAddress + ((uint32_t)DFU_PE_WRITE_SIZE * i), DFU_PE_WRITE_SIZE, &ota_buf) == false)
        {
            /* Error handling */
            DBG_MSG_OTA("PE write reported error");
            return RNWF_FAIL;
        }
    }

    /* Write any remainder */
    if (r > (uint32_t)0)
    {
        HighSpeed_Read_Cont(startAddress + ((uint32_t)DFU_PE_WRITE_SIZE * n), DFU_PE_WRITE_SIZE, (char *)ota_buf); 
        if (DFU_PE_Write(startAddress + ((uint32_t)DFU_PE_WRITE_SIZE * n), r, &ota_buf) == false)
        {
            /* Error handling */
            DBG_MSG_OTA("PE write reported error");
            return RNWF_FAIL;
        }
    }

    /* Reset NC */
    DFU_Reset();

    return RNWF_PASS;
}