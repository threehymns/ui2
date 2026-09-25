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

fn test_repeat_pattern_visible_rect_clips_to_parent_and_reveal_region() {
	pattern := repeat_pattern_test_pattern()
	clip := rect(20, 30, 40, 24)
	background := pattern_background('pattern', pattern, clip).background
	visible := background.visible_rect(0, 0, clip, rect(0, 0, 45, 45))
	assert visible == rect(20, 30, 25, 15)
	assert background.visible_rect(0, 0, Rect{}, Rect{}) == Rect{}
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
}
