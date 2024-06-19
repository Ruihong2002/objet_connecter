/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include "app_log.h"
#include "sl_sensor_rht.h"
#include "temperature.h"
#include "stdint.h"
#include "gatt_db.h"
#include "sl_sleeptimer.h"
#include "sl_simple_led_instances.h"

#define TEMPERATURE_TIMER_SIGNAL (1<<0)

void callback(sl_sleeptimer_timer_callback_t *handle, void * data);
sl_sleeptimer_timer_handle_t handle;
int x=0;
uint8_t connection_temp,connection_digital;
uint16_t characteristic_temp,characteristic_digital;
void * led_handle;
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  app_log_info("%s\n",__FUNCTION__);
  sl_simple_led_init_instances();
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint32_t *rh;
  uint32_t *t;
  uint16_t* sent_lent;


  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);
      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      app_log_info("%s: connection_opened!\n",__FUNCTION__);
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      app_log_info("%s: connection_closed!\n",__FUNCTION__);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_user_read_request_id:
      app_log_info("test_read\n",__FUNCTION__);
      switch (evt->data.evt_gatt_server_user_read_request.characteristic) {
        case gattdb_temperature:
          int temperature=getTemperature();
          app_log_info("temperature:%dC\n",temperature);
          sc=sl_bt_gatt_server_send_user_read_response(evt->data.handle,gattdb_temperature,0,sizeof(temperature),&temperature,&sent_lent);
          app_assert_status(sc);
          break;
       default:
          break;
      }//end switch

      case sl_bt_evt_gatt_server_characteristic_status_id:
       //app_log_info("test_notify\n",__FUNCTION__);
       switch (evt->data.evt_gatt_server_characteristic_status.characteristic){
         case gattdb_temperature:
           /*int temperature=getTemperature();
           app_log_info("notify:Coucou!\n");
           app_log_info("temperature_notify:%dC\n",temperature);*/
           connection_temp=evt->data.evt_gatt_server_characteristic_status.connection;
           characteristic_temp=evt->data.evt_gatt_server_characteristic_status.characteristic;

           if(evt->data.evt_gatt_server_characteristic_status.status_flags && 0x1){//end switch
             app_log_info("config_flag:%d\n",evt->data.evt_gatt_server_characteristic_status.client_config_flags);
             if(evt->data.evt_gatt_server_characteristic_status.client_config_flags){
                 /*int temperature=getTemperature();
                 app_log_info("temperature:%dC\n",temperature);
                 sc=sl_bt_gatt_server_send_user_read_response(evt->data.handle,gattdb_temperature,0,sizeof(temperature),&temperature,&sent_lent);
                 app_assert_status(sc);*/

                 sl_sleeptimer_start_periodic_timer_ms(&handle,1000,callback,NULL,0,0);
                 app_log_info("timer on\n");
                 sl_led_led0.turn_on(sl_led_led0.context); //led actif
             }
             else {
                 app_log_info("timer off\n");
                 sl_sleeptimer_stop_timer(&handle);


                 sl_led_led0.turn_off(sl_led_led0.context);//led inactif
             }
           }

           break;
         default:
           break;
       }
       break;

       case (sl_bt_evt_system_external_signal_id):
           switch (evt->data.evt_system_external_signal.extsignals){
             case(TEMPERATURE_TIMER_SIGNAL):
                 app_log_info("external_signal\n");
                 app_log_info("temp=%d\n",getTemperature());
                 int temp=getTemperature();
                 sl_bt_gatt_server_send_notification(connection_temp,characteristic_temp,sizeof(temp),(const uint8_t*)&temp);
                 break;
             default:
                 break;
           }
       case (sl_bt_evt_gatt_server_user_write_request_id):
              switch(evt->data.evt_gatt_server_user_write_request.characteristic){
                case (gattdb_digital):
                     if (evt->data.evt_gatt_server_user_write_request.att_opcode && sl_bt_gatt_write_command){
                          app_log_info("digital sans\n");
                          sl_led_led0.toggle(sl_led_led0.context);} //led actif
                    else if (evt->data.evt_gatt_server_user_write_request.att_opcode && sl_bt_gatt_write_request){
                    app_log_info("digital\n");
                     sl_led_led0.toggle(sl_led_led0.context); //led actif
                     sl_bt_gatt_server_send_user_write_response(connection_digital,characteristic_digital,0);
                     app_log_info("response write\n");}
                default:
                    break;
              }

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

void callback(sl_sleeptimer_timer_callback_t *handle, void * data){
  handle = handle;
  data = data;
  sl_bt_external_signal(TEMPERATURE_TIMER_SIGNAL);
}
