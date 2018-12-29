/* This code belongs in ble_cus.h*/
#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "ble_link_ctx_manager.h"

#define CUSTOM_SERVICE_UUID_BASE         {0x4C, 0x0D, 0x36, 0xE1 , 0x59 , 0x09 , 0x27 , 0x8B , \
                                          0x73 , 0x45 , 0x11 , 0x8A, 0x97 , 0x55, 0xED, 0x4D}
/* This code belongs in ble_cus.h*/
#define CUSTOM_SERVICE_UUID               0x2501
#define ALARM_TX_VALUE_CHAR_UUID          0x2502
#define ALARM_RX_VALUE_CHAR_UUID          0x2503				

#define OPCODE_LENGTH        1
#define HANDLE_LENGTH        2
																					
/**@brief   Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
#if defined(NRF_SDH_BLE_GATT_MAX_MTU_SIZE) && (NRF_SDH_BLE_GATT_MAX_MTU_SIZE != 0)
    #define BLE_NUS_MAX_DATA_LEN (NRF_SDH_BLE_GATT_MAX_MTU_SIZE - OPCODE_LENGTH - HANDLE_LENGTH)
#else
    #define BLE_NUS_MAX_DATA_LEN (BLE_GATT_MTU_SIZE_DEFAULT - OPCODE_LENGTH - HANDLE_LENGTH)
    #warning NRF_SDH_BLE_GATT_MAX_MTU_SIZE is not defined.
#endif
																					
/**@brief   Macro for defining a ble_cus instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_ALARM_DEF(_name)                                                                        \
static ble_alarm_t _name;																																						\
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_HRS_BLE_OBSERVER_PRIO,                                                     \
                     ble_alarm_on_ble_evt, &_name)

// Forward declaration of the ble_alarm_t type.
typedef struct ble_alarm_s ble_alarm_t;
										
typedef enum
{
		BLE_ALARM_EVT,
		BLE_ALARM_EVT_ALARM,
		BLE_ALARM_EVT_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_ALARM_EVT_NOTIFICATION_DISABLED,                            /**< Custom value notification disabled event. */
    BLE_ALARM_EVT_DISCONNECTED,
    BLE_ALARM_EVT_CONNECTED
} ble_alarm_evt_type_t;

/**@brief   Nordic UART Service @ref BLE_NUS_EVT_RX_DATA event data.
 *
 * @details This structure is passed to an event when @ref BLE_NUS_EVT_RX_DATA occurs.
 */
typedef struct
{
    uint8_t const * p_data; /**< A pointer to the buffer with received data. */
    uint16_t        length; /**< Length of received data. */
} ble_evt_alarm_data_t;


/**@brief Nordic UART Service client context structure.
 *
 * @details This structure contains state context related to hosts.
 */
typedef struct
{
    bool is_notification_enabled; /**< Variable to indicate if the peer has enabled notification of the RX characteristic.*/
} ble_alarm_client_context_t;


/**@brief   Nordic UART Service event structure.
 *
 * @details This structure is passed to an event coming from service.
 */
typedef struct
{
    ble_alarm_evt_type_t           evt_type;        /**< Event type. */
    ble_alarm_t                  * p_alarm;       /**< A pointer to the instance. */
    uint16_t                   	   conn_handle; /**< Connection handle. */
    ble_alarm_client_context_t   * p_link_ctx;  /**< A pointer to the link context. */
    union
    {
        ble_evt_alarm_data_t alarm_data; /**< @ref BLE_NUS_EVT_RX_DATA event data. */
    } params;
} ble_alarm_evt_t;

/**@brief Custom Service event handler type. */
typedef void (*ble_alarm_evt_handler_t) (ble_alarm_t * p_cus, ble_alarm_evt_t * p_evt);

/**@brief Custom Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
		ble_alarm_evt_handler_t       evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint8_t                       initial_custom_value;           /**< Initial custom value */
    ble_srv_cccd_security_mode_t  custom_value_char_attr_md;      /**< Initial security level for Custom characteristics attribute */
} ble_alarm_init_t;


/**@brief Custom Service structure. This contains various status information for the service. */
struct ble_alarm_s
{
		ble_alarm_evt_handler_t       evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint16_t                      service_handle;                 /**< Handle of Custom Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      tx_value_handles;           		/**< Handles related to the TX Value characteristic. */
    ble_gatts_char_handles_t    	rx_value_handles;								/**< Handles related to the RX Value characteristic. */
		uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type; 
	
		blcm_link_ctx_storage_t * const p_link_ctx_storage; /**< Pointer to link context storage with handles of all current connections and its context. */
};

/**@brief Function for initializing the Custom Service.
 *
 * @param[out]  p_cus       Custom Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cus_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_alarm_init(ble_alarm_t * p_alarm, const ble_alarm_init_t * p_alarm_init);


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_alarm_t * p_alarm, const ble_alarm_init_t * p_alarm_init);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Custom Service structure.
 */
void ble_alarm_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief Function for updating the custom value.
 *
 * @details The application calls this function when the cutom value should be updated. If
 *          notification has been enabled, the custom value characteristic is sent to the client.
 *
 * @note 
 *       
 * @param[in]   p_cus          Custom Service structure.
 * @param[in]   Custom value 
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_alarm_custom_value_update(ble_alarm_t * p_cus, uint8_t value);


uint32_t ble_nus_data_send(ble_alarm_t* p_nus, uint8_t * p_data, uint16_t * p_length, uint16_t conn_handle);

