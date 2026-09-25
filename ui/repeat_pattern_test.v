module ui2

fn repeat_pattern_test_pattern() RepeatPattern {
	mut pixels := []u8{len: 4, init: 255}
	pixels[0] = 1
	pixels[1] = 2
	pixels[2] = 3
	pixels[3] = 255
	return RepeatPattern{
		id:           'pattern-test'
		tile_width:   16
		tile_height:  16
		pixel_width:  1
		pixel_height: 1
		channels:     4
		pixels:       pixels
	}
}

fn test_repeat_pattern_contract_is_backend_neutral() {
	pattern := repeat_pattern_test_pattern()
	assert pattern.valid()
	assert pattern.origin_x == 0
	assert pattern.origin_y == 0
	clip := rect(10, 20, 30, 40)
	element := pattern_background('pattern', pattern, clip)
	assert element.kind == .view
	assert element.id == 'pattern'
	assert element.frame == clip
	assert element.box.transparent
	assert element.background.pattern.id == 'pattern-test'
	assert element.background.clip == clip
	assert element.image_path == ''
}

fn test_repeat_pattern_source_rect_keeps_window_phase() {
	mut pattern := repeat_pattern_test_pattern()
	pattern.origin_x = 100
	pattern.origin_y = 50
	first := pattern.source_rect(rect(100, 50, 32, 16))
	second := pattern.source_rect(rect(116, 66, 32, 16))
	assert first.x == 0
	assert first.y == 0
	assert first.width == 2
	assert first.height == 1
	assert second.x == 1
	assert second.y == 1
}

fn test_repeat_pattern_rejects_invalid_tiles() {
	mut pattern := repeat_pattern_test_pattern()
	pattern.channels = 3
	assert !pattern.valid()
	pattern = repeat_pattern_test_pattern()
	pattern.pixels = []u8{}
	assert !pattern.valid()
	assert pattern.source_rect(rect(0, 0, 10, 10)) == Rect{}
}

fn test_repeat_pattern_cache_identity_is_independent_of_window_geometry() {
	pattern := repeat_pattern_test_pattern()
	mut moved := pattern
	moved.origin_x = 200
	moved.origin_y = 100
	assert repeat_pattern_cache_key(pattern) == repeat_pattern_cache_key(moved)
	assert pattern.pixel_width * pattern.pixel_height < 16 * 16
	assert pattern.phase(Rect{}) == Rect{}
}

fn test_repeat_pattern_phase_is_anchored_to_window_origin() {
	pattern := repeat_pattern_test_pattern()
	first := pattern.phase(rect(100, 50, 32, 32))
	moved := pattern.phase(rect(116, 66, 32, 32))
	assert first.x == 12
	assert first.y == 14
	assert first.width == 16
	assert first.height == 16
	assert moved.x == 12
	assert moved.y == 14
}

fn test_repeat_pattern_source_rect_honors_logical_tile_scale() {
	mut pattern := repeat_pattern_test_pattern()
	pattern.tile_width = 32
	pattern.tile_height = 16
	pattern.pixel_width = 64
	pattern.pixel_height = 16
	pattern.pixels = []u8{len: 64 * 16 * 4, init: 255}
	source := pattern.source_rect(rect(16, 8, 32, 16))
	assert source.x == 32
	assert source.y == 8
	assert source.width == 64
	assert source.height == 16
}

fn test_repeat_pattern_clip_intersects_image_reveal_and_resize() {
	first := intersect_rect(rect(100, 80, 64, 48), rect(120, 100, 100, 100))
	assert first == rect(120, 100, 44, 28)
	second := intersect_rect(rect(220, 160, 128, 96), rect(200, 100, 100, 100))
	assert second == rect(220, 160, 80, 40)
}

fn test_repeat_pattern_element_keeps_tile_when_clip_and_frame_change() {
	pattern := repeat_pattern_test_pattern()
	first := pattern_background('pattern', pattern, rect(100, 80, 64, 48))
	second := pattern_background('pattern', pattern, rect(200, 120, 128, 96))
	assert first.background.clip == rect(100, 80, 64, 48)
	assert second.background.clip == rect(200, 120, 128, 96)
	assert repeat_pattern_cache_key(first.background.pattern) == repeat_pattern_cache_key(second.background.pattern)
}
