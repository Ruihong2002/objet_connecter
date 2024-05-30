#include "temperature.h"
#include "sl_sensor_rht.h"


int getTemperature(){
  uint32_t rh;
  uint32_t t;
  sl_sensor_rht_init();
  sl_sensor_rht_get(&rh,&t);
  sl_sensor_rht_deinit();
  return ((int)(t*0.01));
}
