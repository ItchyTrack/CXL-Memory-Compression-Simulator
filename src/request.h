#ifndef request_h
#define request_h

#include <cstdio>
enum ActionType {
	READ,
	WRITE,
	CACHE_EVICT,
	COMPRESS,
};

struct Request {
	Request(ActionType actionType) : action(actionType) {}

	void printInfo() {
		printf("Request ");
		switch (action) {
		case READ: {
			printf("READ");
		} break;
		case WRITE: {
			printf("WRITE");
		} break;
		case CACHE_EVICT: {
			printf("CACHE_EVICT");
		} break;
		case COMPRESS: {
			printf("COMPRESS");
		} break;
		}
		printf("\n");
	}
	ActionType action;
};

#endif /* request_h */
