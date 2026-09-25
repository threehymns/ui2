#ifndef UI2_NATIVE_HELPERS_WINDOWS_H
#define UI2_NATIVE_HELPERS_WINDOWS_H

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>

#define UI2_WM_REFRESH (WM_APP + 77)
#define UI2_WM_TRAY (WM_APP + 78)
#define UI2_WM_PAINT_BACKGROUND (WM_APP + 79)
#define UI2_WM_PAINT_PATTERNS (WM_APP + 80)

enum {
	UI2_WIN_VIEW = 1,
	UI2_WIN_SCROLL = 2,
	UI2_WIN_LABEL = 3,
	UI2_WIN_IMAGE = 4,
	UI2_WIN_BUTTON = 5,
	UI2_WIN_DROPDOWN = 6,
	UI2_WIN_TEXT_FIELD = 7,
	UI2_WIN_TEXT_AREA = 8,
	UI2_WIN_CHECKBOX = 9,
	UI2_WIN_SLIDER = 10,
	UI2_WIN_SWITCH = 11,
	UI2_WIN_TOGGLE_BUTTON = 12
};

extern intptr_t ui2_windows_window_proc(void *hwnd, unsigned int message,
		uintptr_t wparam, intptr_t lparam);
extern int ui2_windows_edit_submit(void *hwnd);
extern int ui2_windows_control_key(void *hwnd, unsigned int virtual_key);
extern int ui2_windows_context_menu(void *hwnd, int screen_x, int screen_y);
extern int ui2_windows_cursor(void *hwnd);
extern void ui2_windows_control_pointer(void *hwnd, unsigned int message, int x, int y);
extern void ui2_windows_control_border(void *hwnd);
extern int ui2_windows_is_transparent_button(void *hwnd);
extern int ui2_windows_paint_transparent_button(void *hwnd);
extern int ui2_windows_paint_image(void *hwnd);

static inline void ui2_win_refresh_text_font(HWND hwnd);
static inline void ui2_win_clear_repeat_pattern(void *hwnd);

static const wchar_t *ui2_win_placeholder_property(void) {
	return L"ui2.placeholder";
}

static inline void ui2_win_store_placeholder(HWND hwnd, const wchar_t *placeholder) {
	wchar_t *previous = (wchar_t *)GetPropW(hwnd, ui2_win_placeholder_property());
	const wchar_t *value = placeholder == NULL ? L"" : placeholder;
	if (previous != NULL && wcscmp(previous, value) == 0) return;
	if (previous != NULL) {
		RemovePropW(hwnd, ui2_win_placeholder_property());
		HeapFree(GetProcessHeap(), 0, previous);
	}
	if (value[0] == 0) return;
	size_t bytes = (wcslen(value) + 1) * sizeof(wchar_t);
	wchar_t *copy = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, bytes);
	if (copy == NULL) return;
	CopyMemory(copy, value, bytes);
	if (!SetPropW(hwnd, ui2_win_placeholder_property(), (HANDLE)copy)) {
		HeapFree(GetProcessHeap(), 0, copy);
	}
}

static inline void ui2_win_draw_placeholder(HWND hwnd) {
	const wchar_t *placeholder = (const wchar_t *)GetPropW(
		hwnd, ui2_win_placeholder_property());
	if (placeholder == NULL || placeholder[0] == 0 || GetFocus() == hwnd
		|| GetWindowTextLengthW(hwnd) != 0) return;
	HDC dc = GetDC(hwnd);
	if (dc == NULL) return;
	RECT rect;
	SendMessageW(hwnd, EM_GETRECT, 0, (LPARAM)&rect);
	HFONT font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
	HGDIOBJ previous_font = font == NULL ? NULL : SelectObject(dc, font);
	int previous_mode = SetBkMode(dc, TRANSPARENT);
	COLORREF previous_color = SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
	DrawTextW(dc, placeholder, -1, &rect,
		DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
	SetTextColor(dc, previous_color);
	SetBkMode(dc, previous_mode);
	if (previous_font != NULL) SelectObject(dc, previous_font);
	ReleaseDC(hwnd, dc);
}

static inline void ui2_win_release_placeholder(HWND hwnd) {
	wchar_t *placeholder = (wchar_t *)RemovePropW(hwnd, ui2_win_placeholder_property());
	if (placeholder != NULL) HeapFree(GetProcessHeap(), 0, placeholder);
}

static inline int ui2_win_apply_cursor(void *hwnd) {
	int cursor = ui2_windows_cursor(hwnd);
	LPCWSTR identifier = NULL;
	switch (cursor) {
	case 1: identifier = IDC_HAND; break;
	case 2: identifier = IDC_SIZENWSE; break;
	case 3: identifier = IDC_SIZENESW; break;
	case 4: identifier = IDC_SIZEWE; break;
	case 5: identifier = IDC_SIZENS; break;
	case 6: identifier = IDC_CROSS; break;
	default: break;
	}
	if (identifier == NULL) return 0;
	SetCursor(LoadCursorW(NULL, identifier));
	return 1;
}

static LRESULT CALLBACK ui2_win_control_subclass(HWND hwnd, UINT message, WPARAM wparam,
		LPARAM lparam, UINT_PTR subclass_id, DWORD_PTR reference_data) {
	(void)subclass_id;
	(void)reference_data;
	if (message == WM_KEYDOWN && wparam == VK_RETURN && ui2_windows_edit_submit(hwnd)) {
		return 0;
	}
	if (message == WM_KEYDOWN && ui2_windows_control_key(hwnd, (unsigned int)wparam)) {
		return 0;
	}
	if (message == WM_KEYDOWN && !IsWindow(hwnd)) return 0;
	if (message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT && ui2_win_apply_cursor(hwnd)) {
		return TRUE;
	}
	if (message == WM_CONTEXTMENU
		&& ui2_windows_context_menu(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam))) {
		return 0;
	}
	if (message == WM_LBUTTONDOWN || message == WM_MOUSEMOVE || message == WM_LBUTTONUP) {
		ui2_windows_control_pointer(hwnd, message, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		if (!IsWindow(hwnd)) return 0;
	}
	if (message == WM_ERASEBKGND && ui2_windows_is_transparent_button(hwnd)) {
		return 1;
	}
	if (message == WM_PAINT) {
		if (ui2_windows_paint_image(hwnd)) return 0;
		if (ui2_windows_paint_transparent_button(hwnd)) return 0;
		LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
		ui2_win_draw_placeholder(hwnd);
		ui2_windows_control_border(hwnd);
		return result;
	}
	if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_SETTEXT) {
		LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
		if (message == WM_SETTEXT && IsWindow(hwnd)) ui2_win_refresh_text_font(hwnd);
		InvalidateRect(hwnd, NULL, TRUE);
		return result;
	}
	if (message == WM_CHAR || message == WM_PASTE || message == EM_REPLACESEL) {
		LRESULT result = DefSubclassProc(hwnd, message, wparam, lparam);
		if (IsWindow(hwnd)) ui2_win_refresh_text_font(hwnd);
		return result;
	}
	if (message == WM_MOUSEWHEEL) {
		HWND parent = GetParent(hwnd);
		wchar_t class_name[64];
		while (parent != NULL) {
			class_name[0] = 0;
			GetClassNameW(parent, class_name, 64);
			if (wcscmp(class_name, L"UI2Container") == 0
				&& (GetWindowLongPtrW(parent, GWL_STYLE) & WS_VSCROLL) != 0) {
				SendMessageW(parent, message, wparam, lparam);
				return 0;
			}
			parent = GetParent(parent);
		}
	}
	if (message == WM_NCDESTROY) {
		ui2_win_release_placeholder(hwnd);
		ui2_win_clear_repeat_pattern(hwnd);
		RemoveWindowSubclass(hwnd, ui2_win_control_subclass, 1);
	}
	return DefSubclassProc(hwnd, message, wparam, lparam);
}

static LRESULT CALLBACK ui2_win_window_proc(HWND hwnd, UINT message, WPARAM wparam,
		LPARAM lparam) {
	if (message == WM_MOUSEWHEEL
		&& (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_VSCROLL) == 0) {
		HWND parent = GetParent(hwnd);
		while (parent != NULL) {
			if ((GetWindowLongPtrW(parent, GWL_STYLE) & WS_VSCROLL) != 0) {
				SendMessageW(parent, message, wparam, lparam);
				return 0;
			}
			parent = GetParent(parent);
		}
	}
	if (message == WM_CONTEXTMENU
		&& ui2_windows_context_menu(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam))) {
		return 0;
	}
	if (message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT && ui2_win_apply_cursor(hwnd)) {
		return TRUE;
	}
	if (message == WM_LBUTTONDOWN || message == WM_MOUSEMOVE || message == WM_LBUTTONUP) {
		ui2_windows_control_pointer(hwnd, message, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		if (!IsWindow(hwnd)) return 0;
	}
	return (LRESULT)ui2_windows_window_proc(hwnd, message, (uintptr_t)wparam,
		(intptr_t)lparam);
}

static inline COLORREF ui2_win_color(unsigned int rgb) {
	return RGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

static HANDLE ui2_win_visual_styles_context = INVALID_HANDLE_VALUE;
static ULONG_PTR ui2_win_visual_styles_cookie = 0;
static int ui2_win_visual_styles_initialized = 0;

static inline int ui2_win_enable_visual_styles(void) {
	if (ui2_win_visual_styles_initialized) {
		return ui2_win_visual_styles_context != INVALID_HANDLE_VALUE;
	}
	ui2_win_visual_styles_initialized = 1;

	wchar_t directory[MAX_PATH + 1];
	wchar_t path[MAX_PATH + 1];
	DWORD directory_length = GetTempPathW(MAX_PATH, directory);
	if (directory_length == 0 || directory_length > MAX_PATH
		|| GetTempFileNameW(directory, L"ui2", 0, path) == 0) {
		return 0;
	}

	static const char manifest[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
		"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
		"<assemblyIdentity name=\"ui2.runtime\" processorArchitecture=\"*\" "
			"version=\"1.0.0.0\" type=\"win32\"/>"
		"<dependency><dependentAssembly><assemblyIdentity type=\"win32\" "
			"name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" "
			"processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" "
			"language=\"*\"/></dependentAssembly></dependency></assembly>";

	HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		DeleteFileW(path);
		return 0;
	}
	DWORD written = 0;
	BOOL wrote_manifest = WriteFile(file, manifest, (DWORD)(sizeof(manifest) - 1),
		&written, NULL);
	CloseHandle(file);
	if (!wrote_manifest || written != sizeof(manifest) - 1) {
		DeleteFileW(path);
		return 0;
	}

	ACTCTXW activation;
	ZeroMemory(&activation, sizeof(activation));
	activation.cbSize = sizeof(activation);
	activation.lpSource = path;
	ui2_win_visual_styles_context = CreateActCtxW(&activation);
	DeleteFileW(path);
	if (ui2_win_visual_styles_context == INVALID_HANDLE_VALUE) return 0;
	if (!ActivateActCtx(ui2_win_visual_styles_context, &ui2_win_visual_styles_cookie)) {
		ReleaseActCtx(ui2_win_visual_styles_context);
		ui2_win_visual_styles_context = INVALID_HANDLE_VALUE;
		return 0;
	}
	return 1;
}

static inline int ui2_win_visual_styles_enabled(void) {
	return ui2_win_visual_styles_context != INVALID_HANDLE_VALUE;
}

static inline HFONT ui2_win_system_font(void) {
	static HFONT font = NULL;
	if (font != NULL) return font;
	NONCLIENTMETRICSW metrics;
	ZeroMemory(&metrics, sizeof(metrics));
	metrics.cbSize = sizeof(metrics);
	if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
		// The message font is reported with the locale charset, which switches
		// GDI font association off and paints a box for anything the face is
		// missing. DEFAULT_CHARSET lets GDI link to other installed fonts.
		metrics.lfMessageFont.lfCharSet = DEFAULT_CHARSET;
		font = CreateFontIndirectW(&metrics.lfMessageFont);
	}
	return font == NULL ? (HFONT)GetStockObject(DEFAULT_GUI_FONT) : font;
}

