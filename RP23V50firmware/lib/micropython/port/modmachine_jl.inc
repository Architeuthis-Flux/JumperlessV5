// Extra glue for Jumperless embed port

// Provide idle hook required by extmod/modmachine.c
static void mp_machine_idle(void) {
    // Allow background processing; nothing required for now
    MICROPY_INTERNAL_EVENT_HOOK;
}

// Expose additional globals in machine module
#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },

// Force QSTRs used by Pin into the embed QSTR scan
static void jl_qstr_refs(void) {
    (void)MP_QSTR_Pin; (void)MP_QSTR_low; (void)MP_QSTR_high; (void)MP_QSTR_toggle; (void)MP_QSTR_ALT;
    (void)MP_QSTR_on;  (void)MP_QSTR_off; (void)MP_QSTR_pull; (void)MP_QSTR_alt;
}
