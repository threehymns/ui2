module ui2

fn adaptive_test_element(root Element, id string) ?Element {
	if root.id == id { return root }
	for child in root.children {
		if found := adaptive_test_element(child, id) { return found }
	}
	return none
}

fn test_adaptive_vml_resizes_against_actual_window_bounds() {
	source := 'Screen { adaptive: true width: 760 height: 520
		Button { id: pinned x: 624 y: 460 width: 112 height: 36 layout_x: end layout_y: end }
		TextField { id: wide x: 24 y: 24 width: 712 height: 36 layout_x: stretch }
		Label { id: centered x: 320 y: 248 width: 120 height: 24 layout_x: center layout_y: center }
	}'
	root := element_from_vml(source, rect(0, 0, 390, 844))!
	assert adaptive_test_element(root, 'pinned')?.frame == rect(254, 784, 112, 36)
	assert adaptive_test_element(root, 'wide')?.frame == rect(24, 24, 342, 36)
	assert adaptive_test_element(root, 'centered')?.frame == rect(135, 410, 120, 24)
	validate_element_tree(root)!
}

fn test_adaptive_vml_metadata_does_not_render_and_does_not_mutate_source() {
	source := 'Screen { adaptive: true width: 760 height: 520
		Button { id: action x: 620.5 y: 460.25 width: 112.5 height: 36.25 layout_x: end layout_y: end
			LayoutVariation { width_class: compact reference_width: 390 reference_height: 844
				x: 24.5 y: 784.25 width: 341 height: 36.25 layout_x: stretch layout_y: end }
		}
	}'
	node := parse_vml(source)!
	before := node.children[0].props.clone()
	for width in [390.0, 1280.0, 320.0, 600.0, 390.0] {
		root := element_from_vnode(node, rect(0, 0, width, 844))!
		control := adaptive_test_element(root, 'action')?
		assert control.children.len == 0
		assert node.children[0].props == before
		assert node.children[0].children.len == 1
		if width < 600 {
			assert control.frame.x == 24.5
			assert control.frame.width == width - 49
		} else {
			assert control.frame.x == width - 139.5
			assert control.frame.width == 112.5
		}
	}
}

fn test_adaptive_vml_selects_exact_class_then_width_then_height() {
	source := 'Screen { adaptive: true width: 760 height: 520
		Label { id: choice x: 1 y: 2 width: 80 height: 24 text: "Shared"
			LayoutVariation { height_class: compact reference_width: 760 reference_height: 520 x: 10 y: 2 width: 80 height: 24 }
			LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 20 y: 2 width: 80 height: 24 }
			LayoutVariation { width_class: compact height_class: compact reference_width: 390 reference_height: 320 x: 30 y: 2 width: 80 height: 24 hidden: true }
		}
	}'
	for size, expected_x in {
		'800x800': 1.0
		'800x400': 10.0
		'400x800': 20.0
		'400x400': 30.0
	} {
		parts := size.split('x')
		root := element_from_vml(source, rect(0, 0, parts[0].f64(), parts[1].f64()))!
		control := adaptive_test_element(root, 'choice')?
		assert control.frame.x == expected_x
		assert control.text == 'Shared'
		assert control.hidden == (size == '400x400')
	}
}

fn test_adaptive_vml_width_wins_single_axis_tie_independent_of_source_order() {
	for variants in [
		'LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 20 y: 2 width: 80 height: 24 }
		 LayoutVariation { height_class: compact reference_width: 760 reference_height: 520 x: 10 y: 2 width: 80 height: 24 }',
		'LayoutVariation { height_class: compact reference_width: 760 reference_height: 520 x: 10 y: 2 width: 80 height: 24 }
		 LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 20 y: 2 width: 80 height: 24 }',
	] {
		root := element_from_vml('Screen { adaptive: true width: 760 height: 520 Label { id: choice width: 80 height: 24 ${variants} } }', rect(0,
			0, 400, 400))!
		assert adaptive_test_element(root, 'choice')?.frame.x == 20
	}
}

fn test_adaptive_vml_is_opt_in_and_retains_dropdown_options() {
	root := element_from_vml('Screen { width: 760 height: 520
		Dropdown { id: choices x: 24 y: 30 width: 180 height: 36 layout_x: end
			Option { text: "One" } Option { text: "Two" }
			LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 10 y: 10 width: 100 height: 36 }
		}
	}', rect(0,
		0, 390, 844))!
	control := adaptive_test_element(root, 'choices')?
	assert control.frame == rect(24, 30, 180, 36)
	assert control.children.len == 0
}

fn test_adaptive_vml_rejects_invalid_constraints_and_variations() {
	invalid := [
		'Screen { adaptive: true width: 0 height: 520 }',
		'Screen { adaptive: true width: 760 height: 520 layout_breakpoint_width: 0 }',
		'Screen { adaptive: true width: 760 height: 520 Button { layout_x: sideways } }',
		'Screen { adaptive: true width: 760 height: 520 Button { width: -1 } }',
		'Screen { adaptive: true width: 760 height: 520 Button { layout_min_width: 100 layout_max_width: 50 } }',
		'Screen { adaptive: true width: 760 height: 520 Button { LayoutVariation { width_class: giant } } }',
		'Screen { adaptive: true width: 760 height: 520 Button { LayoutVariation { width_class: any height_class: any } } }',
		'Screen { adaptive: true width: 760 height: 520 Button { LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 0 y: 0 width: 100 height: 30 mystery: 1 } } }',
		'Screen { adaptive: true width: 760 height: 520 Button { LayoutVariation { width_class: compact } } }',
		'Screen { adaptive: true width: 760 height: 520 LayoutVariation {} }',
	]
	for source in invalid {
		if _ := element_from_vml(source, rect(0, 0, 390, 844)) {
			assert false, 'expected an invalid adaptive document to fail: ${source}'
		}
	}
}

fn test_adaptive_vml_duplicate_class_is_an_error() {
	variation := 'LayoutVariation { width_class: compact reference_width: 390 reference_height: 844 x: 0 y: 0 width: 100 height: 30 }'
	source := 'Screen { adaptive: true width: 760 height: 520 Button { ${variation} ${variation} } }'
	if _ := element_from_vml(source, rect(0, 0, 390, 844)) {
		assert false
	} else {
		assert err.msg().contains('duplicate LayoutVariation')
	}
}
