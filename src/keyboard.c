#include "keyboard.h"
#include "vga_text.h"
#include "scheduler.h"

#define BUFFER_SIZE 1024

static char input_buffer[BUFFER_SIZE];

enum {
	SCAN_ESCAPE = 0x01,
	SCAN_1 = 0x02,
	SCAN_2 = 0x03,
	SCAN_3 = 0x04,
	SCAN_4 = 0x05,
	SCAN_5 = 0x06,
	SCAN_6 = 0x07,
	SCAN_7 = 0x08,
	SCAN_8 = 0x09,
	SCAN_9 = 0x0A,
	SCAN_0 = 0x0B,
	SCAN_MINUS = 0x0C,
	SCAN_EQUALS = 0x0D,
	SCAN_BACKSPACE = 0x0E,
	SCAN_TAB = 0x0F,
	SCAN_Q = 0x10,
	SCAN_W = 0x11,
	SCAN_E = 0x12,
	SCAN_R = 0x13,
	SCAN_T = 0x14,
	SCAN_Y = 0x15,
	SCAN_U = 0x16,
	SCAN_I = 0x17,
	SCAN_O = 0x18,
	SCAN_P = 0x19,
	SCAN_LEFT_BRACKET = 0x1A,
	SCAN_RIGHT_BRACKET = 0x1B,
	SCAN_ENTER = 0x1C,
	SCAN_LEFT_CONTROL = 0x1D,
	SCAN_A = 0x1E,
	SCAN_S = 0x1F,
	SCAN_D = 0x20,
	SCAN_F = 0x21,
	SCAN_G = 0x22,
	SCAN_H = 0x23,
	SCAN_J = 0x24,
	SCAN_K = 0x25,
	SCAN_L = 0x26,
	SCAN_SEMI_COLON = 0x27,
	SCAN_SINGLE_QUOTE = 0x28,
	SCAN_BACK_TICK = 0x29,
	SCAN_LEFT_SHIFT = 0x2A,
	SCAN_BACK_SLASH = 0x2B,
	SCAN_Z = 0x2C,
	SCAN_X = 0X2D,
	SCAN_C = 0x2E,
	SCAN_V = 0x2F,
	SCAN_B = 0x30,
	SCAN_N = 0x31,
	SCAN_M = 0x32,
	SCAN_COMMA = 0x33,
	SCAN_DOT = 0x34,
	SCAN_SLASH = 0x35,
	SCAN_RIGHT_SHIFT = 0x36,
	SCAN_KEYPAD_STAR = 0x37,
	SCAN_LEFT_ALT = 0x38,
	SCAN_SPACE = 0x39,
	SCAN_CAPSLOCK = 0x3A,
	SCAN_F1 = 0x3B,
	SCAN_F2 = 0x3C,
	SCAN_F3 = 0x3D,
	SCAN_F4 = 0x3E,
	SCAN_F5 = 0x3F,
	SCAN_F6 = 0x40,
	SCAN_F7 = 0x41,
	SCAN_F8 = 0x42,
	SCAN_F9 = 0x43,
	SCAN_F10 = 0x44,
	SCAN_NUMBERLOCK = 0x45,
	SCAN_SCROLLLOCK = 0x46,
	SCAN_KEYPAD_7 = 0x47,
	SCAN_KEYPAD_8 = 0x48,
	SCAN_KEYPAD_9 = 0x49,
	SCAN_KEYPAD_MINUS = 0x4A,
	SCAN_KEYPAD_4 = 0x4B,
	SCAN_KEYPAD_5 = 0x4C,
	SCAN_KEYPAD_6 = 0x4D,
	SCAN_KEYPAD_PLUS = 0x4E,
	SCAN_KEYPAD_1 = 0x4F,
	SCAN_KEYPAD_2 = 0x50,
	SCAN_KEYPAD_3 = 0x51,
	SCAN_KEYPAD_0 = 0x52,
	SCAN_KEYPAD_DOT = 0x53,
	SCAN_F11 = 0x57,
	SCAN_F12 = 0x58,
	SCAN_COUNT
};

