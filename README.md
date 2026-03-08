# CXL-Memory-Compression-Simulator

## Structure:

### Block (maybe needs better name)
(SRAM Cache, DRAM meta table, ...)\
Blocks are parts of the system(device) that have a common interface to abstracted implementation.

Each block is made up of:
```
+--------------------+
|       Input        |
+--------------------+
|    Input Buffers   |
+--------------------+
|       Compute      |
+--------------------+
|   Output Routing   |
+--------------------+
```
(In addition to these functions there is an interface that is used to ask about the state of the block.)


- Input:\
The input is an interface that allows other blocks to give `Requests` to this block for it to process. This incudes an extra size one buffer for each input buffer used for (the thing where data can keep moving when buffers are full).

- Input Buffers:\
These are a set simple FIFO buffers that can store requests while others are being processed.

- Compute:\
This is where the impementation (memory reads, data compression) of the block is done. This pulls from the input buffers and outputs the request with the result of the work done.

- Output Routing:\
This takes a compute result and depending on the request it will create new requests that are passed to the inputs of other gates.

### Request
(SRAM cache read for device write, SRAM cache read for device read, ...)\
Requests are how the simulation knows why something is happening. A request is abstract data combinded with an identifier. The abstract data is the minumum amount of information about real data that would be passed around on the machine. The request identifier is used to tell why a request is happening and therefore can be used to decied what do with it after compute happens.

For example when a read request is passed to the input of the SRAM cache then the identifier would say if it was a device read, device write, or other.

The identifier would not effect anything that happens till after the compute step where the request and any result is given to output routing. Output routing will use the request identifier to choose how the request will be routed.

The idea is to seprate different tasks that the device will do. Each should have one or more unique identifiers so that if the routing for a single task needs to change then routing can be changed for some identifiers without having to think about other tasks.
