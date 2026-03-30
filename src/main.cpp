#include "device.h"

int main() {
	Device device;
	device.debugPrint();
	device.read(Request(ActionType::READ));
	device.debugPrint();
	device.update();
	device.debugPrint();
	device.write(Request(ActionType::WRITE));
	device.debugPrint();
	for (unsigned int i = 0; i < 100; i++) {
		device.update();
		device.debugPrint();
	}
}
