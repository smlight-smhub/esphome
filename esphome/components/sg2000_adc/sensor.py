import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_CHANNEL, STATE_CLASS_MEASUREMENT, UNIT_VOLT, DEVICE_CLASS_VOLTAGE

sg2000_adc_ns = cg.esphome_ns.namespace("sg2000_adc")
Sg2000ADC = sg2000_adc_ns.class_("Sg2000ADC", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(
    Sg2000ADC,
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=3,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT,
).extend(
    {
        cv.Required(CONF_CHANNEL): cv.int_range(min=1, max=3),
    }
).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    cg.add(var.set_channel(config[CONF_CHANNEL]))
