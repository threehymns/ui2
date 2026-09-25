@[has_globals]
module ui2

__global startup_phase_test_seen = false
__global startup_phase_test_phase = StartupPhase.process_launch
__global startup_phase_test_at_ns = u64(0)
__global startup_phase_test_frame_complete = false

fn startup_phase_test_handler(phase StartupPhase, at_ns u64) {
	startup_phase_test_seen = true
	startup_phase_test_phase = phase
	startup_phase_test_at_ns = at_ns
}

fn startup_phase_test_frame_handler() {
	startup_phase_test_frame_complete = true
}

fn test_startup_phase_hook_is_backend_neutral() {
	startup_phase_test_seen = false
	set_startup_phase_handler(startup_phase_test_handler)
	trace_startup_phase_at(.window_creation, 123)
	assert startup_phase_test_seen
	assert startup_phase_test_phase == .window_creation
	assert startup_phase_test_at_ns == 123
	assert startup_phase_name(.gpu_context_initialization) == 'gpu_context_initialization'
	assert startup_phase_name(.directory_completion) == 'directory_completion'
	startup_phase_test_frame_complete = false
	set_frame_complete_handler(startup_phase_test_frame_handler)
	trace_frame_complete()
	assert startup_phase_test_frame_complete
	clear_startup_phase_handler()
	clear_frame_complete_handler()
}
