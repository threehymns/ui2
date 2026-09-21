module ui2

import math

// AdaptiveAxis describes a control's relationship to its parent's physical
// edges. Center preserves the design-time center offset; stretch preserves both
// edge insets. This is intentionally not a general constraint solver.
pub enum AdaptiveAxis {
	start
	end
	center
	stretch
}

pub enum AdaptiveSizeClass {
	any
	compact
	regular
}

pub struct AdaptiveLayout {
pub mut:
	horizontal AdaptiveAxis
	vertical   AdaptiveAxis
	min_width  f64
	max_width  f64
	min_height f64
	max_height f64
	hidden     bool
}

// A variation replaces the frame, reference canvas and layout rules as a unit.
// Text, events and other control properties continue to come from the base.
pub struct AdaptiveLayoutVariation {
pub mut:
	width_class      AdaptiveSizeClass
	height_class     AdaptiveSizeClass
	frame            Rect
	reference_width  f64
	reference_height f64
	layout           AdaptiveLayout
}

pub fn adaptive_size_class(length f64, breakpoint f64) AdaptiveSizeClass {
	return if length < breakpoint { .compact } else { .regular }
}

pub fn adaptive_class_matches(selector AdaptiveSizeClass, actual AdaptiveSizeClass) bool {
	return selector == .any || selector == actual
}

// Exact width+height beats single-axis rules. Width-specific rules win ties
// with height-specific rules, independently of the order in the document.
pub fn adaptive_variation_index(variations []AdaptiveLayoutVariation, width_class AdaptiveSizeClass, height_class AdaptiveSizeClass) int {
	mut result := -1
	mut best := -1
	for index, variation in variations {
		if !adaptive_class_matches(variation.width_class, width_class)
			|| !adaptive_class_matches(variation.height_class, height_class) {
			continue
		}
		score := (if variation.width_class != .any { 2 } else { 0 }) +
			(if variation.height_class != .any { 1 } else { 0 })
		if score > best {
			result = index
			best = score
		}
	}
	return result
}

fn adaptive_axis_frame(position f64, size f64, reference f64, available f64, axis AdaptiveAxis, minimum f64, maximum f64) (f64, f64) {
	mut resolved_size := if axis == .stretch { size + available - reference } else { size }
	if resolved_size < minimum {
		resolved_size = minimum
	}
	if resolved_size < 0 {
		resolved_size = 0
	}
	if maximum > 0 && resolved_size > maximum {
		resolved_size = maximum
	}
	resolved_position := match axis {
		.start, .stretch { position }
		.end { available - (reference - position - size) - resolved_size }
		.center { position + (available - reference + size - resolved_size) / 2 }
	}

	return resolved_position, resolved_size
}

// Resolve from the saved reference frame, never from a previously resized
// frame. Repeated resizing therefore cannot accumulate drift. Coordinates are
// parent-local; available.x/y are deliberately not added to the result.
// Min/max limits take precedence over stretch's far edge, retaining its near
// edge. Overflow is not silently moved or scaled away.
pub fn adaptive_layout_frame(design Rect, reference_width f64, reference_height f64, available Rect, layout AdaptiveLayout) Rect {
	x, width := adaptive_axis_frame(design.x, design.width, reference_width, available.width,
		layout.horizontal, layout.min_width, layout.max_width)
	y, height := adaptive_axis_frame(design.y, design.height, reference_height, available.height,
		layout.vertical, layout.min_height, layout.max_height)
	return rect(x, y, width, height)
}

pub fn validate_adaptive_layout(layout AdaptiveLayout) ! {
	for value in [layout.min_width, layout.max_width, layout.min_height, layout.max_height] {
		if math.is_nan(value) || math.is_inf(value, 0) || value < 0 {
			return error('layout size limits must be finite, non-negative numbers (0 means no maximum)')
		}
	}
	if (layout.max_width > 0 && layout.max_width < layout.min_width)
		|| (layout.max_height > 0 && layout.max_height < layout.min_height) {
		return error('layout maximum must not be smaller than minimum')
	}
}
