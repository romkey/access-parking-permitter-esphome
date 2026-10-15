import esphome.codegen as cg
from esphome.components import sensor, text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TIMESTAMP,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
)

CODEOWNERS = ["@romkey"]
AUTO_LOAD = ["sha256", "sensor", "text_sensor"]

CONF_RECORDS = "records"
CONF_LAST_UPDATED = "last_updated"

access_list_ns = cg.esphome_ns.namespace("access_list")
AccessList = access_list_ns.class_("AccessList", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AccessList),
            cv.Optional(CONF_RECORDS): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:card-account-details",
            ),
            # ISO 8601, so Home Assistant shows it as a timestamp.
            cv.Optional(CONF_LAST_UPDATED): text_sensor.text_sensor_schema(
                device_class=DEVICE_CLASS_TIMESTAMP,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if records := config.get(CONF_RECORDS):
        cg.add(var.set_records_sensor(await sensor.new_sensor(records)))
    if last_updated := config.get(CONF_LAST_UPDATED):
        cg.add(var.set_last_updated_sensor(await text_sensor.new_text_sensor(last_updated)))
