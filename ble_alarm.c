#include "sdk_common.h"
#include "ble_srv_common.h"
#include "ble_alarm.h"
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"
#include "app_uart.h"
#include "nrf_uart.h"
#include "ble_link_ctx_manager.h"

uint8_t is_main_data = 1;

uint32_t ble_alarm_init(ble_alarm_t * p_alarm, const ble_alarm_init_t * p_alarm_init)
{
    if (p_alarm == NULL || p_alarm_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t   err_code;
    ble_uuid_t ble_uuid;
		
		// Initialize service structure
		p_alarm->conn_handle               = BLE_CONN_HANDLE_INVALID;
		p_alarm->evt_handler               = p_alarm_init->evt_handler;
		
		// Add Custom Service UUID
		ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
		err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_alarm->uuid_type);
		VERIFY_SUCCESS(err_code);

		ble_uuid.type = p_alarm->uuid_type;
		ble_uuid.uuid = CUSTOM_SERVICE_UUID;

		// Add the Custom Service
		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_alarm->service_handle);
		/**@snippet [Adding proprietary Service to the SoftDevice] */
		VERIFY_SUCCESS(err_code);
		
		// Add Custom Value characteristic
		return custom_value_char_add(p_alarm, p_alarm_init);
}


/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Custom Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_alarm_t * p_alarm, const ble_alarm_init_t * p_alarm_init)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
	
		//Add the TX Characteristic
		memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read   = 1;
    char_md.char_props.write  = 0;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL; 
    char_md.p_sccd_md         = NULL;
	
		memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm 	 = p_alarm_init->custom_value_char_attr_md.read_perm;
    attr_md.write_perm 	 = p_alarm_init->custom_value_char_attr_md.write_perm;
    attr_md.vloc       	 = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth      = 0;
    attr_md.wr_auth      = 0;
    attr_md.vlen         = 0;
		
		ble_uuid.type = p_alarm->uuid_type;
    ble_uuid.uuid = ALARM_TX_VALUE_CHAR_UUID;
		
		memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
		attr_char_value.max_len   = BLE_GATTS_FIX_ATTR_LEN_MAX;
//    attr_char_value.max_len   = sizeof(uint8_t);
		
		memset(&cccd_md, 0, sizeof(cccd_md));

    //  Read  operation on Cccd should be possible without authentication.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
//    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

//		char_md.char_props.notify = 1;  
//    char_md.p_cccd_md         = &cccd_md; 
		
		err_code = sd_ble_gatts_characteristic_add(p_alarm->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_alarm->tx_value_handles);
    VERIFY_SUCCESS(err_code);
		
		//Add the RX Characteristic 
		memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read   = 0;
    char_md.char_props.write  = 1;
    char_md.char_props.notify = 0; 
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL; 
    char_md.p_sccd_md         = NULL;
	
		memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  		= p_alarm_init->custom_value_char_attr_md.read_perm;
    attr_md.write_perm 	  = p_alarm_init->custom_value_char_attr_md.write_perm;
    attr_md.vloc       		= BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    		= 0;
    attr_md.wr_auth    		= 0;
    attr_md.vlen       		= 0;
		
		ble_uuid.type = p_alarm->uuid_type;
    ble_uuid.uuid = ALARM_RX_VALUE_CHAR_UUID;
		
		memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
		attr_char_value.max_len   = BLE_GATTS_FIX_ATTR_LEN_MAX;
