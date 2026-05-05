/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_zigbee_endpoint.c
  * Description        : Zigbee Application to manage endpoints and these clusters.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <assert.h>
#include <stdint.h>

#include "app_common.h"
#include "app_conf.h"
#include "log_module.h"
#include "app_entry.h"
#include "app_zigbee.h"
#include "dbg_trace.h"
#include "ieee802154_enums.h"
#include "mcp_enums.h"

#include "stm32_lpm.h"
#include "stm32_rtos.h"
#include "stm32_timer.h"
#include "stm32_lpm_if.h"

#include "zigbee.h"
#include "zigbee.nwk.h"
#include "zigbee.security.h"

/* Private includes -----------------------------------------------------------*/
#include "zcl/zcl.h"
#include "zcl/general/zcl.onoff.h"
#include "zcl/general/zcl.occupancy.h"

/* USER CODE BEGIN PI */
extern UART_HandleTypeDef huart1;
/* USER CODE END PI */

/* Private defines -----------------------------------------------------------*/
#define APP_ZIGBEE_OCCUPANCY_CLUSTER_ID   ZCL_CLUSTER_OCCUPANCY
#define APP_ZIGBEE_OCCUPANCY_CLUSTER_NAME "Occupancy Server"

#define APP_ZIGBEE_CHANNEL                14u
#define APP_ZIGBEE_CHANNEL_MASK           ( 1u << APP_ZIGBEE_CHANNEL )
#define APP_ZIGBEE_TX_POWER               ((int8_t) 10)    /* TX-Power is at +10 dBm. */

#define APP_ZIGBEE_ENDPOINT               1u
#define APP_ZIGBEE_PROFILE_ID             ZCL_PROFILE_HOME_AUTOMATION
#define APP_ZIGBEE_DEVICE_ID              ZCL_DEVICE_ONOFF_SWITCH

#define APP_ZIGBEE_CLUSTER_ID             ZCL_CLUSTER_ONOFF
#define APP_ZIGBEE_CLUSTER_NAME           "OnOff Client"

/* USER CODE BEGIN PD */
#define CFG_TASK_BUTTON1   (1U << CFG_TASK_ZIGBEE_APP_OTA_REQUEST_ID)
/* USER CODE END PD */

// -- Redefine Clusters to better code read --
#define OnOffClient                       pstZbCluster[0]

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private constants ---------------------------------------------------------*/
/* USER CODE BEGIN PC */

/* USER CODE END PC */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */
void APP_BSP_Button1Action(void);
void APP_ZIGBEE_SendIRState(uint8_t state);
/* USER CODE END PFP */

/* Functions Definition ------------------------------------------------------*/

/**
 * @brief  Zigbee application initialization
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ApplicationInit(void)
{
  LOG_INFO_APP( "ZIGBEE Application Init" );

  /* Initialization of the Zigbee stack */
  APP_ZIGBEE_Init();

  /* Configure Application Form/Join parameters : Startup, Persistence and Start with/without Form/Join */
  stZigbeeAppInfo.eStartupControl = ZbStartTypeJoin;
  stZigbeeAppInfo.bPersistNotification = false;
  stZigbeeAppInfo.bNwkStartup = true;

  /* USER CODE BEGIN APP_ZIGBEE_ApplicationInit */
  LOG_INFO_APP("Spoustim Stack a Join na kanalu 14...");
  UTIL_SEQ_RegTask(1U << CFG_TASK_ZIGBEE_APP1, UTIL_SEQ_RFU, APP_BSP_Button1Action);
  /* USER CODE END APP_ZIGBEE_ApplicationInit */

  /* Initialize Zigbee stack layers */
  APP_ZIGBEE_StackLayersInit();
}

/**
 * @brief  Zigbee application start
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ApplicationStart( void )
{
  /* USER CODE BEGIN APP_ZIGBEE_ApplicationStart */
  UTIL_SEQ_RegTask(1U << CFG_TASK_ZIGBEE_APP1, UTIL_SEQ_RFU, APP_BSP_Button1Action);
  /* USER CODE END APP_ZIGBEE_ApplicationStart */

