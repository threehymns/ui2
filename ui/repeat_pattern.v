module ui2

import math

pub struct RepeatPattern {
pub mut:
	id           string
	tile_width   f64
	tile_height  f64
	pixel_width  int
	pixel_height int
	channels     int
	pixels       []u8
	origin_x     f64
	origin_y     f64
}

pub struct PatternBackground {
pub:
	pattern RepeatPattern
	clip    Rect
}

pub fn pattern_background(id string, pattern RepeatPattern, clip Rect) Element {
	return Element{
		kind:       .view
		id:         id
		frame:      clip
		box:        BoxStyle{
			transparent: true
		}
		background: PatternBackground{
			pattern: pattern
			clip:    clip
		}
	}
}

pub fn (pattern RepeatPattern) valid() bool {
	if pattern.tile_width <= 0 || pattern.tile_height <= 0 || pattern.pixel_width <= 0
		|| pattern.pixel_height <= 0 || pattern.channels != 4 {
		return false
	}
	return pattern.pixels.len >= pattern.pixel_width * pattern.pixel_height * pattern.channels
}

pub fn (pattern RepeatPattern) source_rect(frame Rect) Rect {
	if !pattern.valid() || frame.width <= 0 || frame.height <= 0 {
		return Rect{}
	}
	x_scale := f64(pattern.pixel_width) / pattern.tile_width
	y_scale := f64(pattern.pixel_height) / pattern.tile_height
	return Rect{
		x:      (frame.x - pattern.origin_x) * x_scale
		y:      (frame.y - pattern.origin_y) * y_scale
		width:  frame.width * x_scale
		height: frame.height * y_scale
	}
}

pub fn (pattern RepeatPattern) phase(frame Rect) Rect {
	if !pattern.valid() || frame.width <= 0 || frame.height <= 0 {
		return Rect{}
	}
	return Rect{
		x:      positive_pattern_remainder(pattern.origin_x - frame.x, pattern.tile_width)
		y:      positive_pattern_remainder(pattern.origin_y - frame.y, pattern.tile_height)
		width:  pattern.tile_width
		height: pattern.tile_height
	}
}

fn positive_pattern_remainder(value f64, modulus f64) f64 {
	mut result := math.fmod(value, modulus)
	if result < 0 {
		result += modulus
	}
	return result
}

fn repeat_pattern_cache_key(pattern RepeatPattern) string {
	mut hash := u64(14695981039346656037)
	for byte in pattern.pixels {
		hash ^= u64(byte)
		hash *= u64(1099511628211)
	}
	return '${pattern.id}:${pattern.tile_width}:${pattern.tile_height}:${pattern.pixel_width}:${pattern.pixel_height}:${pattern.channels}:${hash}'
}
