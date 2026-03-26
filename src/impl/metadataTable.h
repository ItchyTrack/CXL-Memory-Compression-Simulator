#ifndef metadataTable_h
#define metadataTable_h

#include "../block.h"

/*
 * The idea is that each components you create the template of Block for each component.
 */

class MetadataTableCompute;
class MetadataTableRouter;

// [read requests, write requests]
typedef Block<2, MetadataTableCompute, MetadataTableRouter> MetadataTable;

class MetadataTableRouter {
public:
	MetadataTableRouter(Device& device) : device(device) {}

	bool route(const Request& request);

private:
	Device& device;
};

class MetadataTableCompute {
public:
	MetadataTableCompute(MetadataTable& metadataTable) : metadataTable(metadataTable) { }

	void update() {
		// do reads before writes
		std::optional<Request> request = metadataTable.blockInput.getNextRequest(0);
		if (!request.has_value()) request = metadataTable.blockInput.getNextRequest(1);

		// if we found any requests then give it to the router
		if (request.has_value()) {
			metadataTable.outputRouter.route(request.value()); // we just assume it works
		}
	}
private:
	MetadataTable& metadataTable;
};

#endif /* metadataTable_h */
