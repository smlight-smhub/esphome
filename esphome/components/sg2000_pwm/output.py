import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID, CONF_CHANNEL, CONF_FREQUENCY

sg2000_pwm_ns = cg.esphome_ns.namespace("sg2000_pwm")
Sg2000PWM = sg2000_pwm_ns.class_("Sg2000PWM", output.FloatOutput, cg.Component)

CONF_PWM_ID = "pwm_id"

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(Sg2000PWM),
        cv.Optional("pin", default="PAD_MIPI_TXM0"): cv.string,
        cv.Required(CONF_PWM_ID): cv.int_range(min=0, max=3),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=3),
        cv.Optional(CONF_FREQUENCY, default="1000Hz"): cv.All(
            cv.frequency, cv.float_range(min=1.0)
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)

    cg.add(var.set_pwm_id(config[CONF_PWM_ID]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))

    pin = config["pin"]
    pwm_id = config[CONF_PWM_ID]
    channel = config[CONF_CHANNEL]
    abs_channel = pwm_id * 4 + channel
    cg.add(var.set_pin_mux(cg.RawExpression(f"FMUX_GPIO_FUNCSEL_{pin}"), cg.RawExpression(f"{pin}__PWM_{abs_channel}")))
