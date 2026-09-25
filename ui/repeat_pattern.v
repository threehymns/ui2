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

pub fn pattern_local_clip(clip Rect, frame_x f64, frame_y f64, bounds Rect) Rect {
	return intersect_rect(bounds, Rect{
		x:      clip.x - frame_x
		y:      clip.y - frame_y
		width:  clip.width
		height: clip.height
	})
}

fn positive_pattern_remainder(value f64, modulus f64) f64 {
	mut result := math.fmod(value, modulus)
	if result < 0 {
		result += modulus
	}
	return result
}

pub fn pattern_source_offset(origin f64, screen_origin f64, tile_size f64, pixel_size int) int {
	if tile_size <= 0 || pixel_size <= 0 {
		return 0
	}
	scale := f64(pixel_size) / tile_size
	offset := int(positive_pattern_remainder(screen_origin - origin, tile_size) * scale)
	return ((offset % pixel_size) + pixel_size) % pixel_size
}

pub fn first_pattern_tile_origin(clip_origin f64, phase f64, tile_size f64) f64 {
	if tile_size <= 0 {
		return 0
	}
	return math.floor((clip_origin - phase) / tile_size) * tile_size + phase
}

pub fn (background PatternBackground) visible_rect(parent_x f64, parent_y f64, frame Rect, parent_clip Rect) Rect {
	if !background.pattern.valid() {
		return Rect{}
	}
	mut clip := background.clip
	if clip.width <= 0 || clip.height <= 0 {
		clip = frame
	}
	visible := intersect_rect(rect(parent_x + clip.x, parent_y + clip.y, clip.width,
		clip.height), parent_clip)
	if visible.width <= 0 || visible.height <= 0 {
		return Rect{}
	}
	return visible
}

fn repeat_pattern_cache_key(pattern RepeatPattern) string {
	mut hash := u64(14695981039346656037)
	for byte in pattern.pixels {
		hash ^= u64(byte)
		hash *= u64(1099511628211)
	}
	return '${pattern.id}:${pattern.tile_width}:${pattern.tile_height}:${pattern.pixel_width}:${pattern.pixel_height}:${pattern.channels}:${hash}'
}