//    attr_char_value.max_len   = sizeof(uint8_t);
		
		err_code = sd_ble_gatts_characteristic_add(p_alarm->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_alarm->rx_value_handles);
		return err_code;
}


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_alarm_t * p_alarm, ble_evt_t const * p_ble_evt)
{	
    p_alarm->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	
		ble_alarm_evt_t evt;

    evt.evt_type = BLE_ALARM_EVT_CONNECTED;

    p_alarm->evt_handler(p_alarm, &evt);
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_alarm_t * p_alarm, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_alarm->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cus       Custom Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_alarm_t * p_alarm, ble_evt_t const * p_ble_evt)
{
		ret_code_t                    err_code;
    ble_alarm_evt_t               evt;
    ble_alarm_client_context_t  * p_client;
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	
		err_code = blcm_link_ctx_get(p_alarm->p_link_ctx_storage,
                                 p_ble_evt->evt.gatts_evt.conn_handle,
                                 (void *) &p_client);
	
		if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("Link context for 0x%02X connection handle could not be fetched.",
                      p_ble_evt->evt.gatts_evt.conn_handle);
    }

    memset(&evt, 0, sizeof(ble_alarm_evt_t));
    evt.p_alarm     = p_alarm;
    evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
    evt.p_link_ctx  = p_client;
		
		 // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == p_alarm->tx_value_handles.cccd_handle)
        && (p_evt_write->len == 2))
    {
        // CCCD written, call application event handler
        if (p_alarm->evt_handler != NULL)
        {
            ble_alarm_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_ALARM_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_ALARM_EVT_NOTIFICATION_DISABLED;
            }
            // Call the application event handler.
            p_alarm->evt_handler(p_alarm, &evt);
        }
    }
		
		else if ((p_evt_write->handle == p_alarm->rx_value_handles.value_handle) &&
             (p_alarm->evt_handler != NULL))
    {	
			if(is_main_data == 1)
			{
				is_main_data = 0;
				if(p_evt_write->data[0] == 's')
				{	
					evt.evt_type                 = BLE_ALARM_EVT_ALARM;
					evt.params.alarm_data.p_data = p_evt_write->data;
					evt.params.alarm_data.length = p_evt_write->len;
					p_alarm->evt_handler(p_alarm, &evt);
				}
				else
				{
					evt.evt_type                 = BLE_ALARM_EVT;
					evt.params.alarm_data.p_data = p_evt_write->data;
					evt.params.alarm_data.length = p_evt_write->len;
					p_alarm->evt_handler(p_alarm, &evt);
				}
			}
			else if(is_main_data == 0)
			{
				is_main_data = 1;
				evt.evt_type                 = BLE_ALARM_EVT;
				evt.params.alarm_data.p_data = p_evt_write->data;
				evt.params.alarm_data.length = p_evt_write->len;
				p_alarm->evt_handler(p_alarm, &evt);
			}
    }
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Battery Service.
 *
 * @note 
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 * @param[in]   p_context  Custom Service structure.
 */
void ble_alarm_on_ble_evt( ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_alarm_t * p_alarm = (ble_alarm_t *) p_context;
    if (p_alarm == NULL || p_ble_evt == NULL)
    {
        return;
    }
		
		switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
						
            on_connect(p_alarm, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
						
            on_disconnect(p_alarm, p_ble_evt);
            break;
				
				case BLE_GATTS_EVT_WRITE:
						on_write(p_alarm, p_ble_evt);
           break;
				
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_nus_data_send(ble_alarm_t * p_nus,
                           uint8_t     * p_data,
                           uint16_t    * p_length,
                           uint16_t      conn_handle)
{
    ret_code_t                   err_code;
    ble_gatts_hvx_params_t       hvx_params;
    ble_alarm_client_context_t * p_client;

    VERIFY_PARAM_NOT_NULL(p_nus);

    err_code = blcm_link_ctx_get(p_nus->p_link_ctx_storage, conn_handle, (void *) &p_client);
    VERIFY_SUCCESS(err_code);

    if ((conn_handle == BLE_CONN_HANDLE_INVALID) || (p_client == NULL))
    {
        return NRF_ERROR_NOT_FOUND;
    }

    if (!p_client->is_notification_enabled)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (*p_length > BLE_NUS_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_nus->tx_value_handles.value_handle;
    hvx_params.p_data = p_data;
    hvx_params.p_len  = p_length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(conn_handle, &hvx_params);
}

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
uint32_t ble_alarm_custom_value_update(ble_alarm_t * p_alarm, uint8_t value)
{
		uint32_t err_code = NRF_SUCCESS;
		ble_gatts_value_t gatts_value;

		// Initialize value struct.
		memset(&gatts_value, 0, sizeof(gatts_value));

		gatts_value.len     = sizeof(uint8_t);
		gatts_value.offset  = 0;
		gatts_value.p_value = &value;

		// Update database.
		err_code = sd_ble_gatts_value_set(p_alarm->conn_handle,
																				p_alarm->tx_value_handles.value_handle,
																				&gatts_value);
    if (p_alarm == NULL)
    {
        return NRF_ERROR_NULL;
    }
		// Send value if connected and notifying.
		if ((p_alarm->conn_handle != BLE_CONN_HANDLE_INVALID)) 
		{
				ble_gatts_hvx_params_t hvx_params;

				memset(&hvx_params, 0, sizeof(hvx_params));

				hvx_params.handle = p_alarm->tx_value_handles.value_handle;
				hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
				hvx_params.offset = gatts_value.offset;
				hvx_params.p_len  = &gatts_value.len;
				hvx_params.p_data = gatts_value.p_value;

				err_code = sd_ble_gatts_hvx(p_alarm->conn_handle, &hvx_params);
		}
		else
		{
				err_code = NRF_ERROR_INVALID_STATE;
		}

		return err_code;
}

