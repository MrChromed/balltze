// SPDX-License-Identifier: GPL-3.0-only

#include <chrono>
#include <balltze/engine/tick.hpp>
#include <balltze/memory/hook.hpp>
#include <balltze/balltze.hpp>
#include <balltze/event/event.hpp>

namespace Balltze {
    static Memory::Hook tick_event_hook;
    static Memory::Hook tick_event_after_chimera_hook;
    static bool first_tick = true;
    static std::chrono::time_point<std::chrono::steady_clock> last_tick;
    static std::chrono::milliseconds tick_duration = std::chrono::milliseconds(0);

    static void tick_event_before_dispatcher() {
        auto tick_count = Engine::get_tick_count();
        TickEventArguments args;
        args.delta_time_ms = tick_duration.count();
        args.tick_count = tick_count;
        TickEvent tick_event(EVENT_TIME_BEFORE, args);
        tick_event.dispatch();
    }

    static void tick_event_after_dispatcher() {
        auto tick_count = Engine::get_tick_count();
        auto current_tick = std::chrono::steady_clock::now();
        if(!first_tick) {
            tick_duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick);
        }
        else {
            first_tick = false;
        }
        last_tick = current_tick;
        TickEventArguments args;
        args.delta_time_ms = tick_duration.count();
        args.tick_count = tick_count;
        TickEvent tick_event(EVENT_TIME_AFTER, args);
        tick_event.dispatch();
    }

    template <>
    void EventHandler<TickEvent>::init() {
        static bool enabled = false;
        if(enabled) {
            return;
        }
        enabled = true;

        static auto &balltze = Balltze::get();
        static auto &sig_manager = balltze.signature_manager();
        static auto *tick_event_sig = sig_manager.get("on_tick");
        if(!tick_event_sig) {
            throw std::runtime_error("Could not find signature for tick event");
        }

        // Workaround for Chimera hook (NEEDS TO BE FIXED)
        std::byte *ptr = Memory::Hook::follow_jump(tick_event_sig->data()) + 23;
        tick_event_after_chimera_hook.initialize(ptr, reinterpret_cast<void *>(tick_event_after_dispatcher));
        tick_event_after_chimera_hook.hook();

        tick_event_hook.initialize(tick_event_sig->data(), reinterpret_cast<void *>(tick_event_before_dispatcher));
        tick_event_hook.hook();
    }
}