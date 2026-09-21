module main

import ui2

fn reset_test_app() IdeApp {
	mut app := new_ide_app('.')
	app.snap_to_grid = false
	app.add_component('button', 24, 48)
	app.set_adaptive_enabled(true)
	app.select_edit_class(.compact, .any)
	assert app.set_geometry_property('property_x', '40')
	app.inspector_tab = 'layout'
	return app
}

fn reset_test_button(app &IdeApp) ui2.Element {
	for element in build_layout_inspector(274, app) {
		if element.id == 'layout_reset_variation' {
			return element
		}
	}
	panic('missing variation reset button')
}

fn test_variation_reset_button_requires_an_editable_size_class() {
	mut app := reset_test_app()
	assert reset_test_button(app).enabled
	for tab in ['source', 'preview'] {
		app.select_tab(tab)
		assert app.editing_variation()
		assert !app.geometry_editable()
		assert !reset_test_button(app).enabled, tab
		app.select_tab('designer')
		assert reset_test_button(app).enabled
	}
	app.select_edit_class(.any, .any)
	assert app.geometry_editable()
	assert !reset_test_button(app).enabled
	app.set_preview_size(390, 844)
	assert !app.geometry_editable()
	assert !reset_test_button(app).enabled
}

fn test_disabled_variation_reset_events_preserve_source_and_editing_state() {
	for tab in ['source', 'preview'] {
		for modified in [false, true] {
			mut app := reset_test_app()
			app.select_tab(tab)
			// Pending source may be incomplete. A disabled action must not even
			// try to apply it or report a source error, let alone reset the scope.
			if modified {
				app.source_text += '\nButton { // unfinished source edit'
			}
			app.source_modified = modified
			app.dirty = modified
			source := app.source_text
			before := app.snapshot()
			undo := app.undo_stack.clone()
			redo := app.redo_stack.clone()
			status := app.status
			messages := app.messages
			width := app.preview_width
			height := app.preview_height

			assert app.handle_adaptive_event('layout_reset_variation')
			assert app.snapshot() == before
			assert app.source_text == source
			assert app.source_modified == modified
			assert app.dirty == modified
			assert app.undo_stack == undo
			assert app.redo_stack == redo
			assert app.active_tab == tab
			assert app.edit_width_class == .compact
			assert app.edit_height_class == .any
			assert app.preview_width == width
			assert app.preview_height == height
			assert app.status == status
			assert app.messages == messages
		}
	}
}

fn test_variation_reset_model_rejects_non_designer_calls() {
	for tab in ['source', 'preview'] {
		mut app := reset_test_app()
		app.select_tab(tab)
		app.source_text += '\n// pending source edit'
		app.source_modified = true
		before := app.snapshot()
		source := app.source_text
		undo := app.undo_stack.clone()
		redo := app.redo_stack.clone()
		app.reset_selected_variation()
		assert app.snapshot() == before
		assert app.source_text == source
		assert app.source_modified
		assert app.undo_stack == undo
		assert app.redo_stack == redo
		assert app.active_tab == tab
		assert app.edit_width_class == .compact
		assert app.edit_height_class == .any
	}
}

fn test_designer_variation_reset_preserves_inheritance_and_undo_redo() {
	mut app := reset_test_app()
	parent := app.components[0].variations[0]
	app.select_edit_class(.compact, .compact)
	before_inherited_reset := app.snapshot()
	undo_count := app.undo_stack.len
	assert app.handle_adaptive_event('layout_reset_variation')
	assert app.snapshot() == before_inherited_reset
	assert app.undo_stack.len == undo_count

	assert app.set_geometry_property('property_y', '80')
	before := app.snapshot()
	before_source := app.source_text
	before_reset := app.undo_stack.len
	app.dirty = false
	assert reset_test_button(app).enabled
	assert app.handle_adaptive_event('layout_reset_variation')
	assert app.components[0].variations == [parent]
	assert app.undo_stack.len == before_reset + 1
	assert app.dirty
	assert !app.source_modified
	assert document_from_vml(app.source_text)!.components[0].variations == [parent]
	app.undo()
	assert app.snapshot() == before
	assert app.source_text == before_source
	app.redo()
	assert app.components[0].variations == [parent]
}
