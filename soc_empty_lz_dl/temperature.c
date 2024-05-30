#include "temperature.h"
#include "sl_sensor_rht.h"


float conversion_Temperature(){
  sl_status_t sc;
  uint32_t *rh;
  uint32_t *t;
  sl_sensor_rht_init();
  sc=sl_sensor_rht_get(&rh,&t);
  sl_sensor_rht_deinit();
  return sc*0.01;
}
