#include "input_center.h"
#define CONIOEX
#include "conioex.h"

// 記録するフレーム数
#define INPUT_RECORD_FRAMES 2

static bool inputs[INPUT_RECORD_FRAMES][INPUT_TYPE_COUNT];
static const unsigned short input_values[INPUT_TYPE_COUNT] = {
	PK_F1,
	PK_F2,
	PK_F3,
	PK_F4,
	PK_F5,
	PK_F6,
	PK_ENTER,
	PK_UP,
	PK_DOWN,
	PK_LEFT,
	PK_RIGHT,
	PK_ESC
};
static int input_index = 0;
static int previous_index = INPUT_RECORD_FRAMES - 1;

// 入力情報を記録
void recordInputs() {
	previous_index = input_index;
	input_index = (input_index + 1) % INPUT_RECORD_FRAMES;

	for (int i = 0; i < INPUT_TYPE_COUNT; i++) {
		inputs[input_index][i] = inport(input_values[i]);
	}
}

bool isInputHold(INPUT_TYPE type) {
	return inputs[input_index][type];
}

bool isInputDown(INPUT_TYPE type) {
	return inputs[input_index][type] && !inputs[previous_index][type];
}

bool isInputUp(INPUT_TYPE type) {
	return !inputs[input_index][type] && inputs[previous_index][type];
}