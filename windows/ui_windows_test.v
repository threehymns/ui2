module ui2

$if !ui2_custom_rendering ? {
	fn windows_test_ready_resource(id string, opacity ImageOpacity) ImageResource {
		return ready_image_resource(id, 'sample.png', ImageResourceInput{
			width:    2
			height:   1
			channels: 4
			pixels:   []u8{len: 8, init: 255}
		}, opacity)
	}

	fn windows_test_repeat_pattern() RepeatPattern {
		mut pixels := []u8{len: 16, init: 255}
		pixels[0] = 255
		pixels[5] = 255
		pixels[10] = 255
		pixels[15] = 255
		return RepeatPattern{
			id:           'windows-pattern'
			tile_width:   2
			tile_height:  2
			pixel_width:  2
			pixel_height: 2
			channels:     4
			pixels:       pixels
		}
	}

	fn windows_test_repeat_pattern_32() RepeatPattern {
		mut pixels := []u8{len: 32 * 32 * 4, init: 255}
		pixels[4 * 4] = 0
		pixels[4 * 4 + 1] = 255
		pixels[4 * 4 + 2] = 0
		pixels[28 * 4] = 0
		pixels[28 * 4 + 1] = 0
		pixels[28 * 4 + 2] = 255
		return RepeatPattern{
			id:           'windows-pattern-32'
			tile_width:   32
			tile_height:  32
			pixel_width:  32
			pixel_height: 32
			channels:     4
			pixels:       pixels
		}
	}

	fn test_windows_image_modes_preserve_states_and_legacy_paths() {
		loading := transformed_image_resource('loading', loading_image_resource('loading-id',
			'pending.png'), rect(0, 0, 20, 10), 0, false)
		assert windows_image_mode(loading) == .pending
		assert windows_image_action(windows_image_mode(loading)) == .retain
		failed := transformed_image_resource('error', error_image_resource('error-id',
			'broken.png', 'decode failed'), rect(0, 0, 20, 10), 0, false)
		assert windows_image_mode(failed) == .pending
		assert windows_image_action(windows_image_mode(failed)) == .retain
		for opacity in [ImageOpacity.unknown, .proven_opaque, .has_alpha] {
			ready := transformed_image_resource('ready', windows_test_ready_resource('ready-id',
				opacity), rect(0, 0, 20, 10), 0, false)
			assert windows_image_mode(ready) == .decoded
			assert ready.image_resource.opacity == opacity
			assert image_path_for_element(ready) == 'sample.png'
			assert windows_image_resource_signature(ready).ends_with('rgba')
		}
		legacy := transformed_image('legacy', 'legacy.bmp', rect(0, 0, 20, 10), 0, false)
		assert windows_image_mode(legacy) == .legacy
		assert image_path_for_element(legacy) == 'legacy.bmp'
		assert control_support(.image) == .supported
	}

	fn test_windows_native_image_options_preserve_transforms_and_filter() {
		resource := windows_test_ready_resource('ready-id', .has_alpha)
		mut element := transformed_image_resource('image', resource, rect(10, 20, 40, 20),
			90, false)
		element = with_flip_h(with_flip_v(with_pixelated(element)))
		options := windows_image_options(element)
		assert options.frame == rect(10, 20, 40, 20)
		assert options.rotation == 90
		assert options.flip_h
		assert options.flip_v
		assert options.pixelated
		assert windows_native_frame(element) == rect(20, 30, 20, 40)
	}

	fn test_windows_pattern_identity_survives_resize_and_tracks_origin() {
		pattern := windows_test_repeat_pattern()
		resized := pattern
		assert windows_pattern_signature(pattern) == windows_pattern_signature(resized)
		mut moved := resized
		moved.origin_x = 16
		moved.origin_y = 8
		assert windows_pattern_signature(pattern) != windows_pattern_signature(moved)
	}

	$if windows ? {
		fn windows_test_channel(value u32, shift int) int {
			return int((value >> u32(shift)) & 0xff)
		}

		fn test_windows_decoded_bitmap_alpha_composites_over_existing_pixels() {
			pixels := [
				u8(255),
				u8(0),
				u8(0),
				u8(128),
				u8(0),
				u8(255),
				u8(0),
				u8(128),
			]
			bitmap := C.ui2_win_create_rgba_bitmap(voidptr(pixels.data), 2, 1)
			dc := C.ui2_win_create_test_dc(2, 1)
			assert bitmap != unsafe { nil }
			assert dc != unsafe { nil }
			defer {
				C.ui2_win_delete_test_dc(dc)
				C.ui2_win_delete_object(bitmap)
			}
			C.ui2_win_test_fill(dc, 0x0000ff)
			assert C.ui2_win_blend_decoded_bitmap(dc, bitmap, 2, 1, 2, 1, 0, 0, 0,
				0) != 0
			red_over_blue := C.ui2_win_test_pixel(dc, 0, 0)
			green_over_blue := C.ui2_win_test_pixel(dc, 1, 0)
			assert windows_test_channel(red_over_blue, 16) >= 120
			assert windows_test_channel(red_over_blue, 16) <= 136
			assert windows_test_channel(red_over_blue, 0) >= 120
			assert windows_test_channel(red_over_blue, 0) <= 136
			assert windows_test_channel(green_over_blue, 8) >= 120
			assert windows_test_channel(green_over_blue, 8) <= 136
			assert windows_test_channel(green_over_blue, 0) >= 120
			assert windows_test_channel(green_over_blue, 0) <= 136
		}

		fn test_windows_pending_and_error_resources_retain_previous_bitmap() {
			assert C.ui2_win_register_classes() != 0
			title := 'image retain test'.to_wide()
			root := C.ui2_win_create_main_window(title, 64, 64)
			unsafe {
				free(title)
			}
			assert root != unsafe { nil }
			empty := ''.to_wide()
			hwnd := C.ui2_win_create_widget(windows_widget_kind(.image), root, 0, 0, 32,
				32, empty, 0, 0, 0, 0, 0)
			unsafe {
				free(empty)
			}
			assert hwnd != unsafe { nil }
			defer {
				windows_cleanup_node_resources('retain-image', hwnd, .image)
				C.ui2_win_destroy(root)
			}
			ready := transformed_image_resource('retain-image',
				windows_test_ready_resource('retain-ready', .proven_opaque), rect(0, 0, 32, 32),
				0, false)
			windows_update_element('retain-image', hwnd, ready, 0, true)
			retained := C.ui2_win_get_bitmap(hwnd)
			assert retained != unsafe { nil }
			loading := transformed_image_resource('retain-image',
				loading_image_resource('retain-loading', 'pending.png'), rect(0, 0, 32, 32),
				0, false)
			windows_update_element('retain-image', hwnd, loading, 0, false)
			assert C.ui2_win_get_bitmap(hwnd) == retained
			failed := transformed_image_resource('retain-image',
				error_image_resource('retain-error', 'pending.png', 'failed'), rect(0, 0, 32, 32),
				0, false)
			windows_update_element('retain-image', hwnd, failed, 0, false)
			assert C.ui2_win_get_bitmap(hwnd) == retained
		}

		fn test_windows_decoded_bitmap_preserves_rotation_and_filtering() {
			pixels := [
				u8(255),
				u8(0),
				u8(0),
				u8(255),
				u8(0),
				u8(255),
				u8(0),
				u8(255),
			]
			bitmap := C.ui2_win_create_rgba_bitmap(voidptr(pixels.data), 2, 1)
			assert bitmap != unsafe { nil }
			defer {
				C.ui2_win_delete_object(bitmap)
			}
			dc := C.ui2_win_create_test_dc(2, 2)
			assert dc != unsafe { nil }
			defer {
				C.ui2_win_delete_test_dc(dc)
			}
			C.ui2_win_test_clear(dc)
			assert C.ui2_win_blend_decoded_bitmap(dc, bitmap, 2, 2, 1, 2, 90, 0, 0,
				1) != 0
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 0, 0), 16) > 200
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 1, 1), 8) > 200
			C.ui2_win_test_clear(dc)
			assert C.ui2_win_blend_decoded_bitmap(dc, bitmap, 2, 2, 1, 2, 90, 1, 0,
				1) != 0
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 0, 0), 8) > 200
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 1, 1), 16) > 200

			linear_dc := C.ui2_win_create_test_dc(4, 1)
			assert linear_dc != unsafe { nil }
			defer {
				C.ui2_win_delete_test_dc(linear_dc)
			}
			C.ui2_win_test_clear(linear_dc)
			assert C.ui2_win_blend_decoded_bitmap(linear_dc, bitmap, 4, 1, 4, 1, 0, 0,
				0, 0) != 0
			middle := C.ui2_win_test_pixel(linear_dc, 1, 0)
			assert windows_test_channel(middle, 16) > 32
			assert windows_test_channel(middle, 8) > 32
		}

		fn test_windows_repeat_pattern_phase_clipping_and_resize() {
			assert C.ui2_win_register_classes() != 0
			title := 'pattern test'.to_wide()
			root := C.ui2_win_create_main_window(title, 160, 128)
			unsafe {
				free(title)
			}
			assert root != unsafe { nil }
			defer {
				C.ui2_win_destroy(root)
			}
			empty := ''.to_wide()
			pattern_hwnd := C.ui2_win_create_widget(windows_widget_kind(.view), root, 3, 5,
				20, 20, empty, 0, 0, 0, 0, 0)
			unsafe {
				free(empty)
			}
			assert pattern_hwnd != unsafe { nil }
			defer {
				C.ui2_win_clear_repeat_pattern(pattern_hwnd)
			}
			pattern := windows_test_repeat_pattern()
			assert C.ui2_win_set_repeat_pattern(pattern_hwnd, voidptr(pattern.pixels.data),
				pattern.pixels.len, pattern.pixel_width, pattern.pixel_height,
				pattern.tile_width, pattern.tile_height, pattern.origin_x,
				pattern.origin_y) != 0
			dc := C.ui2_win_create_test_dc(160, 128)
			assert dc != unsafe { nil }
			defer {
				C.ui2_win_delete_test_dc(dc)
			}
			C.ui2_win_test_clear(dc)
			assert C.ui2_win_paint_repeat_pattern(dc, pattern_hwnd, root) != 0
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 3, 5), 16) == 255
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 4, 5), 8) == 255
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 5, 6), 16) == 255
			assert C.ui2_win_test_pixel(dc, 2, 5) == 0
			assert C.ui2_win_test_pixel(dc, 23, 25) == 0

			C.ui2_win_set_frame(pattern_hwnd, 3, 5, 30, 30)
			C.ui2_win_test_clear(dc)
			assert C.ui2_win_paint_repeat_pattern(dc, pattern_hwnd, root) != 0
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 3, 5), 16) == 255
			assert windows_test_channel(C.ui2_win_test_pixel(dc, 32, 34), 16) == 255
			assert C.ui2_win_test_pixel(dc, 33, 34) == 0

			C.ui2_win_set_frame(pattern_hwnd, 100, 60, 32, 32)
			phase_pattern := windows_test_repeat_pattern_32()
			assert C.ui2_win_set_repeat_pattern(pattern_hwnd, voidptr(phase_pattern.pixels.data),
				phase_pattern.pixels.len, phase_pattern.pixel_width, phase_pattern.pixel_height,
				phase_pattern.tile_width, phase_pattern.tile_height, phase_pattern.origin_x,
				phase_pattern.origin_y) != 0
			C.ui2_win_test_clear(dc)
			assert C.ui2_win_paint_repeat_pattern(dc, pattern_hwnd, root) != 0
			phase_pixel := C.ui2_win_test_pixel(dc, 100, 60)
			assert windows_test_channel(phase_pixel, 8) > 200
			assert windows_test_channel(phase_pixel, 16) < 50
		}
	}

	fn test_windows_virtual_keys_map_to_portable_key_codes() {
		assert windows_key_code(0x4e) == .n
		assert windows_key_code(0xbc) == .comma
		assert windows_key_code(0x71) == .f2
		assert windows_key_code(0xffff) == .invalid
	}

	fn test_windows_widget_kind_mapping_covers_every_native_control() {
		assert windows_widget_kind(.screen) == 0
		assert windows_widget_kind(.view) == 1
		assert windows_widget_kind(.scroll) == 2
		assert windows_widget_kind(.label) == 3
		assert windows_widget_kind(.image) == 4
		assert windows_widget_kind(.button) == 5
		assert windows_widget_kind(.checkbox) == 9
		assert windows_widget_kind(.dropdown) == 6
		assert windows_widget_kind(.text_field) == 7
		assert windows_widget_kind(.text_area) == 8
		assert windows_widget_kind(.slider) == 10
		assert windows_widget_kind(.switch_control) == 11
		assert windows_widget_kind(.toggle_button) == 12
	}

	fn test_windows_structural_transitions_recreate_controls() {
		plain := Element{
			kind: .text_field
		}
		secure := Element{
			kind:   .text_field
			secure: true
		}
		assert windows_structural_signature(plain) != windows_structural_signature(secure)

		area := Element{
			kind: .text_area
		}
		area_without_scroll := Element{
			kind:           .text_area
			disable_scroll: true
		}
		assert windows_structural_signature(area) != windows_structural_signature(area_without_scroll)

		styled_button := Element{
			kind:         .button
			native_style: true
		}
		plain_button := Element{
			kind: .button
		}
		assert windows_structural_signature(styled_button) != windows_structural_signature(plain_button)

		horizontal_slider := Element{
			kind: .slider
		}
		vertical_slider := Element{
			kind:        .slider
			orientation: .vertical
		}
		assert windows_structural_signature(horizontal_slider) != windows_structural_signature(vertical_slider)
	}

	fn test_windows_scroll_content_height_uses_child_extent() {
		children := [
			Element{
				kind:  .label
				frame: rect(0, 10, 50, 20)
			},
			Element{
				kind:  .button
				frame: rect(0, 80, 50, 35)
			},
		]
		assert windows_content_height(children) == 115
	}

	fn test_windows_transparent_push_buttons_use_custom_painting() {
		transparent := BoxStyle{
			transparent: true
		}
		assert windows_uses_transparent_button_paint(.button, transparent)
		assert windows_uses_transparent_button_paint(.toggle_button, transparent)
		assert !windows_uses_transparent_button_paint(.button, BoxStyle{})
		assert !windows_uses_transparent_button_paint(.checkbox, transparent)
	}

	fn test_windows_labels_never_paint_a_background_of_their_own() {
		// The default box is opaque white, so a label carrying it must still be
		// left alone: the view holding one paints what a label paints, and a label
		// given a border or a tooltip cannot come out white on a coloured parent.
		assert windows_draws_no_background(.label, BoxStyle{})
		assert windows_draws_no_background(.checkbox, BoxStyle{})
		assert windows_draws_no_background(.label, BoxStyle{
			border_left: 1
		})
		assert !windows_draws_no_background(.view, BoxStyle{})
		assert !windows_draws_no_background(.button, BoxStyle{})
		assert windows_draws_no_background(.view, BoxStyle{
			transparent: true
		})
	}

	fn windows_test_font_family(font voidptr) string {
		mut buffer := []u16{len: 32}
		C.ui2_win_font_family(font, unsafe { &buffer[0] }, buffer.len)
		return unsafe { string_from_wide(&buffer[0]) }
	}

	fn test_windows_font_glyph_key_only_tracks_characters_that_may_need_a_fallback() {
		assert windows_font_glyph_key('Bond, James') == ''
		assert windows_font_glyph_key('Andr\u00e9') == ''
		assert windows_font_glyph_key('\u2713 Bond, James') == '2713'
		assert windows_font_glyph_key('\u2713\u2713') == '2713'
		assert windows_font_glyph_key('\U0001f642\u2713') == '2713.1f642'
	}

	fn test_windows_font_text_covers_every_string_a_control_draws() {
		field := Element{
			kind:        .text_field
			text:        'Andr\u00e9'
			placeholder: 'Name'
		}
		assert windows_font_text(field) == 'Andr\u00e9Name'

		dropdown := Element{
			kind: .dropdown
			text: 'one'
			menu: [MenuEntry{
				id:    'two'
				title: '\u2713 two'
			}]
		}
		assert windows_font_text(dropdown) == 'one\u2713 two'

		// Context menu entries are drawn by the menu, not by the control font.
		button := Element{
			kind: .button
			text: 'Create'
			menu: [MenuEntry{
				id:    'copy'
				title: 'Copy'
			}]
		}
		assert windows_font_text(button) == 'Create'
	}

	fn test_windows_fonts_fall_back_to_a_family_that_has_the_glyphs() {
		default_family := ''.to_wide()
		plain := 'Bond, James'.to_wide()
		symbols := '\u2713 Bond, James'.to_wide()
		plain_font := C.ui2_win_create_font(unsafe { nil }, 13, default_family, 0, 0, 0,
			0, plain)
		symbol_font := C.ui2_win_create_font(unsafe { nil }, 13, default_family, 0, 0, 0,
			0, symbols)
		assert plain_font != unsafe { nil }
		assert symbol_font != unsafe { nil }
		// Text the UI font can draw keeps the UI font.
		assert windows_test_font_family(plain_font) == 'Segoe UI'
		assert C.ui2_win_font_missing_glyphs(plain_font, plain) == 0
		// A check mark is not in Segoe UI, so it must come from another family.
		assert C.ui2_win_font_missing_glyphs(symbol_font, symbols) == 0
		C.ui2_win_delete_object(plain_font)
		C.ui2_win_delete_object(symbol_font)
		unsafe {
			free(default_family)
			free(plain)
			free(symbols)
		}
	}

	fn test_windows_native_buttons_keep_the_system_font_until_glyphs_are_missing() {
		assert C.ui2_win_register_classes() != 0
		title := 'font test'.to_wide()
		root := C.ui2_win_create_main_window(title, 320, 200)
		unsafe {
			free(title)
		}
		assert root != unsafe { nil }
		defer {
			C.ui2_win_destroy(root)
		}

		plain := 'Bond, James'.to_wide()
		symbols := '\u2713 Bond, James'.to_wide()
		button := C.ui2_win_create_widget(windows_widget_kind(.button), root, 0, 0, 200,
			30, plain, 0, 0, 0, 0, 0)
		assert button != unsafe { nil }
		system_font := C.ui2_win_widget_font(button)
		C.ui2_win_apply_text_font(button, plain)
		assert C.ui2_win_widget_font(button) == system_font

		C.ui2_win_apply_text_font(button, symbols)
		assert C.ui2_win_font_missing_glyphs(C.ui2_win_widget_font(button), symbols) == 0
		unsafe {
			free(plain)
			free(symbols)
		}
	}

	fn test_windows_native_controls_keep_compact_text_layout() {
		assert C.ui2_win_register_classes() != 0
		assert C.ui2_win_visual_styles_enabled() != 0
		title := 'layout test'.to_wide()
		root := C.ui2_win_create_main_window(title, 320, 200)
		unsafe {
			free(title)
		}
		assert root != unsafe { nil }
		defer {
			C.ui2_win_destroy(root)
		}

		empty := ''.to_wide()
		field := C.ui2_win_create_widget(windows_widget_kind(.text_field), root, 0, 0, 200, 32, empty, 0, 0, 0, 0, 0)
		placeholder := 'First name'.to_wide()
		C.ui2_win_set_edit_options(field, placeholder, 0, 12)
		assert C.ui2_win_placeholder_matches(field, placeholder) != 0

		label := C.ui2_win_create_widget(windows_widget_kind(.label), root, 0, 40, 200, 32, empty, 0, 0, 0, 0, 0)
		checkbox := C.ui2_win_create_widget(windows_widget_kind(.checkbox), root, 0, 80, 210, 30, empty, 0, 0, 0, 0, 0)
		switch_view := C.ui2_win_create_widget(windows_widget_kind(.switch_control), root, 0, 120, 60, 32, empty, 0, 0, 0, 0, 0)
		// A label is placed by measuring it now, so it no longer asks the control to
		// centre a line on its behalf: SS_CENTERIMAGE is gone and SS_NOTIFY remains.
		assert C.ui2_win_widget_style(label) & usize(0x0200) == 0
		assert C.ui2_win_widget_style(label) & usize(0x0100) != 0
		measured_text := 'measured label'.to_wide()
		measured := C.ui2_win_create_widget(windows_widget_kind(.label), root, 0, 160,
			200, 32, measured_text, 0, 0, 0, 0, 0)
		unsafe {
			free(measured_text)
		}
		// One line is the font's own height; a budget of several lines wraps and is
		// capped to that budget rather than growing with the text.
		single := C.ui2_win_label_content_height(measured, 200, 1)
		assert single > 0
		assert C.ui2_win_label_content_height(measured, 40, 3) <= single * 3
		assert C.ui2_win_label_content_height(measured, 40, 3) >= single
		assert C.ui2_win_widget_style(checkbox) & usize(0x2000) == 0
		assert C.ui2_win_widget_style(switch_view) & usize(0x1000) != 0
		C.ui2_win_set_checked(switch_view, 1)
		assert C.ui2_win_get_checked(switch_view) != 0

		unsafe {
			free(empty)
			free(placeholder)
		}
	}
}