static inline int ui2_win_register_classes(void) {
	// Common Controls v6 supplies the current Windows button bezel and native
	// hover/pressed states. Activate it before registering or creating controls.
	ui2_win_enable_visual_styles();
	INITCOMMONCONTROLSEX controls;
	ZeroMemory(&controls, sizeof(controls));
	controls.dwSize = sizeof(controls);
	controls.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
	InitCommonControlsEx(&controls);

	HINSTANCE instance = GetModuleHandleW(NULL);
	WNDCLASSEXW cls;
	ZeroMemory(&cls, sizeof(cls));
	cls.cbSize = sizeof(cls);
	cls.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	cls.lpfnWndProc = ui2_win_window_proc;
	cls.hInstance = instance;
	cls.hCursor = LoadCursorW(NULL, IDC_ARROW);
	cls.lpszClassName = L"UI2Window";
	if (RegisterClassExW(&cls) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		return 0;
	}
	cls.lpszClassName = L"UI2Container";
	if (RegisterClassExW(&cls) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		return 0;
	}
	return 1;
}

static inline void *ui2_win_create_main_window(const wchar_t *title, int width, int height) {
	RECT frame = {0, 0, width, height};
	AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
	HWND hwnd = CreateWindowExW(0, L"UI2Window", title == NULL ? L"App" : title,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
		frame.right - frame.left, frame.bottom - frame.top, NULL, NULL,
		GetModuleHandleW(NULL), NULL);
	if (hwnd != NULL) {
		DragAcceptFiles(hwnd, TRUE);
	}
	return hwnd;
}

static inline void ui2_win_apply_min_size(void *hwnd_ptr, intptr_t lparam,
		int width, int height) {
	if (lparam == 0 || (width <= 0 && height <= 0)) return;
	HWND hwnd = (HWND)hwnd_ptr;
	RECT frame = {0, 0, width > 0 ? width : 0, height > 0 ? height : 0};
	AdjustWindowRectEx(&frame, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE),
		GetMenu(hwnd) != NULL, (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	MINMAXINFO *info = (MINMAXINFO *)lparam;
	if (width > 0) info->ptMinTrackSize.x = frame.right - frame.left;
	if (height > 0) info->ptMinTrackSize.y = frame.bottom - frame.top;
}

static inline void ui2_win_set_window_title(void *hwnd, const wchar_t *title) {
	if (hwnd != NULL) SetWindowTextW((HWND)hwnd, title == NULL ? L"" : title);
}

// SS_NOPREFIX because a label's text is text. Without it a static reads an ampersand
// as the marker before an accelerator key, swallowing it and underlining whatever
// follows, and the measurement below — which counts the ampersand as the character it
// is, as every other backend draws it — would wrap at a different width than the
// control does and place the block at the wrong height.
static inline DWORD ui2_win_label_style(int alignment) {
	DWORD alignment_style = SS_LEFT;
	if (alignment == 1) alignment_style = SS_CENTER;
	if (alignment == 2) alignment_style = SS_RIGHT;
	return alignment_style | SS_NOPREFIX;
}

// Height the control's own text needs, in the font the control is using.
// SS_CENTERIMAGE centres a single line of static text and nothing more, so a label is
// measured and moved rather than styled.
//
// One line is the font's own height, which the text metrics give straight away. Only
// a label allowed to wrap is laid out to find where its lines broke, so a screenful
// of ordinary labels does not pay for a layout each.
static inline int ui2_win_label_content_height(void *hwnd_ptr, int width, int lines) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL || width <= 0) return 0;
	HDC hdc = GetDC(hwnd);
	if (hdc == NULL) return 0;
	HFONT font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
	HFONT previous = NULL;
	if (font != NULL) previous = (HFONT)SelectObject(hdc, font);
	int height = 0;
	TEXTMETRICW metrics;
	int line_height = GetTextMetricsW(hdc, &metrics) ? (int)metrics.tmHeight : 0;
	if (lines > 1) {
		int length = GetWindowTextLengthW(hwnd);
		wchar_t *text = length > 0
			? (wchar_t *)malloc((size_t)(length + 1) * sizeof(wchar_t))
			: NULL;
		if (text != NULL) {
			GetWindowTextW(hwnd, text, length + 1);
			RECT rc;
			rc.left = 0;
			rc.top = 0;
			rc.right = width;
			rc.bottom = 0;
			DrawTextW(hdc, text, length, &rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
			height = (int)(rc.bottom - rc.top);
			free(text);
			// A static control shows as many lines as its rectangle holds, so text that
			// wraps past the caller's budget must not stretch the rectangle to fit: the
			// block is only ever as tall as the lines that were asked for.
			if (line_height > 0 && height > line_height * lines) height = line_height * lines;
		}
	} else {
		height = line_height;
	}
	if (previous != NULL) SelectObject(hdc, previous);
	ReleaseDC(hwnd, hdc);
	return height;
}

static inline DWORD ui2_win_edit_style(int alignment) {
	if (alignment == 1) return ES_CENTER;
	if (alignment == 2) return ES_RIGHT;
	return ES_LEFT;
}

static inline void *ui2_win_create_widget(int kind, void *parent_ptr, int x, int y,
		int width, int height, const wchar_t *text, int alignment, int secure,
		int readonly, int disable_scroll, int vertical) {
	HWND parent = (HWND)parent_ptr;
	DWORD style = WS_CHILD | WS_VISIBLE;
	DWORD ex_style = 0;
	const wchar_t *class_name = L"STATIC";
	switch (kind) {
	case UI2_WIN_VIEW:
		class_name = L"UI2Container";
		style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		ex_style = WS_EX_CONTROLPARENT;
		break;
	case UI2_WIN_SCROLL:
		class_name = L"UI2Container";
		style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VSCROLL;
		ex_style = WS_EX_CONTROLPARENT;
		break;
	case UI2_WIN_LABEL:
		class_name = L"STATIC";
		style |= ui2_win_label_style(alignment) | SS_NOTIFY;
		ex_style = WS_EX_TRANSPARENT;
		break;
	case UI2_WIN_IMAGE:
		class_name = L"STATIC";
		style |= SS_BITMAP | SS_CENTERIMAGE | SS_NOTIFY;
		break;
	case UI2_WIN_BUTTON:
		class_name = L"BUTTON";
		style |= BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_TABSTOP;
		break;
	case UI2_WIN_CHECKBOX:
		class_name = L"BUTTON";
		style |= BS_AUTOCHECKBOX | BS_LEFT | BS_VCENTER | WS_TABSTOP;
		break;
	case UI2_WIN_SWITCH:
		class_name = L"BUTTON";
		style |= BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_CENTER | BS_VCENTER | WS_TABSTOP;
		break;
	case UI2_WIN_TOGGLE_BUTTON:
		class_name = L"BUTTON";
		style |= BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_CENTER | BS_VCENTER | WS_TABSTOP;
		break;
	case UI2_WIN_DROPDOWN:
		class_name = L"COMBOBOX";
		style |= CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP;
		ex_style = WS_EX_CLIENTEDGE;
		break;
	case UI2_WIN_TEXT_FIELD:
		class_name = L"EDIT";
		style |= ui2_win_edit_style(alignment) | ES_AUTOHSCROLL | WS_TABSTOP;
		if (secure) style |= ES_PASSWORD;
		if (readonly) style |= ES_READONLY;
		ex_style = WS_EX_CLIENTEDGE;
		break;
	case UI2_WIN_TEXT_AREA:
		class_name = L"EDIT";
		style |= ui2_win_edit_style(alignment) | ES_MULTILINE | ES_AUTOVSCROLL
			| ES_WANTRETURN | WS_TABSTOP;
		if (!disable_scroll) style |= WS_VSCROLL;
		if (readonly) style |= ES_READONLY;
		ex_style = WS_EX_CLIENTEDGE;
		break;
	case UI2_WIN_SLIDER:
		class_name = TRACKBAR_CLASSW;
		style |= WS_TABSTOP | (vertical ? TBS_VERT : TBS_HORZ);
		break;
	default:
		return NULL;
	}
	HWND hwnd = CreateWindowExW(ex_style, class_name, text == NULL ? L"" : text, style,
		x, y, width, height, parent, NULL, GetModuleHandleW(NULL), NULL);
	if (hwnd != NULL && kind != UI2_WIN_VIEW && kind != UI2_WIN_SCROLL) {
		SendMessageW(hwnd, WM_SETFONT, (WPARAM)ui2_win_system_font(), TRUE);
		SetWindowSubclass(hwnd, ui2_win_control_subclass, 1, 0);
	}
	return hwnd;
}

#define UI2_WIN_SLIDER_STEPS 10000

static inline void ui2_win_slider_set_normalized(void *hwnd_ptr,
		double normalized, int vertical) {
	if (hwnd_ptr == NULL) return;
	if (normalized < 0.0) normalized = 0.0;
	if (normalized > 1.0) normalized = 1.0;
	double position = vertical ? 1.0 - normalized : normalized;
	HWND hwnd = (HWND)hwnd_ptr;
	SendMessageW(hwnd, TBM_SETRANGEMIN, FALSE, 0);
	SendMessageW(hwnd, TBM_SETRANGEMAX, FALSE, UI2_WIN_SLIDER_STEPS);
	SendMessageW(hwnd, TBM_SETPAGESIZE, 0, UI2_WIN_SLIDER_STEPS / 10);
	SendMessageW(hwnd, TBM_SETPOS, TRUE,
		(LPARAM)(int)(position * UI2_WIN_SLIDER_STEPS + 0.5));
}

