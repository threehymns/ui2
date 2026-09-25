module ui2

import math

pub enum ImageResourceState {
	loading
	ready
	error
}

pub enum ImageOpacity {
	unknown
	proven_opaque
	has_alpha
}

pub struct ImageResourceInput {
pub:
	width   int
	height  int
	channels int
	pixels  []u8
}

pub struct ImageResource {
pub:
	id             string
	source         string
	state          ImageResourceState
	opacity        ImageOpacity
	renderer_input ImageResourceInput
	error          string
}

pub fn loading_image_resource(id string, source string) ImageResource {
	return ImageResource{
		id:     id
		source: source
		state:  .loading
		opacity: .unknown
	}
}

pub fn ready_image_resource(id string, source string, input ImageResourceInput, opacity ImageOpacity) ImageResource {
	return ImageResource{
		id:             id
		source:         source
		state:          .ready
		opacity:        opacity
		renderer_input: input
	}
}

pub fn error_image_resource(id string, source string, message string) ImageResource {
	return ImageResource{
		id:      id
		source:  source
		state:   .error
		opacity: .unknown
		error:   message
	}
}

pub fn legacy_image_resource(source string, width int, height int) ImageResource {
	return ImageResource{
		source: source
		state:  .ready
		opacity: .unknown
		renderer_input: ImageResourceInput{
			width:    width
			height:   height
			channels: 4
		}
	}
}

pub fn (resource ImageResource) width() int {
	return resource.renderer_input.width
}

pub fn (resource ImageResource) height() int {
	return resource.renderer_input.height
}

pub fn (resource ImageResource) channels() int {
	return resource.renderer_input.channels
}

pub fn (resource ImageResource) decoded_pixels() []u8 {
	return resource.renderer_input.pixels
}

pub fn (input ImageResourceInput) valid() bool {
	if input.width <= 0 || input.height <= 0 || input.channels != 4 {
		return false
	}
	return input.width <= input.pixels.len / input.height / input.channels
}

pub fn (resource ImageResource) renderer_ready() bool {
	return resource.id.len > 0 && resource.state == .ready && resource.renderer_input.valid()
}

pub fn normalized_image_rotation(rotation f64) int {
	value := int(math.fmod(rotation, 360.0))
	return (value % 360 + 360) % 360
}

pub fn image_texture_flips(rotation f64, flip_h bool, flip_v bool) (bool, bool) {
	normalized := normalized_image_rotation(rotation)
	if normalized == 90 || normalized == 270 {
		return flip_v, flip_h
	}
	return flip_h, flip_v
}

pub fn transformed_image_bounds(frame Rect, rotation f64) Rect {
	if frame.width <= 0 || frame.height <= 0 {
		return Rect{
			x: frame.x
			y: frame.y
		}
	}
	normalized := normalized_image_rotation(rotation)
	mut cosine := 1.0
	mut sine := 0.0
	if normalized % 90 != 0 {
		radians := rotation * math.pi / 180.0
		cosine = math.abs(math.cos(radians))
		sine = math.abs(math.sin(radians))
	} else if normalized == 90 || normalized == 270 {
		cosine = 0.0
		sine = 1.0
	}
	width := frame.width * cosine + frame.height * sine
	height := frame.width * sine + frame.height * cosine
	center_x := frame.x + frame.width / 2.0
	center_y := frame.y + frame.height / 2.0
	return Rect{
		x:      center_x - width / 2.0
		y:      center_y - height / 2.0
		width:  width
		height: height
	}
}

pub fn image_source_uv(frame Rect, rotation f64, flip_h bool, flip_v bool, screen_x f64, screen_y f64) (f64, f64) {
	if frame.width <= 0 || frame.height <= 0 {
		return 0, 0
	}
	dx := screen_x - (frame.x + frame.width / 2.0)
	dy := screen_y - (frame.y + frame.height / 2.0)
	normalized := normalized_image_rotation(rotation)
	mut x := dx
	mut y := dy
	if normalized % 90 == 0 {
		match normalized {
			90 { x = dy
				y = -dx }
			180 { x = -dx
				y = -dy }
			270 { x = -dy
				y = dx }
			else {}
		}
	} else {
		radians := rotation * math.pi / 180.0
		cosine := math.cos(radians)
		sine := math.sin(radians)
		x = dx * cosine + dy * sine
		y = -dx * sine + dy * cosine
	}
	if flip_h {
		x = -x
	}
	if flip_v {
		y = -y
	}
	return x / frame.width + 0.5, y / frame.height + 0.5
}

pub fn image_resource(id string, resource ImageResource, frame Rect) Element {
	return Element{
		kind:          .image
		id:            id
		image_path:    resource.source
		image_resource: resource
		frame:         frame
	}
}

pub fn transformed_image_resource(id string, resource ImageResource, frame Rect, rotation f64, clickable bool) Element {
	return Element{
		...image_resource(id, resource, frame)
		rotation: rotation
		clickable: clickable
	}
}

pub fn transformed_image_resource_with_cursor(id string, resource ImageResource, frame Rect, rotation f64, clickable bool, cursor string) Element {
	return Element{
		...image_resource(id, resource, frame)
		rotation: rotation
		clickable: clickable
		cursor: cursor
	}
}

pub fn image_path_for_element(el Element) string {
	if el.image_resource.id.len == 0 {
		return el.image_path
	}
	if el.image_resource.state != .ready {
		return ''
	}
	if el.image_path.len > 0 {
		return el.image_path
	}
	return el.image_resource.source
}
