module ui2

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