static inline double ui2_win_slider_normalized(void *hwnd_ptr, int vertical) {
	if (hwnd_ptr == NULL) return 0.0;
	double position = (double)SendMessageW((HWND)hwnd_ptr, TBM_GETPOS, 0, 0)
		/ (double)UI2_WIN_SLIDER_STEPS;
	return vertical ? 1.0 - position : position;
}

static inline void ui2_win_show_main_window(void *hwnd_ptr) {
	HWND hwnd = (HWND)hwnd_ptr;
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);
}

static inline void ui2_win_set_checked(void *hwnd_ptr, int checked) {
	if (hwnd_ptr != NULL) {
		SendMessageW((HWND)hwnd_ptr, BM_SETCHECK,
			checked ? BST_CHECKED : BST_UNCHECKED, 0);
	}
}

static inline int ui2_win_get_checked(void *hwnd_ptr) {
	if (hwnd_ptr == NULL) return 0;
	return SendMessageW((HWND)hwnd_ptr, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

#define UI2_WIN_MAX_ACCELERATORS 128

static ACCEL ui2_win_accel_entries[UI2_WIN_MAX_ACCELERATORS];
static int ui2_win_accel_count = 0;
static HACCEL ui2_win_accel_table = NULL;
static HWND ui2_win_accel_window = NULL;

static inline int ui2_win_message_loop(void) {
	MSG message;
	while (GetMessageW(&message, NULL, 0, 0) > 0) {
		if (ui2_win_accel_table != NULL && ui2_win_accel_window != NULL
			&& TranslateAcceleratorW(ui2_win_accel_window, ui2_win_accel_table, &message)) {
			continue;
		}
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return (int)message.wParam;
}

static inline intptr_t ui2_win_default_proc(void *hwnd, unsigned int message,
		uintptr_t wparam, intptr_t lparam) {
	return (intptr_t)DefWindowProcW((HWND)hwnd, message, (WPARAM)wparam, (LPARAM)lparam);
}

static inline void ui2_win_post_quit(int code) {
	PostQuitMessage(code);
}

static inline void ui2_win_post_refresh(void *hwnd) {
	if (hwnd != NULL) PostMessageW((HWND)hwnd, UI2_WM_REFRESH, 0, 0);
}

static inline void ui2_win_close(void *hwnd) {
	if (hwnd != NULL) PostMessageW((HWND)hwnd, WM_CLOSE, 0, 0);
}

static inline void ui2_win_destroy(void *hwnd) {
	if (hwnd != NULL && IsWindow((HWND)hwnd)) DestroyWindow((HWND)hwnd);
}

static inline int ui2_win_is_window(void *hwnd) {
	return hwnd != NULL && IsWindow((HWND)hwnd);
}

static inline void *ui2_win_parent(void *hwnd) {
	return hwnd == NULL ? NULL : GetParent((HWND)hwnd);
}

static inline void ui2_win_set_parent(void *hwnd, void *parent) {
	if (hwnd != NULL) SetParent((HWND)hwnd, (HWND)parent);
}

static inline void ui2_win_set_frame(void *hwnd, int x, int y, int width, int height) {
	if (hwnd != NULL) {
		SetWindowPos((HWND)hwnd, NULL, x, y, width, height,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
	}
}

static inline void ui2_win_set_widget_frame(void *hwnd, int kind, int x, int y,
		int width, int height) {
	// Win32 uses the combo box window height for both the closed control and its
	// drop-down list. Give the list room without changing the visible row height.
	ui2_win_set_frame(hwnd, x, y, width,
		kind == UI2_WIN_DROPDOWN ? height + 240 : height);
}

static inline void ui2_win_place_after(void *hwnd, void *previous) {
	if (hwnd == NULL) return;
	(void)previous;
	// Rendering visits siblings from back to front. Moving each visited child to
	// the top gives later declarative siblings the same stacking priority used
	// by the other backends.
	SetWindowPos((HWND)hwnd, HWND_TOP,
		0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static inline void ui2_win_show(void *hwnd, int visible) {
	if (hwnd != NULL) ShowWindow((HWND)hwnd, visible ? SW_SHOWNA : SW_HIDE);
}

static inline void ui2_win_enable(void *hwnd, int enabled) {
	if (hwnd != NULL) EnableWindow((HWND)hwnd, enabled);
}

typedef struct ui2_win_tooltip_binding {
	HWND tooltip;
	wchar_t *text;
} ui2_win_tooltip_binding;

// Give a tooltip one more window to appear over. TTF_SUBCLASS has the tooltip watch
// that window's own mouse messages, so a window the pointer can land on has to be a
// target in its own right: a window with a child over it never sees the mouse there.
static inline int ui2_win_tooltip_add_target(void *binding_ptr, void *target_ptr) {
	ui2_win_tooltip_binding *binding = (ui2_win_tooltip_binding *)binding_ptr;
	HWND target = (HWND)target_ptr;
	if (binding == NULL || binding->tooltip == NULL || target == NULL) return 0;
	TOOLINFOW tool;
	ZeroMemory(&tool, sizeof(tool));
	tool.cbSize = sizeof(tool);
	tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	tool.hwnd = GetAncestor(target, GA_ROOT);
	tool.uId = (UINT_PTR)target;
	tool.lpszText = binding->text;
	return SendMessageW(binding->tooltip, TTM_ADDTOOLW, 0, (LPARAM)&tool) ? 1 : 0;
}

static inline void *ui2_win_create_tooltip(void *target_ptr, const wchar_t *text) {
	HWND target = (HWND)target_ptr;
	if (target == NULL || text == NULL || text[0] == 0) return NULL;
	ui2_win_tooltip_binding *binding = (ui2_win_tooltip_binding *)HeapAlloc(
		GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ui2_win_tooltip_binding));
	if (binding == NULL) return NULL;
	size_t bytes = (wcslen(text) + 1) * sizeof(wchar_t);
	binding->text = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, bytes);
	if (binding->text == NULL) {
		HeapFree(GetProcessHeap(), 0, binding);
		return NULL;
	}
	CopyMemory(binding->text, text, bytes);
	HWND owner = GetAncestor(target, GA_ROOT);
	binding->tooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, owner, NULL, GetModuleHandleW(NULL), NULL);
	if (binding->tooltip == NULL) {
		HeapFree(GetProcessHeap(), 0, binding->text);
		HeapFree(GetProcessHeap(), 0, binding);
		return NULL;
	}
	if (!ui2_win_tooltip_add_target(binding, target)) {
		DestroyWindow(binding->tooltip);
		HeapFree(GetProcessHeap(), 0, binding->text);
		HeapFree(GetProcessHeap(), 0, binding);
		return NULL;
	}
	SendMessageW(binding->tooltip, TTM_SETMAXTIPWIDTH, 0, 480);
	SetWindowPos(binding->tooltip, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	return binding;
}

static inline void ui2_win_destroy_tooltip(void *binding_ptr) {
	ui2_win_tooltip_binding *binding = (ui2_win_tooltip_binding *)binding_ptr;
	if (binding == NULL) return;
	if (binding->tooltip != NULL && IsWindow(binding->tooltip)) {
		DestroyWindow(binding->tooltip);
	}
	if (binding->text != NULL) HeapFree(GetProcessHeap(), 0, binding->text);
	HeapFree(GetProcessHeap(), 0, binding);
}

static inline void ui2_win_focus(void *hwnd) {
	if (hwnd != NULL) SetFocus((HWND)hwnd);
}

static inline void *ui2_win_focus_handle(void) {
	return GetFocus();
}

static inline void ui2_win_clear_focus(void *root) {
	if (root != NULL) SetFocus((HWND)root);
}

static inline int ui2_win_client_width(void *hwnd) {
	RECT rect;
	if (hwnd == NULL || !GetClientRect((HWND)hwnd, &rect)) return 0;
	return rect.right - rect.left;
}

static inline int ui2_win_client_height(void *hwnd) {
	RECT rect;
	if (hwnd == NULL || !GetClientRect((HWND)hwnd, &rect)) return 0;
	return rect.bottom - rect.top;
}

static inline int ui2_win_text_length(void *hwnd) {
	return hwnd == NULL ? 0 : GetWindowTextLengthW((HWND)hwnd);
}

static inline int ui2_win_get_text(void *hwnd, wchar_t *buffer, int capacity) {
	if (hwnd == NULL || buffer == NULL || capacity <= 0) return 0;
	return GetWindowTextW((HWND)hwnd, buffer, capacity);
}

static inline void ui2_win_set_text(void *hwnd, const wchar_t *text) {
	if (hwnd != NULL) SetWindowTextW((HWND)hwnd, text == NULL ? L"" : text);
}

static inline void ui2_win_set_edit_options(void *hwnd, const wchar_t *placeholder,
		int readonly, int padding_left) {
	if (hwnd == NULL) return;
	SendMessageW((HWND)hwnd, EM_SETREADONLY, readonly ? TRUE : FALSE, 0);
#ifdef EM_SETCUEBANNER
	// Draw cue text in the control subclass. This remains reliable under Wine,
	// where EM_SETCUEBANNER may report support without painting anything.
	SendMessageW((HWND)hwnd, EM_SETCUEBANNER, TRUE, (LPARAM)L"");
#endif
	ui2_win_store_placeholder((HWND)hwnd, placeholder);
	SendMessageW((HWND)hwnd, EM_SETMARGINS, EC_LEFTMARGIN,
		MAKELPARAM(padding_left < 0 ? 0 : padding_left, 0));
	InvalidateRect((HWND)hwnd, NULL, TRUE);
}

static inline int ui2_win_placeholder_matches(void *hwnd, const wchar_t *expected) {
	if (hwnd == NULL) return 0;
	const wchar_t *placeholder = (const wchar_t *)GetPropW(
		(HWND)hwnd, ui2_win_placeholder_property());
	const wchar_t *value = expected == NULL ? L"" : expected;
	return placeholder != NULL && wcscmp(placeholder, value) == 0;
}

static inline uintptr_t ui2_win_widget_style(void *hwnd) {
	return hwnd == NULL ? 0 : (uintptr_t)GetWindowLongPtrW((HWND)hwnd, GWL_STYLE);
}

static inline void ui2_win_get_selection(void *hwnd, unsigned int *start, unsigned int *end) {
	DWORD from = 0;
	DWORD to = 0;
	if (hwnd != NULL) SendMessageW((HWND)hwnd, EM_GETSEL, (WPARAM)&from, (LPARAM)&to);
	if (start != NULL) *start = from;
	if (end != NULL) *end = to;
}

static inline void ui2_win_set_selection(void *hwnd, unsigned int start, unsigned int end,
		int focus) {
	if (hwnd == NULL) return;
	if (focus) SetFocus((HWND)hwnd);
	SendMessageW((HWND)hwnd, EM_SETSEL, start, end);
	SendMessageW((HWND)hwnd, EM_SCROLLCARET, 0, 0);
}

static inline void ui2_win_replace_selection(void *hwnd, const wchar_t *text) {
	if (hwnd != NULL) SendMessageW((HWND)hwnd, EM_REPLACESEL, TRUE,
		(LPARAM)(text == NULL ? L"" : text));
}

static inline void ui2_win_combo_reset(void *hwnd) {
	if (hwnd != NULL) SendMessageW((HWND)hwnd, CB_RESETCONTENT, 0, 0);
}

static inline void ui2_win_combo_add(void *hwnd, const wchar_t *text) {
	if (hwnd != NULL) SendMessageW((HWND)hwnd, CB_ADDSTRING, 0,
		(LPARAM)(text == NULL ? L"" : text));
}

static inline void ui2_win_combo_select_text(void *hwnd, const wchar_t *text) {
	if (hwnd == NULL) return;
	LRESULT index = SendMessageW((HWND)hwnd, CB_FINDSTRINGEXACT, (WPARAM)-1,
		(LPARAM)(text == NULL ? L"" : text));
	SendMessageW((HWND)hwnd, CB_SETCURSEL, index == CB_ERR ? (WPARAM)-1 : (WPARAM)index, 0);
}

// Segoe UI, the Windows UI font, has no dingbats, no arrows and no emoji, so
// GDI paints a .notdef box for a check mark or a smiley. These families cover
// those ranges and keep the Segoe design for the rest of the string.
typedef struct {
	const wchar_t *family;
	int covers_astral;
} ui2_win_font_fallback;

static const ui2_win_font_fallback ui2_win_font_fallbacks[] = {
	{L"Segoe UI Symbol", 0},
	{L"Segoe UI Emoji", 1},
	{L"Segoe UI Historic", 0},
};

#define UI2_WIN_FONT_FALLBACK_COUNT \
	((int)(sizeof(ui2_win_font_fallbacks) / sizeof(ui2_win_font_fallbacks[0])))
// Control text is short; stop scanning long text area documents.
#define UI2_WIN_GLYPH_SCAN_LIMIT 1024

// Missing from some of the leaner Windows headers shipped with C compilers.
#ifndef GGI_MARK_NONEXISTING_GLYPHS
#define GGI_MARK_NONEXISTING_GLYPHS 1
#endif

// Characters that carry no glyph of their own, such as the variation selector
// that follows an emoji, are missing from every font by design.
static inline int ui2_win_glyph_optional(unsigned int unit) {
	if (unit < 0x20 || unit == 0x7f) return 1;
	if (unit >= 0x200b && unit <= 0x200f) return 1;
	if (unit >= 0x202a && unit <= 0x202e) return 1;
	if (unit >= 0xfe00 && unit <= 0xfe0f) return 1;
	return unit == 0xfeff;
}

// Counts the characters of `text` that `font` has no glyph for, or -1 when the
// font is unusable. `family`, when given, rejects a font the GDI mapper
// substituted because the requested family is not installed. `astral` reports
// characters above the BMP: GDI resolves glyphs per UTF-16 unit, so it always
// reports the surrogate halves of an emoji as missing.
static int ui2_win_font_gaps(HFONT font, const wchar_t *text, const wchar_t *family,
		int *astral) {
	if (astral != NULL) *astral = 0;
	if (font == NULL) return -1;
	HDC dc = CreateCompatibleDC(NULL);
	if (dc == NULL) return -1;
	HGDIOBJ previous = SelectObject(dc, font);
	int gaps = 0;
	if (family != NULL) {
		wchar_t face[LF_FACESIZE];
		face[0] = 0;
		if (GetTextFaceW(dc, LF_FACESIZE, face) == 0 || wcscmp(face, family) != 0) gaps = -1;
	}
	int index = 0;
	while (gaps >= 0 && text != NULL && text[index] != 0 && index < UI2_WIN_GLYPH_SCAN_LIMIT) {
		WORD glyphs[64];
		int count = 0;
		while (count < 64 && text[index + count] != 0) count++;
		if (GetGlyphIndicesW(dc, text + index, count, glyphs, GGI_MARK_NONEXISTING_GLYPHS)
			== GDI_ERROR) break;
		for (int i = 0; i < count; i++) {
			unsigned int unit = (unsigned int)text[index + i];
			if (unit >= 0xd800 && unit <= 0xdfff) {
				if (astral != NULL) *astral = 1;
			} else if (!ui2_win_glyph_optional(unit) && glyphs[i] == 0xffff) {
				gaps++;
			}
		}
		index += count;
	}
	SelectObject(dc, previous);
	DeleteDC(dc);
	return gaps;
}

static HFONT ui2_win_font_with_family(const LOGFONTW *base, const wchar_t *family) {
	LOGFONTW description = *base;
	description.lfCharSet = DEFAULT_CHARSET;
	lstrcpynW(description.lfFaceName, family, LF_FACESIZE);
	return CreateFontIndirectW(&description);
}

// Returns the family that can draw all of `text`, or NULL when `font` already
// can. A family is only picked when it has a glyph for every character, so the
// emoji font, which carries no letters, never takes over a mixed string.
static const wchar_t *ui2_win_fallback_family(HFONT font, const wchar_t *text) {
	if (text == NULL || text[0] == 0) return NULL;
	int astral = 0;
	int gaps = ui2_win_font_gaps(font, text, NULL, &astral);
	if (gaps <= 0 && !astral) return NULL;
	LOGFONTW base;
	if (GetObjectW(font, sizeof(base), &base) == 0) return NULL;
	const wchar_t *best = NULL;
	for (int i = 0; i < UI2_WIN_FONT_FALLBACK_COUNT; i++) {
		const ui2_win_font_fallback *candidate = &ui2_win_font_fallbacks[i];
		HFONT probe = ui2_win_font_with_family(&base, candidate->family);
		if (probe == NULL) continue;
		int probe_gaps = ui2_win_font_gaps(probe, text, candidate->family, NULL);
		DeleteObject(probe);
		if (probe_gaps != 0) continue;
		if (!astral || candidate->covers_astral) return candidate->family;
		if (gaps > 0 && best == NULL) best = candidate->family;
	}
	return best;
}

static inline void *ui2_win_create_font(void *hwnd, double point_size,
		const wchar_t *family, int bold, int italic, int underline, int strikeout,
		const wchar_t *text) {
	UINT dpi = 96;
	if (hwnd != NULL) {
		HDC dc = GetDC((HWND)hwnd);
		if (dc != NULL) {
			dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSY);
			ReleaseDC((HWND)hwnd, dc);
		}
	}
	double size = point_size > 0 ? point_size : 15.0;
	int height = -MulDiv((int)(size * 10.0), (int)dpi, 720);
	HFONT font = CreateFontW(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
		italic ? TRUE : FALSE, underline ? TRUE : FALSE, strikeout ? TRUE : FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		family == NULL || family[0] == 0 ? L"Segoe UI" : family);
	if (font == NULL) return NULL;
	const wchar_t *fallback = ui2_win_fallback_family(font, text);
	if (fallback == NULL) return font;
	LOGFONTW base;
	if (GetObjectW(font, sizeof(base), &base) == 0) return font;
	HFONT replacement = ui2_win_font_with_family(&base, fallback);
	if (replacement == NULL) return font;
	DeleteObject(font);
	return replacement;
}

// Reports how many characters of `text` the font cannot draw, so tests can
// assert that a control ended up with a font that covers its text.
static inline int ui2_win_font_missing_glyphs(void *font, const wchar_t *text) {
	return ui2_win_font_gaps((HFONT)font, text, NULL, NULL);
}

static inline void ui2_win_font_family(void *font, wchar_t *buffer, int capacity) {
	if (buffer == NULL || capacity <= 0) return;
	buffer[0] = 0;
	LOGFONTW description;
	if (font == NULL || GetObjectW((HFONT)font, sizeof(description), &description) == 0) return;
	lstrcpynW(buffer, description.lfFaceName, capacity);
}

static inline void *ui2_win_widget_font(void *hwnd) {
	return hwnd == NULL ? NULL : (void *)SendMessageW((HWND)hwnd, WM_GETFONT, 0, 0);
}

static inline void ui2_win_apply_font(void *hwnd, void *font) {
	if (hwnd != NULL && font != NULL) SendMessageW((HWND)hwnd, WM_SETFONT, (WPARAM)font, TRUE);
}

// Fallbacks are shared by every control that borrows them and are never
// destroyed, so a control can keep one for as long as it lives.
#define UI2_WIN_FONT_VARIANT_LIMIT 32

static HFONT ui2_win_font_variant(HFONT base, const wchar_t *family) {
	static LOGFONTW descriptions[UI2_WIN_FONT_VARIANT_LIMIT];
	static HFONT fonts[UI2_WIN_FONT_VARIANT_LIMIT];
	static int count = 0;
	LOGFONTW description;
	if (base == NULL || GetObjectW(base, sizeof(description), &description) == 0) return NULL;
	description.lfCharSet = DEFAULT_CHARSET;
	ZeroMemory(description.lfFaceName, sizeof(description.lfFaceName));
	lstrcpynW(description.lfFaceName, family, LF_FACESIZE);
	for (int i = 0; i < count; i++) {
		if (memcmp(&descriptions[i], &description, sizeof(LOGFONTW)) == 0) return fonts[i];
	}
	if (count == UI2_WIN_FONT_VARIANT_LIMIT) return NULL;
	HFONT font = CreateFontIndirectW(&description);
	if (font == NULL) return NULL;
	descriptions[count] = description;
	fonts[count] = font;
	count++;
	return font;
}

// Keeps the native system font on controls that render with the platform look,
// switching to a fallback family only for text the message font cannot draw.
static inline void ui2_win_apply_text_font(void *hwnd_ptr, const wchar_t *text) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return;
	HFONT font = ui2_win_system_font();
	const wchar_t *family = ui2_win_fallback_family(font, text);
	if (family != NULL) {
		HFONT variant = ui2_win_font_variant(font, family);
		if (variant != NULL) font = variant;
	}
	if ((HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0) != font) {
		SendMessageW(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
	}
}

// Text typed or pasted into an edit control never passes through the element
// tree, so the control picks its own fallback family for what it now holds.
static inline void ui2_win_refresh_text_font(HWND hwnd) {
	int length = GetWindowTextLengthW(hwnd);
	if (length <= 0 || length > UI2_WIN_GLYPH_SCAN_LIMIT) return;
	wchar_t text[UI2_WIN_GLYPH_SCAN_LIMIT + 1];
	if (GetWindowTextW(hwnd, text, length + 1) <= 0) return;
	HFONT base = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
	if (base == NULL) base = ui2_win_system_font();
	const wchar_t *family = ui2_win_fallback_family(base, text);
	if (family == NULL) return;
	HFONT variant = ui2_win_font_variant(base, family);
	if (variant != NULL && variant != base) {
		SendMessageW(hwnd, WM_SETFONT, (WPARAM)variant, TRUE);
	}
}

static inline void *ui2_win_create_brush(unsigned int color) {
	return CreateSolidBrush(ui2_win_color(color));
}

static inline void ui2_win_delete_object(void *object) {
	if (object != NULL) DeleteObject((HGDIOBJ)object);
}

static inline intptr_t ui2_win_apply_control_colors(void *dc_ptr, unsigned int foreground,
		unsigned int background, int transparent, void *brush) {
	HDC dc = (HDC)dc_ptr;
	if (dc == NULL) return 0;
	SetTextColor(dc, ui2_win_color(foreground));
	if (transparent) {
		SetBkMode(dc, TRANSPARENT);
		return (intptr_t)GetStockObject(HOLLOW_BRUSH);
	}
	SetBkMode(dc, OPAQUE);
	SetBkColor(dc, ui2_win_color(background));
	return (intptr_t)brush;
}

static inline int ui2_win_border_width(double width, int extent) {
	if (width <= 0.0 || extent <= 0) return 0;
	int pixels = (int)(width + 0.5);
	if (pixels < 1) pixels = 1;
	return pixels < extent ? pixels : extent;
}

static inline void ui2_win_draw_borders(HDC dc, const RECT *rect, unsigned int color,
		double left_width, double top_width, double right_width, double bottom_width) {
	if (dc == NULL || rect == NULL) return;
	int width = rect->right - rect->left;
	int height = rect->bottom - rect->top;
	int left = ui2_win_border_width(left_width, width);
	int top = ui2_win_border_width(top_width, height);
	int right = ui2_win_border_width(right_width, width);
	int bottom = ui2_win_border_width(bottom_width, height);
	if (left == 0 && top == 0 && right == 0 && bottom == 0) return;
	HBRUSH brush = CreateSolidBrush(ui2_win_color(color));
	if (brush == NULL) return;
	RECT edge;
	if (left > 0) {
		edge = (RECT){rect->left, rect->top, rect->left + left, rect->bottom};
		FillRect(dc, &edge, brush);
	}
	if (top > 0) {
		edge = (RECT){rect->left, rect->top, rect->right, rect->top + top};
		FillRect(dc, &edge, brush);
	}
	if (right > 0) {
		edge = (RECT){rect->right - right, rect->top, rect->right, rect->bottom};
		FillRect(dc, &edge, brush);
	}
	if (bottom > 0) {
		edge = (RECT){rect->left, rect->bottom - bottom, rect->right, rect->bottom};
		FillRect(dc, &edge, brush);
	}
	DeleteObject(brush);
}

static inline void ui2_win_paint_control_border(void *hwnd_ptr, unsigned int color,
		double radius, double left, double top, double right, double bottom) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return;
	HDC dc = GetDC(hwnd);
	if (dc == NULL) return;
	RECT rect;
	GetClientRect(hwnd, &rect);
	HRGN clip = NULL;
	if (radius > 0.5) {
		int diameter = (int)(radius * 2.0 + 0.5);
		clip = CreateRoundRectRgn(rect.left, rect.top, rect.right + 1, rect.bottom + 1,
			diameter, diameter);
		if (clip != NULL) SelectClipRgn(dc, clip);
	}
	ui2_win_draw_borders(dc, &rect, color, left, top, right, bottom);
	if (clip != NULL) {
		SelectClipRgn(dc, NULL);
		DeleteObject(clip);
	}
	ReleaseDC(hwnd, dc);
}

static inline void ui2_win_fill_background(HDC dc, const RECT *rect,
		unsigned int background, double radius) {
	HBRUSH brush = CreateSolidBrush(ui2_win_color(background));
	if (brush == NULL) return;
	if (radius > 0.5) {
		HPEN pen = CreatePen(PS_NULL, 0, ui2_win_color(background));
		HGDIOBJ old_brush = SelectObject(dc, brush);
		HGDIOBJ old_pen = SelectObject(dc, pen);
		int diameter = (int)(radius * 2.0 + 0.5);
		RoundRect(dc, rect->left, rect->top, rect->right, rect->bottom, diameter, diameter);
		SelectObject(dc, old_pen);
		SelectObject(dc, old_brush);
		DeleteObject(pen);
	} else {
		FillRect(dc, rect, brush);
	}
	DeleteObject(brush);
}

static inline void ui2_win_paint_parent_background(HWND hwnd, HDC dc) {
	HWND parent = GetParent(hwnd);
	if (parent == NULL) return;
	POINT parent_origin = {0, 0};
	MapWindowPoints(parent, hwnd, &parent_origin, 1);
	int saved = SaveDC(dc);
	OffsetViewportOrgEx(dc, parent_origin.x, parent_origin.y, NULL);
	SendMessageW(parent, UI2_WM_PAINT_BACKGROUND, (WPARAM)dc, 0);
	if (saved != 0) {
		RestoreDC(dc, saved);
	} else {
		OffsetViewportOrgEx(dc, -parent_origin.x, -parent_origin.y, NULL);
	}
}

static inline void ui2_win_paint_background_into(void *hwnd_ptr, void *dc_ptr,
		unsigned int background, double radius, int transparent,
		unsigned int border_color, double border_left, double border_top,
		double border_right, double border_bottom) {
	HWND hwnd = (HWND)hwnd_ptr;
	HDC dc = (HDC)dc_ptr;
	if (hwnd == NULL || dc == NULL) return;

	HWND parent = GetParent(hwnd);
	if (parent != NULL) ui2_win_paint_parent_background(hwnd, dc);
	RECT rect;
	GetClientRect(hwnd, &rect);
	if (parent == NULL) {
		ui2_win_fill_background(dc, &rect, background, radius);
		SendMessageW(hwnd, UI2_WM_PAINT_PATTERNS, (WPARAM)dc, 0);
	} else {
		SendMessageW(hwnd, UI2_WM_PAINT_PATTERNS, (WPARAM)dc, 0);
		if (!transparent) ui2_win_fill_background(dc, &rect, background, radius);
	}
	HRGN clip = NULL;
	if (radius > 0.5) {
		int diameter = (int)(radius * 2.0 + 0.5);
		clip = CreateRoundRectRgn(rect.left, rect.top, rect.right + 1, rect.bottom + 1,
			diameter, diameter);
		if (clip != NULL) SelectClipRgn(dc, clip);
	}
	ui2_win_draw_borders(dc, &rect, border_color, border_left, border_top,
		border_right, border_bottom);
	if (clip != NULL) {
		SelectClipRgn(dc, NULL);
		DeleteObject(clip);
	}
}

static inline void ui2_win_paint_background(void *hwnd_ptr, unsigned int background,
		double radius, int transparent, unsigned int border_color, double border_left,
		double border_top, double border_right, double border_bottom) {
	HWND hwnd = (HWND)hwnd_ptr;
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(hwnd, &paint);
	if (dc != NULL) {
		ui2_win_paint_background_into(hwnd, dc, background, radius, transparent,
			border_color, border_left, border_top, border_right, border_bottom);
	}
	EndPaint(hwnd, &paint);
}

static inline void ui2_win_paint_transparent_button(void *hwnd_ptr,
		unsigned int foreground, double radius, unsigned int border_color,
		double border_left, double border_top, double border_right,
		double border_bottom) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return;
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(hwnd, &paint);
	if (dc != NULL) {
		ui2_win_paint_parent_background(hwnd, dc);
		RECT rect;
		GetClientRect(hwnd, &rect);
		HRGN clip = NULL;
		if (radius > 0.5) {
			int diameter = (int)(radius * 2.0 + 0.5);
			clip = CreateRoundRectRgn(rect.left, rect.top, rect.right + 1,
				rect.bottom + 1, diameter, diameter);
			if (clip != NULL) SelectClipRgn(dc, clip);
		}
		ui2_win_draw_borders(dc, &rect, border_color, border_left, border_top,
			border_right, border_bottom);
		if (clip != NULL) {
			SelectClipRgn(dc, NULL);
			DeleteObject(clip);
		}

		int length = GetWindowTextLengthW(hwnd);
		wchar_t *text = (wchar_t *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			(size_t)(length + 1) * sizeof(wchar_t));
		if (text != NULL) {
			GetWindowTextW(hwnd, text, length + 1);
			HFONT font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
			HGDIOBJ old_font = font == NULL ? NULL : SelectObject(dc, font);
			int old_mode = SetBkMode(dc, TRANSPARENT);
			COLORREF old_color = SetTextColor(dc, IsWindowEnabled(hwnd)
				? ui2_win_color(foreground) : GetSysColor(COLOR_GRAYTEXT));
			RECT text_rect = rect;
			if ((SendMessageW(hwnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0) {
				OffsetRect(&text_rect, 1, 1);
			}
			DrawTextW(dc, text, length, &text_rect, DT_CENTER | DT_VCENTER
				| DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
			SetTextColor(dc, old_color);
			SetBkMode(dc, old_mode);
			if (old_font != NULL) SelectObject(dc, old_font);
			HeapFree(GetProcessHeap(), 0, text);
		}
		if (GetFocus() == hwnd) {
			RECT focus = rect;
			InflateRect(&focus, -3, -3);
			DrawFocusRect(dc, &focus);
		}
	}
	EndPaint(hwnd, &paint);
}

static inline void ui2_win_invalidate(void *hwnd) {
	if (hwnd != NULL) InvalidateRect((HWND)hwnd, NULL, TRUE);
}

static inline void ui2_win_invalidate_parent(void *hwnd) {
	if (hwnd != NULL) {
		HWND parent = GetParent((HWND)hwnd);
		if (parent != NULL) InvalidateRect(parent, NULL, TRUE);
	}
}

static inline HBITMAP ui2_win_create_rgba_bitmap(const unsigned char *pixels,
		int width, int height) {
	if (pixels == NULL || width <= 0 || height <= 0) return NULL;
	BITMAPINFO info;
	ZeroMemory(&info, sizeof(info));
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void *bits = NULL;
	HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	if (bitmap == NULL || bits == NULL) {
		if (bitmap != NULL) DeleteObject(bitmap);
		return NULL;
	}
	unsigned char *destination = (unsigned char *)bits;
	for (int y = 0; y < height; y++) {
		const unsigned char *source = pixels + (size_t)y * (size_t)width * 4u;
		unsigned char *row = destination + (size_t)y * (size_t)width * 4u;
		for (int x = 0; x < width; x++) {
			size_t offset = (size_t)x * 4u;
			row[offset] = source[offset + 2];
			row[offset + 1] = source[offset + 1];
			row[offset + 2] = source[offset];
			row[offset + 3] = source[offset + 3];
		}
	}
	return bitmap;
}

static inline void *ui2_win_set_decoded_bitmap(void *hwnd_ptr,
		const unsigned char *pixels, int width, int height) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return NULL;
	HBITMAP image = ui2_win_create_rgba_bitmap(pixels, width, height);
	HBITMAP old = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
	if (old != NULL && old != image) DeleteObject(old);
	return image;
}

static inline void *ui2_win_set_bitmap(void *hwnd_ptr, const wchar_t *path,
		int width, int height) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return NULL;
	HBITMAP image = NULL;
	if (path != NULL && path[0] != 0) {
		image = (HBITMAP)LoadImageW(NULL, path, IMAGE_BITMAP,
			width > 0 ? width : 0, height > 0 ? height : 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	}
	HBITMAP old = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image);
	if (old != NULL && old != image) DeleteObject(old);
	return image;
}

static inline void ui2_win_clear_bitmap(void *hwnd_ptr) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return;
	HBITMAP old = (HBITMAP)SendMessageW(hwnd, STM_SETIMAGE, IMAGE_BITMAP, 0);
	if (old != NULL) DeleteObject(old);
}

typedef struct ui2_win_repeat_binding {
	HBITMAP bitmap;
	int pixel_width;
	int pixel_height;
	double tile_width;
	double tile_height;
	double origin_x;
	double origin_y;
} ui2_win_repeat_binding;

static const wchar_t ui2_win_repeat_property_name[] = L"ui2.repeat_pattern";

static inline void ui2_win_clear_repeat_pattern(void *hwnd_ptr) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return;
	ui2_win_repeat_binding *binding = (ui2_win_repeat_binding *)RemovePropW(
		hwnd, ui2_win_repeat_property_name);
	if (binding == NULL) return;
	if (binding->bitmap != NULL) DeleteObject(binding->bitmap);
	HeapFree(GetProcessHeap(), 0, binding);
}

