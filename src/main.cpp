#include "device.h"

int main() {
	Device device;
	device.read(Request(ActionType::READ));
	device.update();
	device.write(Request(ActionType::WRITE));
	for (unsigned int i = 0; i < 1000; i++) {
		device.update();
	}
}