#if ( CFG_LPM_LEVEL != 0)
  /* Authorize LowPower now */
  UTIL_LPM_SetMaxMode( 1 << CFG_LPM_APP, UTIL_LPM_MAX_MODE );
#endif /* CFG_LPM_LEVEL */
}

/**
 * @brief  Configure Zigbee application endpoints
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_ConfigEndpoints(void)
{
  struct ZbApsmeAddEndpointReqT   stRequest;
  struct ZbApsmeAddEndpointConfT  stConfig;
  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints1 */

  /* USER CODE END APP_ZIGBEE_ConfigEndpoints1 */

  /* Add EndPoint */
  memset( &stRequest, 0, sizeof( stRequest ) );
  memset( &stConfig, 0, sizeof( stConfig ) );

  stRequest.profileId = APP_ZIGBEE_PROFILE_ID;
  stRequest.deviceId = APP_ZIGBEE_DEVICE_ID;
  stRequest.endpoint = APP_ZIGBEE_ENDPOINT;
  ZbZclAddEndpoint( stZigbeeAppInfo.pstZigbee, &stRequest, &stConfig );
  assert( stConfig.status == ZB_STATUS_SUCCESS );

  /* 1. Tady je tvůj původní OnOff Client - TOHLE TAM NECH */
  stZigbeeAppInfo.OnOffClient = ZbZclOnOffClientAlloc( stZigbeeAppInfo.pstZigbee, APP_ZIGBEE_ENDPOINT );
  assert( stZigbeeAppInfo.OnOffClient != NULL );
  if ( ZbZclClusterEndpointRegister( stZigbeeAppInfo.OnOffClient ) == false )
  {
    LOG_ERROR_APP( "Error during OnOff Client Endpoint Register." );
  }

  /* >>> SEM VLOŽ TYTO NOVÉ ŘÁDKY PRO IR SENZOR: <<< */
  stZigbeeAppInfo.OccupancyServer = ZbZclOccupancyServerAlloc( stZigbeeAppInfo.pstZigbee, APP_ZIGBEE_ENDPOINT );
  assert( stZigbeeAppInfo.OccupancyServer != NULL );
  if ( ZbZclClusterEndpointRegister( stZigbeeAppInfo.OccupancyServer ) == false )
  {
    LOG_ERROR_APP( "Error during Occupancy Server Endpoint Register." );
  }
  /* >>> KONEC VKLÁDÁNÍ <<< */

  /* USER CODE BEGIN APP_ZIGBEE_ConfigEndpoints2 */

  /* USER CODE END APP_ZIGBEE_ConfigEndpoints2 */
}

/**
 * @brief  Set Group Addressing mode (if used)
 * @param  None
 * @retval 'true' if Group Address used else 'false'.
 */
bool APP_ZIGBEE_ConfigGroupAddr( void )
{
  /* Not used */

  return false;
}

/**
 * @brief  Return the Startup Configuration
 * @param  pstConfig  Configuration structure to fill
 * @retval None
 */
void APP_ZIGBEE_GetStartupConfig( struct ZbStartupT * pstConfig )
{
  /* Attempt to join a zigbee network */
  ZbStartupConfigGetProDefaults( pstConfig );

  /* Using the default HA preconfigured Link Key */
  memcpy( pstConfig->security.preconfiguredLinkKey, sec_key_ha, ZB_SEC_KEYSIZE );

  /* Setting up additional startup configuration parameters */
  pstConfig->startupControl = stZigbeeAppInfo.eStartupControl;
  pstConfig->channelList.count = 1;
  pstConfig->channelList.list[0].page = 0;
  pstConfig->channelList.list[0].channelMask = APP_ZIGBEE_CHANNEL_MASK;

  /* Set the TX-Power */
  if ( APP_ZIGBEE_SetTxPower( APP_ZIGBEE_TX_POWER ) == false )
  {
    LOG_ERROR_APP( "Switching to %d dB failed.", APP_ZIGBEE_TX_POWER );
    return;
  }

  /* USER CODE BEGIN APP_ZIGBEE_GetStartupConfig */

  /* USER CODE END APP_ZIGBEE_GetStartupConfig */
}

