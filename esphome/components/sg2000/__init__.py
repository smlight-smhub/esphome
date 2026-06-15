# Copyright 2026 SMLIGHT

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_BOARD,
    KEY_CORE,
    KEY_FRAMEWORK_VERSION,
    KEY_TARGET_FRAMEWORK,
    KEY_TARGET_PLATFORM,
    PLATFORM_SG2000,
    ThreadModel,
)
from esphome.core import CORE

import os
from .const import KEY_SG2000
from esphome import pins
from esphome.const import CONF_NUMBER, CONF_ID

sg2000_ns = cg.esphome_ns.namespace("sg2000")
Sg2000InternalGPIOPin = sg2000_ns.class_("Sg2000InternalGPIOPin", cg.InternalGPIOPin)
SG2000_PIN_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Sg2000InternalGPIOPin),
            cv.Required(CONF_NUMBER): cv.Any(cv.int_, cv.string),
        },
        extra=cv.ALLOW_EXTRA,
    )
)

@pins.PIN_SCHEMA_REGISTRY.register("sg2000", SG2000_PIN_SCHEMA)
async def sg2000_pin_to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_pin_name(str(config[CONF_NUMBER])))
    cg.add(var.set_inverted(config.get(pins.CONF_INVERTED, False)))
    cg.add(var.set_flags(pins.gpio_flags_expr(config.get(pins.CONF_MODE, "INPUT"))))
    return var

CODEOWNERS = ["@esphome/core"]
AUTO_LOAD = ["network", "socket"]
IS_TARGET_PLATFORM = True

CONF_VERSION = "version"

def set_core_data(config):
    CORE.data[KEY_SG2000] = {}
    CORE.data[KEY_CORE][KEY_TARGET_PLATFORM] = PLATFORM_SG2000
    CORE.data[KEY_CORE][KEY_TARGET_FRAMEWORK] = "freertos"
    CORE.data[KEY_CORE][KEY_FRAMEWORK_VERSION] = cv.Version(1, 0, 0)
    if "esphome" in CORE.raw_config:
        CORE.raw_config["esphome"].setdefault("name_add_mac_suffix", True)
    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.Optional(CONF_BOARD, default="c906"): cv.string,
            cv.Optional(CONF_VERSION): cv.string,
        }
    ),
    set_core_data,
)

async def to_code(config):
    cg.add_global(sg2000_ns.using)
    cg.add_build_flag("-DUSE_SG2000")
    cg.add_define("USE_NATIVE_64BIT_TIME")
    cg.set_cpp_standard("gnu++20")
    cg.add_build_flag("-DCONFIG_64BIT")

    cg.add_define("ESPHOME_MANUFACTURER", "SMLIGHT")
    cg.add_define("ESPHOME_BOARD", config[CONF_BOARD])

    if CONF_VERSION in config:
        version_val = config[CONF_VERSION]
    else:
        from esphome.const import __version__
        version_val = __version__
        if version_val.endswith("-dev"):
            version_val = version_val.replace("-dev", "-clean")

    cg.add_define("ESPHOME_FIRMWARE_VERSION_STR", f'"===ESPHOME_BIN_VERSION:{version_val}==="')
    
    # The xPack GCC 15 toolchain natively supports RISC-V hardware atomics (lr.w/sc.w).
    # We use ThreadModel.MULTI_ATOMICS to eliminate FreeRTOS Critical Section overhead
    # and utilize lock-free data structures.
    cg.add_define(ThreadModel.MULTI_ATOMICS)
    
    cg.add_platformio_option(
        "platform", "symlink:///usr/local/src/smlight/smhub/antigravity-nodered/platform-sg2000"
    )
    cg.add_platformio_option(
        "platform_packages",
        "framework-sg2000-rtos @ symlink:///usr/local/src/smlight/smhub/antigravity-nodered/framework-sg2000-rtos",
    )
    cg.add_platformio_option("framework", "freertos")
    cg.add_platformio_option("board", "smhub")
