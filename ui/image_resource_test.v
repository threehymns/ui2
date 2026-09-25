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

	failed := error_image_resource('error-id', 'sample.png', 'decode failed')
	assert failed.state == .error
	assert failed.opacity == .unknown
	assert failed.error == 'decode failed'
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

fn test_legacy_image_constructor_remains_path_compatible() {
	element := transformed_image('photo', 'sample.png', rect(0, 0, 10, 10), 0, false)
	assert element.image_path == 'sample.png'
	assert element.image_resource.id == ''
	assert image_path_for_element(element) == 'sample.png'
}
