@[has_globals]
module ui2

__global startup_phase_test_seen = false
__global startup_phase_test_phase = StartupPhase.process_launch

fn startup_phase_test_handler(phase StartupPhase, _at_ns u64) {
	startup_phase_test_seen = true
	startup_phase_test_phase = phase
}

fn test_startup_phase_hook_is_backend_neutral() {
	startup_phase_test_seen = false
	set_startup_phase_handler(startup_phase_test_handler)
	trace_startup_phase(.window_creation)
	assert startup_phase_test_seen
	assert startup_phase_test_phase == .window_creation
	assert startup_phase_name(.directory_completion) == 'directory_completion'
	clear_startup_phase_handler()
}