static char scancode_map[] = {
	[0x02] = '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	[0x0F] = '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	[0x1E] = 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	[0x2B] = '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	[0x37] = '*',
	[0x39] = ' ',
	[0x47] = '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
};

static unsigned char is_pressed[SCAN_COUNT];
static unsigned char caps_lock = 0;

static int line_buffer_idx = 0;
static char line_buffer[256];

int circular_buffer_start = 0,
	circular_buffer_idx = 0,
	available = 4095;
static uint8_t circular_buffer[4096];

static struct task_wait buffer_wait = TASK_WAIT_DEFAULT;

// TODO: Don't fail when buffer is longer than 4096.
void circular_buffer_add(char c) {
	if (!available)
		return;

	if (circular_buffer_idx >= (int)COUNTOF(circular_buffer)) {
		circular_buffer_idx = 0;
	}

	circular_buffer[circular_buffer_idx++] = c;
	available--;
}

int circular_buffer_take(char *c) {
	if (available == 4095)
		return 0;

	if (circular_buffer_start >= (int)COUNTOF(circular_buffer)) {
		circular_buffer_start = 0;
	}

	*c = circular_buffer[circular_buffer_start++];
	available++;

	return 1;
}

void flush_line(void) {
	for (int i = 0; i < line_buffer_idx; i++)
		circular_buffer_add(line_buffer[i]);
	line_buffer_idx = 0;

	scheduler_unwait(&buffer_wait);
}

void add_char_to_buffer(char c) {
	if (line_buffer_idx >= (int)COUNTOF(line_buffer))
		return;

	line_buffer[line_buffer_idx++] = c;

	print_char(c);

	if (c == '\n') {
		flush_line();
	}
}

void remove_char_from_buffer(void) {
	if (line_buffer_idx == 0)
		return;

	print_char('\b');
	line_buffer_idx--;
}

int toupper(int c) {
	return (unsigned)c - 'a' < 26 ? c ^ 32 : c;
}

void keyboard_feed_scancode(unsigned char scancode) {
	//if (scancode == 0x2a)
	int pressed = 1;
	if (scancode >= 0x81 && scancode <= 0xD7) {
		pressed = 0;
		scancode -= 0x80;
	}

	if (scancode < SCAN_COUNT)
		is_pressed[scancode] = pressed;

	if (scancode > 0x57)
		return;

	if (pressed && scancode == SCAN_CAPSLOCK)
		caps_lock = !caps_lock;

	int uppercase = caps_lock || is_pressed[SCAN_LEFT_SHIFT] || is_pressed[SCAN_RIGHT_SHIFT];

	// If is printable.
	if (scancode < sizeof scancode_map / sizeof *scancode_map &&
		scancode_map[scancode] != 0 &&
		pressed) {
		char c = scancode_map[scancode];
		if (uppercase)
			c = toupper(c);

		add_char_to_buffer(c);
	}

	if (pressed && scancode == SCAN_BACKSPACE)
		remove_char_from_buffer();
}

static ssize_t keyboard_file_read(struct inode *inode, size_t *offset, void *data, size_t count) {
	ssize_t read = 0;

	uint8_t *user_buffer = data;

	while (available < 4095 || scheduler_wait(&buffer_wait)) {
		char c;
		while ((size_t)read < count &&
			   circular_buffer_take(&c)) {
			user_buffer[read++] = c;
		}

		if ((size_t)read == count)
			break;
	}

	return read;
}

static ssize_t keyboard_file_write(struct inode *inode, size_t *offset, const void *data, size_t count) {
	return 0;
}

void keyboard_init_inode(struct inode *inode) {
	inode->type = INODE_FILE;
	inode->data = NULL;
	inode->read = keyboard_file_read;
	inode->write = keyboard_file_write;
}