static inline int ui2_win_set_repeat_pattern(void *hwnd_ptr,
		const unsigned char *pixels, size_t pixel_len, int pixel_width, int pixel_height,
		double tile_width, double tile_height, double origin_x, double origin_y) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return 0;
	ui2_win_clear_repeat_pattern(hwnd);
	if (pixels == NULL || pixel_width <= 0 || pixel_height <= 0 || tile_width <= 0.0
		|| tile_height <= 0.0 || (size_t)pixel_width > pixel_len / 4u
		|| (size_t)pixel_height > pixel_len / 4u / (size_t)pixel_width) return 0;
	HBITMAP bitmap = ui2_win_create_rgba_bitmap(pixels, pixel_width, pixel_height);
	if (bitmap == NULL) return 0;
	ui2_win_repeat_binding *binding = (ui2_win_repeat_binding *)HeapAlloc(
		GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ui2_win_repeat_binding));
	if (binding == NULL) {
		DeleteObject(bitmap);
		return 0;
	}
	binding->bitmap = bitmap;
	binding->pixel_width = pixel_width;
	binding->pixel_height = pixel_height;
	binding->tile_width = tile_width;
	binding->tile_height = tile_height;
	binding->origin_x = origin_x;
	binding->origin_y = origin_y;
	if (!SetPropW(hwnd, ui2_win_repeat_property_name, (HANDLE)binding)) {
		HeapFree(GetProcessHeap(), 0, binding);
		DeleteObject(bitmap);
		return 0;
	}
	return 1;
}

