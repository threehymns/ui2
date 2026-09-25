module ui2

import macos
import math

$if ios ? {

	fn uikit_test_image_resource(id string, red u8, opacity ImageOpacity) ImageResource {
		mut pixels := []u8{len: 4, init: 255}
		pixels[0] = red
		return ready_image_resource(id, 'missing.png', ImageResourceInput{
			width: 1
			height: 1
			channels: 4
			pixels: pixels
		}, opacity)
	}

	fn uikit_test_pattern() RepeatPattern {
		mut pixels := []u8{len: 4, init: 255}
		pixels[0] = 1
		pixels[1] = 2
		pixels[2] = 3
		return RepeatPattern{
			id:           'uikit-pattern'
			tile_width:   16
			tile_height:  16
			pixel_width:  1
			pixel_height: 1
			channels:     4
			pixels:       pixels
		}
	}

	fn test_uikit_decoded_resource_retains_previous_image_and_uses_latest_ready_resource() {
		ensure_runtime_classes()
		pool := macos.autorelease_pool_new()
		defer {
			macos.release(pool)
		}
		first := uikit_test_image_resource('uikit-first', 255, .proven_opaque)
		view := new_image_view_element(transformed_image_resource('image', first, rect(10, 20, 40, 30), 0, false))
		defer {
			g_image_states.delete(u64(view))
			macos.release(view)
		}
		first_image := macos.msg_id(view, 'image')
		assert first_image != unsafe { nil }

		loading := loading_image_resource('uikit-loading', 'missing-next.png')
		native_update_image_element(view, transformed_image_resource('image', loading, rect(10, 20, 40, 30), 0, false))
		assert macos.msg_id(view, 'image') == first_image

		failed := error_image_resource('uikit-error', 'missing-error.png', 'decode failed')
		native_update_image_element(view, transformed_image_resource('image', failed, rect(10, 20, 40, 30), 0, false))
		assert macos.msg_id(view, 'image') == first_image

		unknown := uikit_test_image_resource('uikit-unknown', 2, .unknown)
		native_update_image_element(view, transformed_image_resource('image', unknown, rect(10, 20, 40, 30), 0, false))
		assert macos.msg_id(view, 'image') != first_image

		latest := uikit_test_image_resource('uikit-latest', 1, .has_alpha)
		native_update_image_element(view, transformed_image_resource('image', latest, rect(10, 20, 40, 30), 0, false))
		assert macos.msg_id(view, 'image') != first_image
	}

	fn test_uikit_image_transform_filter_and_rotation_state() {
		ensure_runtime_classes()
		pool := macos.autorelease_pool_new()
		defer {
			macos.release(pool)
		}
		resource := uikit_test_image_resource('uikit-transform', 128, .has_alpha)
		view := new_image_view_element(transformed_image_resource('image', resource, rect(0, 0, 80, 40), 90, false))
		defer {
			g_image_states.delete(u64(view))
			macos.release(view)
		}
		mut element := transformed_image_resource('image', resource, rect(0, 0, 80, 40), 90, false)
		element = with_flip_h(element)
		element = with_pixelated(element)
		native_update_image_element(view, element)
		layer := macos.msg_id(view, 'layer')
		scale_x := macos.msg_id1(layer, 'valueForKeyPath:', macos.nsstring('transform.scale.x'))
		filter := macos.msg_id1(layer, 'valueForKeyPath:', macos.nsstring('magnificationFilter'))
		rotation := macos.msg_id1(layer, 'valueForKeyPath:', macos.nsstring('transform.rotation'))
		assert macos.msg_f64(scale_x, 'doubleValue') == -1.0
		assert macos.utf8_string(filter) == 'nearest'
		assert math.abs(macos.msg_f64(rotation, 'doubleValue') - math.pi / 2.0) < 0.000001
	}

	fn test_uikit_pattern_phase_clip_and_resize_reuse_native_tile() {
		ensure_runtime_classes()
		pool := macos.autorelease_pool_new()
		defer {
			macos.release(pool)
		}
		pattern := uikit_test_pattern()
		first := pattern_background('pattern', pattern, rect(100, 50, 64, 48))
		view := native_create_element(first)
		key := u64(view)
		first_image := g_pattern_images[key] or { unsafe { nil } }
		first_state := g_pattern_states[key] or { NativePatternState{} }
		assert first_image != unsafe { nil }
		assert first_state.phase_x == 12
		assert first_state.phase_y == 14
		assert first_state.clip == first.background.clip
		macos.msg_void_rect(view, 'setFrame:', macos.rect(116, 66, 128, 96))
		second := pattern_background('pattern', pattern, rect(116, 66, 128, 96))
		native_update_pattern(view, second)
		second_image := g_pattern_images[key] or { unsafe { nil } }
		second_state := g_pattern_states[key] or { NativePatternState{} }
		assert second_image == first_image
		assert second_state.phase_x == 12
		assert second_state.phase_y == 14
		assert second_state.clip == second.background.clip
		native_clear_pattern(view)
		macos.release(view)
	}
}
