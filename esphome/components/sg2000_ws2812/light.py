import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light
from esphome.const import CONF_OUTPUT_ID, CONF_NUM_LEDS

sg2000_ws2812_ns = cg.esphome_ns.namespace('sg2000_ws2812')
Sg2000Ws2812 = sg2000_ws2812_ns.class_('Sg2000Ws2812', light.LightOutput, cg.Component)

CONF_SPI_BASE = "spi_base"

CONFIG_SCHEMA = light.ADDRESSABLE_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(Sg2000Ws2812),
    cv.Required(CONF_SPI_BASE): cv.hex_int,
    cv.Required(CONF_NUM_LEDS): cv.int_,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID], config[CONF_SPI_BASE], config[CONF_NUM_LEDS])
    await cg.register_component(var, config)
    await light.register_light(var, config)