static inline double ui2_win_positive_mod(double value, double extent) {
	double result = fmod(value, extent);
	if (result < 0.0) result += extent;
	return result;
}

static inline int ui2_win_pattern_is_below(void *pattern_ptr, void *target_ptr) {
	HWND pattern = (HWND)pattern_ptr;
	HWND target = (HWND)target_ptr;
	if (pattern == NULL || target == NULL) return 0;
	if (pattern == target || GetParent(pattern) == target) return 1;
	HWND ancestor = GetAncestor(target, GA_PARENT);
	while (ancestor != NULL) {
		if (ancestor == pattern) return 1;
		ancestor = GetParent(ancestor);
	}
	HWND root = GetAncestor(pattern, GA_ROOT);
	HWND pattern_branch = GetParent(root);
	HWND target_branch = GetParent(GetAncestor(target, GA_ROOT));
	if (pattern_branch == NULL || target_branch == NULL || pattern_branch != target_branch) return 0;
	HWND sibling = target_branch;
	while (sibling != NULL) {
		sibling = GetWindow(sibling, GW_HWNDPREV);
		if (sibling == pattern_branch) return 1;
	}
	return 0;
}

static inline int ui2_win_paint_repeat_pattern(void *dc_ptr, void *pattern_ptr,
		void *target_ptr) {
	HDC dc = (HDC)dc_ptr;
	HWND pattern_hwnd = (HWND)pattern_ptr;
	HWND target_hwnd = (HWND)target_ptr;
	if (dc == NULL || pattern_hwnd == NULL || target_hwnd == NULL) return 0;
	ui2_win_repeat_binding *binding = (ui2_win_repeat_binding *)GetPropW(
		pattern_hwnd, ui2_win_repeat_property_name);
	if (binding == NULL || binding->bitmap == NULL) return 0;
	RECT pattern_rect;
	GetClientRect(pattern_hwnd, &pattern_rect);
	POINT corners[2] = {{pattern_rect.left, pattern_rect.top},
		{pattern_rect.right, pattern_rect.bottom}};
	MapWindowPoints(pattern_hwnd, target_hwnd, corners, 2);
	int left = corners[0].x;
	int top = corners[0].y;
	int right = corners[1].x;
	int bottom = corners[1].y;
	if (left >= right || top >= bottom) return 0;
	HWND root = GetAncestor(pattern_hwnd, GA_ROOT);
	POINT root_position = {0, 0};
	MapWindowPoints(pattern_hwnd, root, &root_position, 1);
	double x_scale = (double)binding->pixel_width / binding->tile_width;
	double y_scale = (double)binding->pixel_height / binding->tile_height;
	int source_x = (int)(ui2_win_positive_mod((double)root_position.x -
		binding->origin_x, binding->tile_width) * x_scale) % binding->pixel_width;
	int source_y = (int)(ui2_win_positive_mod((double)root_position.y -
		binding->origin_y, binding->tile_height) * y_scale) % binding->pixel_height;
	if (fabs(x_scale - 1.0) < 0.000001 && fabs(y_scale - 1.0) < 0.000001) {
		int pattern_saved = SaveDC(dc);
		IntersectClipRect(dc, left, top, right, bottom);
		HBRUSH pattern_brush = CreatePatternBrush(binding->bitmap);
		POINT previous_origin;
		SetBrushOrgEx(dc, left - source_x, top - source_y, &previous_origin);
		HGDIOBJ previous_brush = pattern_brush == NULL ? NULL : SelectObject(dc, pattern_brush);
		RECT pattern_rect = {left, top, right, bottom};
		if (previous_brush != NULL) FillRect(dc, &pattern_rect, pattern_brush);
		if (previous_brush != NULL) SelectObject(dc, previous_brush);
		SetBrushOrgEx(dc, previous_origin.x, previous_origin.y, NULL);
		if (pattern_brush != NULL) DeleteObject(pattern_brush);
		if (pattern_saved != 0) RestoreDC(dc, pattern_saved);
		return pattern_brush != NULL;
	}
	int saved = SaveDC(dc);
	IntersectClipRect(dc, left, top, right, bottom);
	HDC source_dc = CreateCompatibleDC(dc);
	HGDIOBJ old_bitmap = source_dc == NULL ? NULL : SelectObject(source_dc, binding->bitmap);
	int old_mode = SetStretchBltMode(dc, COLORONCOLOR);
	int cell_width = (int)ceil(binding->tile_width);
	int cell_height = (int)ceil(binding->tile_height);
	if (cell_width < 1) cell_width = 1;
	if (cell_height < 1) cell_height = 1;
	for (int y = top; y < bottom; y += cell_height) {
		int cell_source_y = y == top ? source_y : 0;
		int destination_height = cell_height;
		if (y + destination_height > bottom) destination_height = bottom - y;
		int first_height = destination_height;
		if (first_height > 0) {
			int source_height = (int)ceil((double)first_height * y_scale);
			if (source_height > binding->pixel_height - cell_source_y) {
				source_height = binding->pixel_height - cell_source_y;
			}
			if (source_height < 1) source_height = 1;
			first_height = (int)ceil((double)source_height / y_scale);
			if (first_height > destination_height) first_height = destination_height;
		}
		int second_height = destination_height - first_height;
		for (int x = left; x < right; x += cell_width) {
			int cell_source_x = x == left ? source_x : 0;
			int destination_width = cell_width;
			if (x + destination_width > right) destination_width = right - x;
			int first_width = destination_width;
			if (first_width > 0) {
				int source_width = (int)ceil((double)first_width * x_scale);
				if (source_width > binding->pixel_width - cell_source_x) {
					source_width = binding->pixel_width - cell_source_x;
				}
				if (source_width < 1) source_width = 1;
				first_width = (int)ceil((double)source_width / x_scale);
				if (first_width > destination_width) first_width = destination_width;
			}
			int second_width = destination_width - first_width;
			if (source_dc == NULL || old_bitmap == NULL) continue;
			int first_source_width = (int)ceil((double)first_width * x_scale);
			int first_source_height = (int)ceil((double)first_height * y_scale);
			if (first_source_width > binding->pixel_width - cell_source_x) {
				first_source_width = binding->pixel_width - cell_source_x;
			}
			if (first_source_height > binding->pixel_height - cell_source_y) {
				first_source_height = binding->pixel_height - cell_source_y;
			}
			int second_source_width = (int)ceil((double)second_width * x_scale);
			int second_source_height = (int)ceil((double)second_height * y_scale);
			if (second_source_width > binding->pixel_width) second_source_width = binding->pixel_width;
			if (second_source_height > binding->pixel_height) second_source_height = binding->pixel_height;
			if (first_source_width > 0 && first_source_height > 0) {
				StretchBlt(dc, x, y, first_width, first_height, source_dc, cell_source_x,
					cell_source_y, first_source_width, first_source_height, SRCCOPY);
			}
			if (second_width > 0 && first_source_height > 0 && second_source_width > 0) {
				StretchBlt(dc, x + first_width, y, second_width, first_height,
					source_dc, 0, cell_source_y, second_source_width, first_source_height,
					SRCCOPY);
			}
			if (first_source_width > 0 && second_height > 0 && second_source_height > 0) {
				StretchBlt(dc, x, y + first_height, first_width, second_height,
					source_dc, cell_source_x, 0, first_source_width, second_source_height,
					SRCCOPY);
			}
			if (second_width > 0 && second_height > 0 && second_source_width > 0
				&& second_source_height > 0) {
				StretchBlt(dc, x + first_width, y + first_height, second_width,
					second_height, source_dc, 0, 0, second_source_width,
					second_source_height, SRCCOPY);
			}
		}
	}
	SetStretchBltMode(dc, old_mode);
	if (source_dc != NULL) {
		SelectObject(source_dc, old_bitmap);
		DeleteDC(source_dc);
	}
	if (saved != 0) RestoreDC(dc, saved);
	return 1;
}

