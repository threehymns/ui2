@[has_globals]
module ui2

import time

pub enum StartupPhase {
	process_launch
	window_creation
	font_work
	ui2_setup
	gpu_setup
	first_content
	first_input
	directory_completion
}

pub type StartupPhaseHandler = fn (StartupPhase, u64)
pub type FrameCompleteHandler = fn ()

__global g_startup_phase_handler = StartupPhaseHandler(unsafe { nil })
__global g_frame_complete_handler = FrameCompleteHandler(unsafe { nil })

pub fn set_startup_phase_handler(handler StartupPhaseHandler) {
	g_startup_phase_handler = handler
}

pub fn clear_startup_phase_handler() {
	g_startup_phase_handler = StartupPhaseHandler(unsafe { nil })
}

pub fn set_frame_complete_handler(handler FrameCompleteHandler) {
	g_frame_complete_handler = handler
}

pub fn clear_frame_complete_handler() {
	g_frame_complete_handler = FrameCompleteHandler(unsafe { nil })
}

pub fn trace_frame_complete() {
	if voidptr(g_frame_complete_handler) == unsafe { nil } {
		return
	}
	g_frame_complete_handler()
}

pub fn trace_startup_phase(phase StartupPhase) {
	if voidptr(g_startup_phase_handler) == unsafe { nil } {
		return
	}
	g_startup_phase_handler(phase, time.sys_mono_now())
}

pub fn startup_phase_name(phase StartupPhase) string {
	return match phase {
		.process_launch { 'process_launch' }
		.window_creation { 'window_creation' }
		.font_work { 'font_work' }
		.ui2_setup { 'ui2_setup' }
		.gpu_setup { 'gpu_setup' }
		.first_content { 'first_content' }
		.first_input { 'first_input' }
		.directory_completion { 'directory_completion' }
	}
}
