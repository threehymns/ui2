module ui2

fn image_resource_test_pixels() []u8 {
	mut pixels := []u8{len: 8}
	pixels[0] = 255
	pixels[1] = 0
	pixels[2] = 0
	pixels[3] = 0
	pixels[4] = 0
	pixels[5] = 255
	pixels[6] = 0
	pixels[7] = 128
	return pixels
}

fn test_image_resource_states_and_opacity_are_explicit() {
	loading := loading_image_resource('loading-id', 'sample.png')
	assert loading.id == 'loading-id'
	assert loading.source == 'sample.png'
	assert loading.state == .loading
	assert loading.opacity == .unknown
	assert !loading.renderer_ready()

	pixels := image_resource_test_pixels()
	ready := ready_image_resource('ready-id', 'sample.png', ImageResourceInput{
		width: 2
		height: 1
		channels: 4
		pixels: pixels
	}, .has_alpha)
	assert ready.state == .ready
	assert ready.opacity == .has_alpha
	assert ready.width() == 2
	assert ready.height() == 1
	assert ready.channels() == 4
	assert ready.decoded_pixels() == pixels
	assert ready.renderer_ready()

	failed := error_image_resource('error-id', 'sample.png', 'decode failed')
	assert failed.state == .error
	assert failed.opacity == .unknown
	assert failed.error == 'decode failed'
	assert !failed.renderer_ready()
}

fn test_ready_image_resource_preserves_each_opacity_state() {
	pixels := image_resource_test_pixels()
	for opacity in [ImageOpacity.unknown, .proven_opaque, .has_alpha] {
		resource := ready_image_resource('ready-id', 'sample.png', ImageResourceInput{
			width: 2
			height: 1
			channels: 4
			pixels: pixels
		}, opacity)
		assert resource.state == .ready
		assert resource.opacity == opacity
		assert resource.renderer_ready()
	}
}

fn test_renderer_ready_requires_valid_decoded_rgba_pixels() {
	invalid_inputs := [
		ImageResourceInput{},
		ImageResourceInput{
			width: 1
			height: 1
			channels: 3
			pixels: []u8{len: 3, init: 255}
		},
		ImageResourceInput{
			width: 2
			height: 1
			channels: 4
			pixels: []u8{len: 7, init: 255}
		},
	]
	for input in invalid_inputs {
		assert !input.valid()
		assert !ready_image_resource('ready-id', 'sample.png', input, .unknown).renderer_ready()
	}
	assert !ready_image_resource('', 'sample.png', ImageResourceInput{
		width: 1
		height: 1
		channels: 4
		pixels: []u8{len: 4, init: 255}
	}, .proven_opaque).renderer_ready()
}

fn test_resource_image_constructor_preserves_transform_fields_and_path_adapter() {
	resource := ready_image_resource('ready-id', 'sample.png', ImageResourceInput{
		width: 4
		height: 3
		channels: 4
		pixels: []u8{len: 48, init: 255}
	}, .proven_opaque)
	element := transformed_image_resource('photo', resource, rect(2, 3, 40, 30), 90, true)
	assert element.kind == .image
	assert element.id == 'photo'
	assert element.image_resource.id == 'ready-id'
	assert element.image_path == 'sample.png'
	assert element.frame == rect(2, 3, 40, 30)
	assert element.rotation == 90
	assert element.clickable
	assert image_path_for_element(element) == 'sample.png'
	assert with_flip_h(with_pixelated(element)).flip_h
	assert with_flip_v(element).flip_v
}

fn test_image_transform_contract_covers_rotation_and_flips() {
	frame := rect(10, 20, 20, 10)
	assert transformed_image_bounds(frame, 0) == frame
	assert transformed_image_bounds(frame, 90) == rect(15, 15, 10, 20)
	assert transformed_image_bounds(frame, 180) == rect(10, 20, 20, 10)
	assert transformed_image_bounds(frame, 270) == rect(15, 15, 10, 20)
	assert transformed_image_bounds(frame, -90) == rect(15, 15, 10, 20)

	flip_x, flip_y := image_texture_flips(90, true, false)
	assert !flip_x
	assert flip_y
	turned_flip_x, turned_flip_y := image_texture_flips(270, false, true)
	assert turned_flip_x
	assert !turned_flip_y
	reversed_flip_x, reversed_flip_y := image_texture_flips(180, true, true)
	assert reversed_flip_x
	assert reversed_flip_y
}

fn test_image_source_uv_preserves_screen_space_transforms() {
	frame := rect(0, 0, 2, 1)
	u, v := image_source_uv(frame, 90, false, false, 1, -0.5)
	assert u == 0
	assert v == 0.5
	flipped_u, flipped_v := image_source_uv(frame, 90, true, false, 1, -0.5)
	assert flipped_u == 1
	assert flipped_v == 0.5
	vertical_u, vertical_v := image_source_uv(frame, 0, false, true, 2, 1)
	assert vertical_u == 1
	assert vertical_v == 0
	center_u, center_v := image_source_uv(frame, 0, false, false, 1, 0.5)
	assert center_u == 0.5
	assert center_v == 0.5
}

fn test_legacy_image_constructor_remains_path_compatible() {
	element := transformed_image('photo', 'sample.png', rect(0, 0, 10, 10), 0, false)
	assert element.image_path == 'sample.png'
	assert element.image_resource.id == ''
	assert image_path_for_element(element) == 'sample.png'
}
