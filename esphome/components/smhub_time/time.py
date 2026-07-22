# Copyright 2026 SMLIGHT

import esphome.codegen as cg
from esphome.components import time as time_
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@esphome"]
DEPENDENCIES = ["time"]

smhub_time_ns = cg.esphome_ns.namespace("smhub_time")
SmhubTime = smhub_time_ns.class_("SmhubTime", time_.RealTimeClock)

CONFIG_SCHEMA = time_.TIME_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(SmhubTime),
    }
).extend(cv.polling_component_schema("15min"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await time_.register_time(var, config)