/**
 * @brief  Manage a New Device on Network (called only if Coord or Router).
 * @param  iShortAddress      Short Address of new Device
 * @param  dlExtendedAddress  Extended Address of new Device
 * @param  cCapability        Capability of new Device
 * @retval Group Address
 */
void APP_ZIGBEE_SetNewDevice( uint16_t iShortAddress, uint64_t dlExtendedAddress, uint8_t cCapability )
{
  LOG_INFO_APP( "New Device (%d) on Network : with Extended ( " LOG_DISPLAY64() " ) and Short ( 0x%04X ) Address.", cCapability, LOG_NUMBER64( dlExtendedAddress ), iShortAddress );

  /* USER CODE BEGIN APP_ZIGBEE_SetNewDevice */

  /* USER CODE END APP_ZIGBEE_SetNewDevice */
}

/**
 * @brief  Print application information to the console
 * @param  None
 * @retval None
 */
void APP_ZIGBEE_PrintApplicationInfo(void)
{
  LOG_INFO_APP( "**********************************************************" );
  LOG_INFO_APP( "Network config : CENTRALIZED ROUTER" );

  /* USER CODE BEGIN APP_ZIGBEE_PrintApplicationInfo1 */

  /* USER CODE END APP_ZIGBEE_PrintApplicationInfo1 */
  LOG_INFO_APP( "Channel used: %d.", APP_ZIGBEE_CHANNEL );

  APP_ZIGBEE_PrintGenericInfo();

  LOG_INFO_APP( "Clusters allocated are:" );
  LOG_INFO_APP( "  %s on Endpoint %d.", APP_ZIGBEE_CLUSTER_NAME, APP_ZIGBEE_ENDPOINT );

  /* USER CODE BEGIN APP_ZIGBEE_PrintApplicationInfo2 */

  /* USER CODE END APP_ZIGBEE_PrintApplicationInfo2 */

  LOG_INFO_APP( "**********************************************************" );
}

/* USER CODE BEGIN FD_LOCAL_FUNCTIONS */

/**
 * @brief  Management of the SW1 button : Send the OnOff Command
 * @param  None
 * @retval None
 */
void APP_BSP_Button1Action(void)
{
    struct ZbApsAddrT   stDest;
    enum ZclStatusCodeT eStatus;

    #define PRINT(msg) HAL_UART_Transmit(&huart1, (uint8_t*)(msg), strlen(msg), 200)

    PRINT("[DEBUG] Tlacitko stisknuto\r\n");

    if ( APP_ZIGBEE_IsAppliJoinNetwork() != false )
    {

        memset( &stDest, 0, sizeof(stDest) );
        stDest.endpoint = APP_ZIGBEE_ENDPOINT;
        stDest.mode     = ZB_APSDE_ADDRMODE_SHORT;
        stDest.nwkAddr  = 0x0000;

        eStatus = ZbZclOnOffClientToggleReq(stZigbeeAppInfo.OnOffClient, &stDest, NULL, NULL);

        if (eStatus != ZCL_STATUS_SUCCESS)
        {
            char buf[50];
            snprintf(buf, sizeof(buf), "[ERROR] Odeslani selhalo! Status: 0x%02X\r\n", eStatus);
            PRINT(buf);
        }
        else
        {
            PRINT("[SUCCESS] Prikaz Toggle odeslan.\r\n");
        }
    }

    #undef PRINT
}

void APP_ZIGBEE_SendIRState(uint8_t state)
{
    if ( APP_ZIGBEE_IsAppliJoinNetwork() != false )
    {

        ZbZclAttrIntegerWrite(stZigbeeAppInfo.OccupancyServer, ZCL_OCC_ATTR_OCCUPANCY, (long long)state);


        enum ZclStatusCodeT eStatus = ZbZclClusterReportsSend(stZigbeeAppInfo.OccupancyServer, false, NULL, NULL);

        if (eStatus != ZCL_STATUS_SUCCESS)
        {
            printf("[ERROR] Odeslani IR reportu selhalo! Status: 0x%02X\r\n", eStatus);
        }
        else
        {
            printf("[SUCCESS] IR stav %d odeslan přes Occupancy Cluster.\r\n", state);
        }
    }
}
/* USER CODE END FD_LOCAL_FUNCTIONS */