static inline int ui2_win_clamp_int(int value, int minimum, int maximum) {
	if (value < minimum) return minimum;
	if (value > maximum) return maximum;
	return value;
}

static inline void ui2_win_source_pixel(const unsigned char *bits, int stride,
		int top_down, int width, int height, int x, int y, unsigned char *rgba) {
	x = ui2_win_clamp_int(x, 0, width - 1);
	y = ui2_win_clamp_int(y, 0, height - 1);
	int row = top_down ? y : height - 1 - y;
	const unsigned char *pixel = bits + (size_t)row * (size_t)stride + (size_t)x * 4u;
	rgba[0] = pixel[2];
	rgba[1] = pixel[1];
	rgba[2] = pixel[0];
	rgba[3] = pixel[3];
}

static inline void ui2_win_sample_image(const unsigned char *bits, int stride,
		int top_down, int width, int height, double source_x, double source_y,
		int nearest, unsigned char *rgba) {
	if (nearest) {
		ui2_win_source_pixel(bits, stride, top_down, width, height,
			(int)floor(source_x + 0.5), (int)floor(source_y + 0.5), rgba);
		return;
	}
	int x0 = (int)floor(source_x);
	int y0 = (int)floor(source_y);
	double x_fraction = source_x - floor(source_x);
	double y_fraction = source_y - floor(source_y);
	unsigned char c00[4], c10[4], c01[4], c11[4];
	ui2_win_source_pixel(bits, stride, top_down, width, height, x0, y0, c00);
	ui2_win_source_pixel(bits, stride, top_down, width, height, x0 + 1, y0, c10);
	ui2_win_source_pixel(bits, stride, top_down, width, height, x0, y0 + 1, c01);
	ui2_win_source_pixel(bits, stride, top_down, width, height, x0 + 1, y0 + 1, c11);
	for (int channel = 0; channel < 4; channel++) {
		double top = (double)c00[channel] * (1.0 - x_fraction)
			+ (double)c10[channel] * x_fraction;
		double bottom = (double)c01[channel] * (1.0 - x_fraction)
			+ (double)c11[channel] * x_fraction;
		rgba[channel] = (unsigned char)(top * (1.0 - y_fraction)
			+ bottom * y_fraction + 0.5);
	}
}

