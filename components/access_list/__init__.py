import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@romkey"]
AUTO_LOAD = ["sha256"]

access_list_ns = cg.esphome_ns.namespace("access_list")
AccessList = access_list_ns.class_("AccessList", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema({cv.GenerateID(): cv.declare_id(AccessList)}).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
