#include "esphome/core/application.h"
#include <cstring>

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#ifdef USE_LIGHT
#include "esphome/components/light/light_state.h"
#endif

#ifdef USE_RTTTL
#include "esphome/components/rtttl/rtttl.h"

esphome::rtttl::Rtttl *global_rtttl = nullptr;
#endif

extern "C" {
    void smhub_rpc_control_switch(uint32_t key, bool state) {
#ifdef USE_SWITCH
        auto *sw = esphome::App.get_switch_by_key(key);
        if (sw != nullptr) {
            if (state) {
                sw->turn_on();
            } else {
                sw->turn_off();
            }
        }
#endif
    }

    void smhub_rpc_control_light(uint32_t key, bool state, float brightness, float r, float g, float b, const char* effect) {
#ifdef USE_LIGHT
        auto *light = esphome::App.get_light_by_key(key);
        if (light != nullptr) {
            auto call = light->make_call();
            call.set_state(state);
            call.set_brightness(brightness);
            call.set_red(r);
            call.set_green(g);
            call.set_blue(b);
            if (effect != nullptr && strlen(effect) > 0) {
                call.set_effect(effect);
            }
            call.perform();
        }
#endif
    }

    void smhub_rpc_control_buzzer(const char* song) {
#ifdef USE_RTTTL
        if (global_rtttl != nullptr && song != nullptr && strlen(song) > 0) {
            global_rtttl->play(song);
        }
#endif
    }
}
