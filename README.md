# CXL-Memory-Compression-Simulator

## Structure:

### Block (maybe needs better name)
(SRAM Cache, DRAM meta table)\
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
This is where the impementation (memory reads, data compression) of the block is done 