static inline int ui2_win_blend_decoded_bitmap(void *dc_ptr, void *bitmap_ptr,
		int output_width, int output_height, double frame_width, double frame_height,
		double rotation, int flip_h, int flip_v, int nearest) {
	HDC dc = (HDC)dc_ptr;
	HBITMAP bitmap = (HBITMAP)bitmap_ptr;
	if (dc == NULL || bitmap == NULL || output_width <= 0 || output_height <= 0
		|| frame_width <= 0.0 || frame_height <= 0.0) return 0;
	BITMAP source_bitmap;
	ZeroMemory(&source_bitmap, sizeof(source_bitmap));
	if (GetObjectW(bitmap, sizeof(source_bitmap), &source_bitmap) == 0
		|| source_bitmap.bmBits == NULL || source_bitmap.bmBitsPixel != 32) return 0;
	int source_width = source_bitmap.bmWidth < 0 ? -source_bitmap.bmWidth : source_bitmap.bmWidth;
	int source_height = source_bitmap.bmHeight < 0 ? -source_bitmap.bmHeight : source_bitmap.bmHeight;
	int top_down = 1;
	int stride = source_bitmap.bmWidthBytes;
	unsigned char *source_bits = (unsigned char *)source_bitmap.bmBits;
	if (source_width <= 0 || source_height <= 0) return 0;
	HDC buffer_dc = CreateCompatibleDC(dc);
	if (buffer_dc == NULL) return 0;
	BITMAPINFO output_info;
	ZeroMemory(&output_info, sizeof(output_info));
	output_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	output_info.bmiHeader.biWidth = output_width;
	output_info.bmiHeader.biHeight = -output_height;
	output_info.bmiHeader.biPlanes = 1;
	output_info.bmiHeader.biBitCount = 32;
	output_info.bmiHeader.biCompression = BI_RGB;
	void *output_bits = NULL;
	HBITMAP output_bitmap = CreateDIBSection(NULL, &output_info, DIB_RGB_COLORS,
		&output_bits, NULL, 0);
	if (output_bitmap == NULL) {
		DeleteDC(buffer_dc);
		return 0;
	}
	HGDIOBJ old_output = SelectObject(buffer_dc, output_bitmap);
	double normalized = fmod(rotation, 360.0);
	if (normalized < 0.0) normalized += 360.0;
	double cosine = 1.0;
	double sine = 0.0;
	if (normalized == 90.0 || normalized == 270.0) {
		cosine = 0.0;
		sine = 1.0;
	} else if (normalized == 180.0) {
		cosine = -1.0;
	} else if (normalized != 0.0) {
		double radians = normalized * 3.14159265358979323846 / 180.0;
		cosine = cos(radians);
		sine = sin(radians);
	}
	double center_x = (double)output_width / 2.0;
	double center_y = (double)output_height / 2.0;
	for (int y = 0; y < output_height; y++) {
		unsigned char *destination = (unsigned char *)output_bits
			+ (size_t)y * (size_t)output_width * 4u;
		for (int x = 0; x < output_width; x++) {
			double screen_x = (double)x + 0.5 - center_x;
			double screen_y = (double)y + 0.5 - center_y;
			double local_x = screen_x * cosine + screen_y * sine;
			double local_y = -screen_x * sine + screen_y * cosine;
			if (flip_h) local_x = -local_x;
			if (flip_v) local_y = -local_y;
			double source_x = (local_x / frame_width + 0.5) * (double)source_width - 0.5;
			double source_y = (local_y / frame_height + 0.5) * (double)source_height - 0.5;
			if (source_x < 0.0) source_x = 0.0;
			if (source_y < 0.0) source_y = 0.0;
			if (source_x > (double)(source_width - 1)) source_x = (double)(source_width - 1);
			if (source_y > (double)(source_height - 1)) source_y = (double)(source_height - 1);
			unsigned char rgba[4];
			ui2_win_sample_image(source_bits, stride, top_down, source_width,
				source_height, source_x, source_y, nearest, rgba);
			size_t offset = (size_t)x * 4u;
			destination[offset] = rgba[2];
			destination[offset + 1] = rgba[1];
			destination[offset + 2] = rgba[0];
			destination[offset + 3] = rgba[3];
		}
	}
	BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
	int result = AlphaBlend(dc, 0, 0, output_width, output_height, buffer_dc, 0, 0,
		output_width, output_height, blend);
	SelectObject(buffer_dc, old_output);
	DeleteObject(output_bitmap);
	DeleteDC(buffer_dc);
	return result;
}

static inline int ui2_win_paint_decoded_image(void *hwnd_ptr, double frame_width,
		double frame_height, double rotation, int flip_h, int flip_v, int nearest) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return 1;
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(hwnd, &paint);
	if (dc != NULL) {
		ui2_win_paint_parent_background(hwnd, dc);
		RECT rect;
		GetClientRect(hwnd, &rect);
		HBITMAP bitmap = (HBITMAP)SendMessageW(hwnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
		if (bitmap != NULL) {
			ui2_win_blend_decoded_bitmap(dc, bitmap, rect.right - rect.left,
				rect.bottom - rect.top, frame_width, frame_height, rotation,
				flip_h, flip_v, nearest);
		}
	}
	EndPaint(hwnd, &paint);
	return 1;
}

static inline void *ui2_win_create_test_dc(int width, int height) {
	HDC screen = GetDC(NULL);
	if (screen == NULL || width <= 0 || height <= 0) return NULL;
	HDC dc = CreateCompatibleDC(screen);
	ReleaseDC(NULL, screen);
	if (dc == NULL) return NULL;
	BITMAPINFO info;
	ZeroMemory(&info, sizeof(info));
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void *bits = NULL;
	HBITMAP bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);
	if (bitmap == NULL) {
		DeleteDC(dc);
		return NULL;
	}
	SelectObject(dc, bitmap);
	return dc;
}

static inline void ui2_win_test_fill(void *dc_ptr, unsigned int color) {
	HDC dc = (HDC)dc_ptr;
	if (dc == NULL) return;
	HBRUSH brush = CreateSolidBrush(ui2_win_color(color));
	HGDIOBJ previous = SelectObject(dc, brush);
	PatBlt(dc, 0, 0, INT_MAX, INT_MAX, PATCOPY);
	SelectObject(dc, previous);
	DeleteObject(brush);
}

static inline unsigned int ui2_win_test_pixel(void *dc_ptr, int x, int y) {
	HDC dc = (HDC)dc_ptr;
	if (dc == NULL) return 0;
	COLORREF color = GetPixel(dc, x, y);
	return ((unsigned int)GetRValue(color) << 16)
		| ((unsigned int)GetGValue(color) << 8)
		| (unsigned int)GetBValue(color);
}

static inline void ui2_win_test_clear(void *dc_ptr) {
	ui2_win_test_fill(dc_ptr, 0);
}

static inline void ui2_win_delete_test_dc(void *dc_ptr) {
	HDC dc = (HDC)dc_ptr;
	if (dc == NULL) return;
	HGDIOBJ bitmap = GetCurrentObject(dc, OBJ_BITMAP);
	HBITMAP placeholder = CreateBitmap(1, 1, 1, 1, NULL);
	if (placeholder != NULL) SelectObject(dc, placeholder);
	if (bitmap != NULL) DeleteObject(bitmap);
	DeleteDC(dc);
	if (placeholder != NULL) DeleteObject(placeholder);
}

static inline int ui2_win_set_scroll(void *hwnd_ptr, int content_height, int position) {
	HWND hwnd = (HWND)hwnd_ptr;
	RECT rect;
	GetClientRect(hwnd, &rect);
	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	info.nMin = 0;
	info.nMax = content_height > 0 ? content_height - 1 : 0;
	info.nPage = (UINT)(rect.bottom - rect.top);
	info.nPos = position;
	SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
	info.fMask = SIF_POS;
	GetScrollInfo(hwnd, SB_VERT, &info);
	return info.nPos;
}

static inline int ui2_win_scroll_message(void *hwnd_ptr, uintptr_t wparam) {
	HWND hwnd = (HWND)hwnd_ptr;
	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &info);
	int position = info.nPos;
	switch (LOWORD(wparam)) {
	case SB_TOP: position = info.nMin; break;
	case SB_BOTTOM: position = info.nMax; break;
	case SB_LINEUP: position -= 24; break;
	case SB_LINEDOWN: position += 24; break;
	case SB_PAGEUP: position -= (int)info.nPage; break;
	case SB_PAGEDOWN: position += (int)info.nPage; break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION: position = info.nTrackPos; break;
	default: break;
	}
	info.fMask = SIF_POS;
	info.nPos = position;
	SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &info);
	return info.nPos;
}

static inline int ui2_win_scroll_wheel(void *hwnd_ptr, uintptr_t wparam) {
	HWND hwnd = (HWND)hwnd_ptr;
	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &info);
	int delta = GET_WHEEL_DELTA_WPARAM(wparam);
	info.fMask = SIF_POS;
	info.nPos -= (delta / WHEEL_DELTA) * 48;
	SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &info);
	return info.nPos;
}

static inline int ui2_win_scroll_to_rect(void *hwnd_ptr, int top, int bottom) {
	HWND hwnd = (HWND)hwnd_ptr;
	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &info);
	int position = info.nPos;
	if (top < position) position = top;
	if (bottom > position + (int)info.nPage) position = bottom - (int)info.nPage;
	info.fMask = SIF_POS;
	info.nPos = position;
	SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &info);
	return info.nPos;
}

// Put a scroll bar at a position rather than scrolling the least amount that brings
// a rect into view. SetScrollInfo clamps to the range the element currently has, and
// reading back reports where it settled.
static inline int ui2_win_set_scroll_position(void *hwnd_ptr, int position) {
	HWND hwnd = (HWND)hwnd_ptr;
	SCROLLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize = sizeof(info);
	info.fMask = SIF_POS;
	info.nPos = position;
	SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
	GetScrollInfo(hwnd, SB_VERT, &info);
	return info.nPos;
}

// The static control a label draws with: the child of the view holding it, or the
// control itself when the label is not held in one.
static inline void *ui2_win_label_text_hwnd(void *hwnd_ptr) {
	HWND hwnd = (HWND)hwnd_ptr;
	if (hwnd == NULL) return NULL;
	HWND child = GetWindow(hwnd, GW_CHILD);
	return child != NULL ? (void *)child : (void *)hwnd;
}

static inline void ui2_win_capture_mouse(void *hwnd) {
	if (hwnd != NULL) SetCapture((HWND)hwnd);
}

static inline void ui2_win_release_mouse(void) {
	ReleaseCapture();
}

static inline unsigned long long ui2_win_ticks(void) {
	return GetTickCount64();
}

static inline void ui2_win_point_to_root(void *hwnd_ptr, void *root_ptr, int *x, int *y) {
	POINT point;
	point.x = x == NULL ? 0 : *x;
	point.y = y == NULL ? 0 : *y;
	MapWindowPoints((HWND)hwnd_ptr, (HWND)root_ptr, &point, 1);
	if (x != NULL) *x = point.x;
	if (y != NULL) *y = point.y;
}

static inline void *ui2_win_menu_create(void) {
	return CreatePopupMenu();
}

static inline void ui2_win_menu_add(void *menu, unsigned int command, const wchar_t *title,
		int enabled) {
	if (menu != NULL) AppendMenuW((HMENU)menu, MF_STRING | (enabled ? 0 : MF_GRAYED),
		command, title == NULL ? L"" : title);
}

static inline unsigned int ui2_win_menu_track(void *menu, void *hwnd, int x, int y) {
	if (menu == NULL || hwnd == NULL) return 0;
	if (x == -1 && y == -1) {
		RECT rect;
		GetWindowRect((HWND)hwnd, &rect);
		x = rect.left + (rect.right - rect.left) / 2;
		y = rect.top + (rect.bottom - rect.top) / 2;
	}
	return TrackPopupMenu((HMENU)menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
		x, y, 0, (HWND)hwnd, NULL);
}

static inline void ui2_win_menu_destroy(void *menu) {
	if (menu != NULL) DestroyMenu((HMENU)menu);
}

static inline void *ui2_win_menubar_create(void) {
	return CreateMenu();
}

static inline void ui2_win_menu_add_item(void *menu, unsigned int command,
		const wchar_t *title, int enabled, int checked) {
	if (menu == NULL) return;
	AppendMenuW((HMENU)menu, MF_STRING | (enabled ? MF_ENABLED : MF_GRAYED)
		| (checked ? MF_CHECKED : MF_UNCHECKED), command, title == NULL ? L"" : title);
}

static inline void ui2_win_menu_add_separator(void *menu) {
	if (menu != NULL) AppendMenuW((HMENU)menu, MF_SEPARATOR, 0, NULL);
}

// The parent menu takes ownership of the submenu, so only the outermost menu
// is ever destroyed by hand.
static inline void ui2_win_menu_add_submenu(void *menu, void *submenu,
		const wchar_t *title) {
	if (menu == NULL || submenu == NULL) return;
	AppendMenuW((HMENU)menu, MF_POPUP | MF_ENABLED, (UINT_PTR)(HMENU)submenu,
		title == NULL ? L"" : title);
}

static inline void ui2_win_set_menubar(void *hwnd, void *menu) {
	if (hwnd == NULL) return;
	HMENU previous = GetMenu((HWND)hwnd);
	SetMenu((HWND)hwnd, (HMENU)menu);
	if (previous != NULL) DestroyMenu(previous);
	DrawMenuBar((HWND)hwnd);
}

static inline void ui2_win_accel_reset(void) {
	ui2_win_accel_count = 0;
}

static inline void ui2_win_accel_add(int virtual_key, int control, int alt, int shift,
		unsigned int command) {
	if (virtual_key == 0 || ui2_win_accel_count >= UI2_WIN_MAX_ACCELERATORS) return;
	ACCEL *entry = &ui2_win_accel_entries[ui2_win_accel_count++];
	entry->fVirt = (BYTE)(FVIRTKEY | (control ? FCONTROL : 0) | (alt ? FALT : 0)
		| (shift ? FSHIFT : 0));
	entry->key = (WORD)virtual_key;
	entry->cmd = (WORD)command;
}

static inline void ui2_win_accel_install(void *hwnd) {
	if (ui2_win_accel_table != NULL) {
		DestroyAcceleratorTable(ui2_win_accel_table);
		ui2_win_accel_table = NULL;
	}
	ui2_win_accel_window = (HWND)hwnd;
	if (ui2_win_accel_count > 0) {
		ui2_win_accel_table = CreateAcceleratorTableW(ui2_win_accel_entries,
			ui2_win_accel_count);
	}
}

static NOTIFYICONDATAW ui2_win_tray_data;
static int ui2_win_tray_added = 0;
// Only an icon this layer loaded from a file is its to destroy; the stock
// application icon is shared.
static HICON ui2_win_tray_loaded_icon = NULL;

static inline unsigned int ui2_win_tray_message(void) {
	return UI2_WM_TRAY;
}

static inline void ui2_win_tray_set(void *hwnd, const wchar_t *tooltip,
		const wchar_t *icon_path) {
	if (hwnd == NULL) return;
	HICON icon = NULL;
	if (icon_path != NULL && icon_path[0] != L'\0') {
		icon = (HICON)LoadImageW(NULL, icon_path, IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_LOADFROMFILE);
	}
	HICON previous = ui2_win_tray_loaded_icon;
	ui2_win_tray_loaded_icon = icon;
	if (icon == NULL) icon = LoadIconW(NULL, IDI_APPLICATION);

	ZeroMemory(&ui2_win_tray_data, sizeof(ui2_win_tray_data));
	ui2_win_tray_data.cbSize = sizeof(ui2_win_tray_data);
	ui2_win_tray_data.hWnd = (HWND)hwnd;
	ui2_win_tray_data.uID = 1;
	ui2_win_tray_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	ui2_win_tray_data.uCallbackMessage = UI2_WM_TRAY;
	ui2_win_tray_data.hIcon = icon;
	if (tooltip != NULL) {
		size_t capacity = sizeof(ui2_win_tray_data.szTip) / sizeof(wchar_t);
		wcsncpy(ui2_win_tray_data.szTip, tooltip, capacity - 1);
		ui2_win_tray_data.szTip[capacity - 1] = L'\0';
	}
	Shell_NotifyIconW(ui2_win_tray_added ? NIM_MODIFY : NIM_ADD, &ui2_win_tray_data);
	ui2_win_tray_added = 1;
	if (previous != NULL) DestroyIcon(previous);
}

static inline void ui2_win_tray_remove(void) {
	if (!ui2_win_tray_added) return;
	Shell_NotifyIconW(NIM_DELETE, &ui2_win_tray_data);
	ui2_win_tray_added = 0;
	if (ui2_win_tray_loaded_icon != NULL) {
		DestroyIcon(ui2_win_tray_loaded_icon);
		ui2_win_tray_loaded_icon = NULL;
	}
}

// A notification area menu only dismisses when its owner is foreground, and
// the trailing WM_NULL is the documented workaround for the first click after
// it closes being swallowed.
static inline unsigned int ui2_win_tray_popup(void *hwnd, void *menu) {
	if (hwnd == NULL || menu == NULL) return 0;
	POINT point;
	GetCursorPos(&point);
	SetForegroundWindow((HWND)hwnd);
	unsigned int command = (unsigned int)TrackPopupMenu((HMENU)menu,
		TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, point.x, point.y, 0, (HWND)hwnd,
		NULL);
	PostMessageW((HWND)hwnd, WM_NULL, 0, 0);
	return command;
}

static inline unsigned int ui2_win_drop_count(void *drop) {
	return DragQueryFileW((HDROP)drop, 0xffffffff, NULL, 0);
}

static inline unsigned int ui2_win_drop_path_length(void *drop, unsigned int index) {
	return DragQueryFileW((HDROP)drop, index, NULL, 0);
}

static inline void ui2_win_drop_path(void *drop, unsigned int index, wchar_t *buffer,
		unsigned int capacity) {
	DragQueryFileW((HDROP)drop, index, buffer, capacity);
}

static inline void ui2_win_drop_point(void *drop, int *x, int *y) {
	POINT point;
	point.x = 0;
	point.y = 0;
	DragQueryPoint((HDROP)drop, &point);
	if (x != NULL) *x = point.x;
	if (y != NULL) *y = point.y;
}

static inline void ui2_win_drop_finish(void *drop) {
	DragFinish((HDROP)drop);
}

static inline int ui2_win_key_down(int virtual_key) {
	return (GetKeyState(virtual_key) & 0x8000) != 0;
}

#endif
