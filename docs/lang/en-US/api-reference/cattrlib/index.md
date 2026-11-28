# CATTRLIB

Provides Process, Thread, and Dispatcher operations for execution and work selection.

**C Header:** `cattrlib.h`  
**C++ Header:** `cattrlib.hpp`  
**Namespace:** `catalyst`  
**Library:** `cattr`  
**Dependencies:** [CATALYST](../catalyst/index.md), [CATMMLIB](../catmmlib/index.md), [CATRLLIB](../catrllib/index.md)  
**Specification:** [FRELSPEC Revision 2.1](https://catalystui.org/specifications/languages/frelspec/)

## Contents

- [Usage Notes](#usage-notes)
- [Summary](#summary)
- [Macros](#macros)
- [Types](#types)
- [Functions](#functions)

## Usage Notes

CATTRLIB implements Process, Thread and Dispatcher relationships from FRELSPEC. Shared execution bookkeeping uses Catalyst memory and collection operations; native execution and synchronization use POSIX on Linux and macOS or Win32 on Windows. CMake builds CATTRLIB for these targets. Other targets, including 8-bit systems without an implemented backend, do not currently provide CATTRLIB execution.

Initialize unused storage only. Do not shallow-copy owning records, mutate their fields directly or separately release their allocations. Keep Process records live while Threads borrow them, and Thread records live while Dispatchers borrow them. Keep executable and argument storage valid through their documented lifetimes. Serialize lifecycle changes against external uses. Submission, counting, waiting, polling and closing synchronize internally on successfully initialized dispatchers.

A dispatcher does not automatically start, join or service its thread. The associated thread calls wait and poll as needed. The caller closes admission before service ends if other callers may still submit or wait. Close does not establish quiescence; all access must cease before free. Entries and callbacks must return normally. Native cancellation, forced termination and cyclic awaited dependencies are outside the contract.

Pending invocations are distinct even when executable references and arguments match. Invocation records are reserved in a Pool. FIFO membership uses a Queue only; custom selection uses a Set only. The public count operation works in both modes. A custom rule receives live references while holding the monitor and must not modify pending storage or reenter the dispatcher.

Native failures map to Catalyst status codes; native error numbers are not Result detail. Allocation and collection failures retain applicable status and detail at the public boundary. Result tables show the mapped outcomes; availability of native failures depends on backend and underlying operation.

For Core-wide requirements, see [Assumptions](../../assumptions/index.md).

## Summary

| Type | Meaning |
| --- | --- |
| [CATTRLIB_POINTERX](#cattrlib_pointerx) | Reference to executable instructions. |
| [CATTRLIB_PROCESS](#cattrlib_process) | Bounded flow of execution. |
| [CATTRLIB_THREAD](#cattrlib_thread) | Path of execution within a process. |
| [CATTRLIB_DISPATCHER](#cattrlib_dispatcher) | Mechanism selecting work for a thread. |

| Macro | Meaning |
| --- | --- |
| [CATTRLIB_API](#cattrlib_api) | Native symbol export/import annotation. |

| Function | Behavior |
| --- | --- |
| [cattr_process_current](#cattr_process_current) | Capture current process identity. |
| [cattr_process_equals](#cattr_process_equals) | Compare process identities. |
| [cattr_process_free](#cattr_process_free) | Release process identity storage. |
| [cattr_thread_create](#cattr_thread_create) | Establish an unstarted thread. |
| [cattr_thread_current](#cattr_thread_current) | Capture current thread identity. |
| [cattr_thread_start](#cattr_thread_start) | Start a created thread. |
| [cattr_thread_wait](#cattr_thread_wait) | Join a started thread. |
| [cattr_thread_equals](#cattr_thread_equals) | Compare thread identities. |
| [cattr_thread_free](#cattr_thread_free) | Release thread state. |
| [cattr_dispatcher_create](#cattr_dispatcher_create) | Establish bounded dispatch storage. |
| [cattr_dispatcher_submit](#cattr_dispatcher_submit) | Submit an independent invocation. |
| [cattr_dispatcher_inspect](#cattr_dispatcher_inspect) | Inspect a live rule candidate. |
| [cattr_dispatcher_count](#cattr_dispatcher_count) | Count pending invocations. |
| [cattr_dispatcher_wait](#cattr_dispatcher_wait) | Wait for work availability. |
| [cattr_dispatcher_poll](#cattr_dispatcher_poll) | Select and execute one invocation. |
| [cattr_dispatcher_close](#cattr_dispatcher_close) | Close admission and cancel pending work. |
| [cattr_dispatcher_free](#cattr_dispatcher_free) | Release closed dispatcher resources. |

## Macros

### CATTRLIB_API

Export/import annotation for CATTRLIB's C ABI. On Windows/Cygwin, `CATTRLIB_EXPORTS` selects `__declspec(dllexport)` and `CATTRLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the macro is empty. The `cattr` library depends on `catrl` and transitively `catmm`. The CMake `cattrlib_static` and `cattrlib_shared` targets propagate corresponding Catalyst dependencies and supply native threading linkage where required.

## Types

### CATTRLIB_POINTERX

Opaque function pointer, not a data pointer. Convert an actual `void (CATMMLIB_POINTERW)` function to this type for entry or submission; CATTRLIB converts it back before invocation. Do not call the converted reference directly.

```c
typedef void (*CATTRLIB_POINTERX)(void);
```

**C++ equivalent:** `catalyst::POINTERX`.

### CATTRLIB_PROCESS

A bounded flow of execution which transforms input into output. The current backend captures the native process identity; it does not create, enumerate or terminate processes.

```c
typedef struct CATTRLIB_PROCESS {
    CATMMLIB_POINTERW state;
} CATTRLIB_PROCESS;
```

**Fields:**

- `state`: Owned opaque process identity storage.

**C++ equivalent:** `catalyst::PROCESS`.

### CATTRLIB_THREAD

A path of execution contained within a process. Created threads own native execution resources; captures own only identity storage.

```c
typedef struct CATTRLIB_THREAD {
    CATTRLIB_PROCESS* process;
    CATTRLIB_POINTERX entry;
    CATMMLIB_POINTERW arguments;
    CATMMLIB_POINTERW state;
} CATTRLIB_THREAD;
```

**Fields:**

- `process`: Borrowed Process, kept live through cleanup.  
- `entry`: Executable reference; null for a capture.  
- `arguments`: Borrowed pointer passed to entry; null for a capture.  
- `state`: Owned opaque native and lifecycle state.

**C++ equivalent:** `catalyst::THREAD`.

### CATTRLIB_DISPATCHER

Selects pending invocations according to a dispatch rule and executes them on one associated thread. Owns a bounded invocation Pool and exactly one allocated pending collection.

```c
typedef struct CATTRLIB_DISPATCHER {
    CATTRLIB_THREAD* thread;
    CATRLLIB_SET work;
    CATMMLIB_POINTER (*rule)(const CATRLLIB_SET*);
    CATMMLIB_POINTERW state;
} CATTRLIB_DISPATCHER;
```

**Fields:**

- `thread`: Borrowed associated Thread, kept live through cleanup.  
- `work`: Owned Set of opaque invocation references for a custom rule; empty and unallocated in FIFO mode.  
- `rule`: Borrowed selection callback, or null for FIFO.  
- `state`: Owned opaque monitor, Pool and FIFO bookkeeping.

**C++ equivalent:** `catalyst::DISPATCHER`.

## Functions

### cattr_process_current

```c
CATALYST_RESULT cattr_process_current(CATTRLIB_PROCESS* process);
```

Capture the current native process into unused storage. The record owns its identity storage, not the native process. Failure leaves state null.

**Parameters:**

- `process`: Writable unused Process storage.

**C++ equivalent:** `catalyst::process_current`.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010100` | `SUCCESS` | Capture identity | — |
| `0x84010100` | `ERROR_ACCESS_DENIED`: Allocation denied | Allocate identity record | — |
| `0x86010100` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Allocate identity record | — |
| `0x90010100` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Allocate identity record | — |
| `0x91010100` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Allocate identity record | — |

### cattr_process_equals

```c
CATALYST_BOOL cattr_process_equals(const CATTRLIB_PROCESS* process1, const CATTRLIB_PROCESS* process2);
```

Determine whether two live Process records identify the same native process.

**Parameters:**

- `process1`: Valid captured Process.  
- `process2`: Valid captured Process.

**C++ equivalent:** `catalyst::process_equals`.

**Returns:** `CATALYST_TRUE` if identities match; otherwise `CATALYST_FALSE`.

### cattr_process_free

```c
CATALYST_RESULT cattr_process_free(CATTRLIB_PROCESS* process);
```

Release identity storage without terminating the native process. An already released record or failed capture with null state is a no-op. Failure retains ownership for retry.

**Parameters:**

- `process`: Initialized Process with no thread borrowers, including failed capture or released state.

**C++ equivalent:** `catalyst::process_free`.

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01030100` | `SUCCESS_NOOP`: No owned state | Keep empty record | — |
| `0x00030200` | `SUCCESS` | Release identity | — |
| `0x82030200` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Release identity | — |
| `0x84030200` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Release identity | — |

### cattr_thread_create

```c
CATALYST_RESULT cattr_thread_create(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process, CATTRLIB_POINTERX entry, CATMMLIB_POINTERW arguments);
```

Prepare a thread in the current process without starting entry. Failure permits only free.

**Parameters:**

- `thread`: Writable unused Thread storage.  
- `process`: Live Process identifying current native process; borrowed through cleanup.  
- `entry`: Non-null executable reference to `void (CATMMLIB_POINTERW)` returning normally.  
- `arguments`: Borrowed pointer passed unchanged; may be null if accepted by entry.

**C++ equivalent:** `catalyst::thread_create`.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82040100` | `ERROR_INVALID_STATE`: Process does not identify current execution | Validate process | — |
| `0x84040200` | `ERROR_ACCESS_DENIED`: Allocation denied | Allocate thread state | — |
| `0x86040200` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Allocate thread state | — |
| `0x90040200` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Allocate thread state | — |
| `0x91040200` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Allocate thread state | — |
| `0x00040300` | `SUCCESS` | Establish thread | — |

**See Also:** [cattr_thread_start](#cattr_thread_start), [cattr_thread_free](#cattr_thread_free)

### cattr_thread_current

```c
CATALYST_RESULT cattr_thread_current(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process);
```

Capture the current native thread without starting execution or owning its native lifetime. Entry and arguments are null. Failure permits only free. The captured native identity must remain live through its last use other than free.

**Parameters:**

- `thread`: Writable unused Thread storage.  
- `process`: Live Process identifying current native process; borrowed through cleanup.

**C++ equivalent:** `catalyst::thread_current`.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82050100` | `ERROR_INVALID_STATE`: Process does not identify current execution | Validate process | — |
| `0x84050300` | `ERROR_ACCESS_DENIED`: Allocation denied | Allocate capture state | — |
| `0x86050300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Allocate capture state | — |
| `0x90050300` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Allocate capture state | — |
| `0x91050300` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Allocate capture state | — |
| `0x00050400` | `SUCCESS` | Capture identity | — |

### cattr_thread_start

```c
CATALYST_RESULT cattr_thread_start(CATTRLIB_THREAD* thread);
```

Start an unstarted created Thread. Native identity is published before entry can run; entry may run before this call returns. Failed startup permits free only. Serialize startup against external uses; the entry may use its published Thread.

**Parameters:**

- `thread`: Initialized Thread with valid state.

**C++ equivalent:** `catalyst::thread_start`.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82060100` | `ERROR_INVALID_STATE`: Not created | Validate lifecycle | — |
| `0x82060200` | `ERROR_INVALID_STATE`: Native state or operation failure | Start native thread | — |
| `0x84060200` | `ERROR_ACCESS_DENIED`: Native operation denied | Start native thread | — |
| `0x86060200` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Start native thread | — |
| `0x87060200` | `ERROR_TIMEOUT`: Native timeout | Start native thread | — |
| `0x89060200` | `ERROR_INTERRUPTED`: Native operation interrupted | Start native thread | — |
| `0x91060200` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Start native thread | — |
| `0x00060300` | `SUCCESS` | Publish execution | — |

### cattr_thread_wait

```c
CATALYST_RESULT cattr_thread_wait(CATTRLIB_THREAD* thread);
```

Wait indefinitely from another thread until execution completes. Repeat join is a no-op. Native wait failure leaves the started lifecycle unchanged. Serialize joining against external thread and dispatcher operations; the executing entry may continue until joined.

**Parameters:**

- `thread`: Initialized Thread with valid state.

**C++ equivalent:** `catalyst::thread_wait`.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01070100` | `SUCCESS_NOOP`: Already joined | Keep joined state | — |
| `0x82070200` | `ERROR_INVALID_STATE`: Not started or self-join | Validate lifecycle | — |
| `0x82070300` | `ERROR_INVALID_STATE`: Native state or operation failure | Join native thread | — |
| `0x84070300` | `ERROR_ACCESS_DENIED`: Native operation denied | Join native thread | — |
| `0x86070300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Join native thread | — |
| `0x87070300` | `ERROR_TIMEOUT`: Native timeout | Join native thread | — |
| `0x89070300` | `ERROR_INTERRUPTED`: Native operation interrupted | Join native thread | — |
| `0x91070300` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Join native thread | — |
| `0x00070400` | `SUCCESS` | Record completion | — |

### cattr_thread_equals

```c
CATALYST_BOOL cattr_thread_equals(const CATTRLIB_THREAD* thread1, const CATTRLIB_THREAD* thread2);
```

Identical state records compare equal. Otherwise processes must match and both records must be started or captured. Unstarted and joined records compare only to themselves. Distinct captures require live native identities. Comparison of exited but unjoined threads may differ by backend; this is not a portable liveness test.

**Parameters:**

- `thread1`: Initialized Thread with valid state.  
- `thread2`: Initialized Thread with valid state.

**C++ equivalent:** `catalyst::thread_equals`.

**Returns:** `CATALYST_TRUE` for equal identities; otherwise `CATALYST_FALSE`.

### cattr_thread_free

```c
CATALYST_RESULT cattr_thread_free(CATTRLIB_THREAD* thread);
```

Release a captured, unstarted, joined or failed thread. Started threads must be joined even if entry already ended. Captures are not terminated or joined. Successful release of owned state clears all fields; null state is a no-op. Failure retains remaining resources for retry.

**Parameters:**

- `thread`: Initialized Thread, including failed creation/start or released state, with no active users or borrowing dispatchers.

**C++ equivalent:** `catalyst::thread_free`.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01090100` | `SUCCESS_NOOP`: No owned state | Keep empty record | — |
| `0x82090200` | `ERROR_INVALID_STATE`: Started thread not joined | Validate lifecycle | — |
| `0x82090300` | `ERROR_INVALID_STATE`: Native state or operation failure | Release thread state | — |
| `0x84090300` | `ERROR_ACCESS_DENIED`: Native operation denied | Release thread state | — |
| `0x86090300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Release thread state | — |
| `0x87090300` | `ERROR_TIMEOUT`: Native timeout | Release thread state | — |
| `0x89090300` | `ERROR_INTERRUPTED`: Native operation interrupted | Release thread state | — |
| `0x91090300` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Release thread state | — |
| `0x00090500` | `SUCCESS` | Clear ownership | — |

### cattr_dispatcher_create

```c
CATALYST_RESULT cattr_dispatcher_create(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_THREAD* thread, CATMMLIB_POINTER (*rule)(const CATRLLIB_SET*), CATALYST_NUINT capacity);
```

Establish a dispatcher without starting its thread. The Pool reserves `capacity` records; pending, running and completed-but-not-reclaimed awaited records consume capacity. FIFO allocates only a Queue; custom selection allocates only a Set. Submission requires no further allocation. Failure permits only free, including partially initialized storage.

A custom rule receives a nonempty Set of opaque invocation references under the monitor on the associated thread. Only the first `cardinality` slots are live; use `size` as byte stride and copy the stored `CATMMLIB_POINTERW` bytes into a pointer variable. Return one member reference, not the address of its storage slot. Inspect candidates with `cattr_dispatcher_inspect`. Do not modify the collection, retain references beyond the rule, reenter or block on the dispatcher.

**Parameters:**

- `dispatcher`: Writable unused Dispatcher storage.  
- `thread`: Live created or captured associated Thread; borrowed until free.  
- `rule`: Selection callback, or null for FIFO.  
- `capacity`: Nonzero maximum outstanding invocation count; internal byte extents must fit allocation limits.

**C++ equivalent:** `catalyst::dispatcher_create`.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x810A0103` | `ERROR_INVALID_ARGUMENT`: Zero capacity | Validate capacity | Argument index 3 (`capacity`) |
| `0x820A0300` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Reserve invocation Pool | — |
| `0x840A0300` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Reserve invocation Pool | — |
| `0x860A0300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Reserve invocation Pool | — |
| `0x900A0300` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Reserve invocation Pool | — |
| `0x910A0300` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Reserve invocation Pool | — |
| `0x820A0384` | `ERROR_INVALID_STATE`: Dual cleanup failure | Reserve invocation Pool | Other failure is `ERROR_ACCESS_DENIED` |
| `0x840A0382` | `ERROR_ACCESS_DENIED`: Dual cleanup failure | Reserve invocation Pool | Other failure is `ERROR_INVALID_STATE` |
| `0x820A0400` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Reserve custom-rule Set | — |
| `0x840A0400` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Reserve custom-rule Set | — |
| `0x860A0400` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Reserve custom-rule Set | — |
| `0x900A0400` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Reserve custom-rule Set | — |
| `0x910A0400` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Reserve custom-rule Set | — |
| `0x820A0484` | `ERROR_INVALID_STATE`: Dual cleanup failure | Reserve custom-rule Set | Other failure is `ERROR_ACCESS_DENIED` |
| `0x840A0482` | `ERROR_ACCESS_DENIED`: Dual cleanup failure | Reserve custom-rule Set | Other failure is `ERROR_INVALID_STATE` |
| `0x820A0500` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Reserve FIFO Queue | — |
| `0x840A0500` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Reserve FIFO Queue | — |
| `0x860A0500` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native allocation configuration unsupported | Reserve FIFO Queue | — |
| `0x900A0500` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Reserve FIFO Queue | — |
| `0x910A0500` | `ERROR_ALLOCATION_FAILED`: Allocation failed | Reserve FIFO Queue | — |
| `0x820A0584` | `ERROR_INVALID_STATE`: Dual cleanup failure | Reserve FIFO Queue | Other failure is `ERROR_ACCESS_DENIED` |
| `0x840A0582` | `ERROR_ACCESS_DENIED`: Dual cleanup failure | Reserve FIFO Queue | Other failure is `ERROR_INVALID_STATE` |
| `0x820A0600` | `ERROR_INVALID_STATE`: Native state or operation failure | Create monitor | — |
| `0x840A0600` | `ERROR_ACCESS_DENIED`: Native operation denied | Create monitor | — |
| `0x860A0600` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Create monitor | — |
| `0x870A0600` | `ERROR_TIMEOUT`: Native timeout | Create monitor | — |
| `0x890A0600` | `ERROR_INTERRUPTED`: Native operation interrupted | Create monitor | — |
| `0x910A0600` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Create monitor | — |
| `0x900A0600` | `ERROR_BUFFER_OVERFLOW`: Allocation extent unrepresentable | Create monitor | — |
| `0x000A0700` | `SUCCESS` | Open admission | — |

**See Also:** [CATRLLIB_POOL](../catrllib/index.md#catrllib_pool), [CATRLLIB_QUEUE](../catrllib/index.md#catrllib_queue), [CATRLLIB_SET](../catrllib/index.md#catrllib_set), [cattr_dispatcher_inspect](#cattr_dispatcher_inspect)

### cattr_dispatcher_submit

```c
CATALYST_RESULT cattr_dispatcher_submit(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_POINTERX operation, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout);
```

Submit one independent invocation; identical operation/argument pairs remain separate. Asynchronous success means admission, not execution. Awaited success means normal completion. An awaited invocation still pending when the wait fails is removed; once running, the call waits for normal completion even if the original timeout expires or a native wait fails. Close cancels pending work, not running callbacks.

Timeout covers admission and the pending interval, excluding monitor acquisition and execution after an awaited callback starts. Readiness after reacquiring the monitor wins a race with expiry. On the associated thread, awaited submission is invalid and asynchronous admission is nonblocking. Avoid cyclic awaited dependencies. Preserve asynchronously borrowed arguments through execution or cancellation; an awaited return ends the borrow. A native failure after execution starts does not imply that the callback did not execute.

**Parameters:**

- `dispatcher`: Successfully created Dispatcher, not being freed.  
- `operation`: Non-null executable reference to `void (CATMMLIB_POINTERW)` returning normally.  
- `arguments`: Borrowed pointer passed unchanged; may be null if accepted by callback.  
- `wait`: `CATALYST_TRUE` awaits completion; `CATALYST_FALSE` returns after admission.  
- `timeout`: Milliseconds; zero polls, `CATALYST_NUINT_MAX` waits indefinitely.

**C++ equivalent:** `catalyst::dispatcher_submit`.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x820B0100` | `ERROR_INVALID_STATE`: Dispatcher already closed | Validate admission | — |
| `0x820B0200` | `ERROR_INVALID_STATE`: Awaited submission from associated thread | Validate dependency | — |
| `0x820B0300` | `ERROR_INVALID_STATE`: Native state or operation failure | Wait for admission | — |
| `0x840B0300` | `ERROR_ACCESS_DENIED`: Native operation denied | Wait for admission | — |
| `0x860B0300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Wait for admission | — |
| `0x870B0300` | `ERROR_TIMEOUT`: Native timeout | Wait for admission | — |
| `0x890B0300` | `ERROR_INTERRUPTED`: Native operation interrupted | Wait for admission | — |
| `0x910B0300` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Wait for admission | — |
| `0x900B0300` | `ERROR_BUFFER_OVERFLOW`: Native deadline extent unrepresentable | Wait for admission | — |
| `0x890B0400` | `ERROR_INTERRUPTED`: Closed before admission | Interrupt admission | — |
| `0x000B0500` | `SUCCESS` | Admit asynchronous work | — |
| `0x820B0600` | `ERROR_INVALID_STATE`: Native state or operation failure | Remove pending invocation | — |
| `0x840B0600` | `ERROR_ACCESS_DENIED`: Native operation denied | Remove pending invocation | — |
| `0x860B0600` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Remove pending invocation | — |
| `0x870B0600` | `ERROR_TIMEOUT`: Native timeout | Remove pending invocation | — |
| `0x890B0600` | `ERROR_INTERRUPTED`: Native operation interrupted | Remove pending invocation | — |
| `0x910B0600` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Remove pending invocation | — |
| `0x900B0600` | `ERROR_BUFFER_OVERFLOW`: Native deadline extent unrepresentable | Remove pending invocation | — |
| `0x820B0700` | `ERROR_INVALID_STATE`: Native state or operation failure | Finish awaited execution after native failure | — |
| `0x840B0700` | `ERROR_ACCESS_DENIED`: Native operation denied | Finish awaited execution after native failure | — |
| `0x860B0700` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Finish awaited execution after native failure | — |
| `0x890B0700` | `ERROR_INTERRUPTED`: Native operation interrupted | Finish awaited execution after native failure | — |
| `0x910B0700` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Finish awaited execution after native failure | — |
| `0x890B0800` | `ERROR_INTERRUPTED`: Pending invocation cancelled by close | Observe cancellation | — |
| `0x000B0900` | `SUCCESS` | Complete awaited execution | — |
| `0x820B0A00` | `ERROR_INVALID_STATE`: Thread joined or failed startup | Validate associated thread | — |

**See Also:** [cattr_dispatcher_poll](#cattr_dispatcher_poll), [cattr_dispatcher_close](#cattr_dispatcher_close)

### cattr_dispatcher_inspect

```c
void cattr_dispatcher_inspect(CATMMLIB_POINTER submission, CATTRLIB_POINTERX* operation, CATMMLIB_POINTERW* arguments);
```

Retrieve the executable reference and borrowed argument pointer of an opaque invocation during a custom rule. The rule already holds the monitor; this operation does not acquire it.

**Parameters:**

- `submission`: Live member reference from the rule's Set, not its storage address.  
- `operation`: Writable output for executable reference.  
- `arguments`: Writable output for borrowed argument pointer; may receive null.

**C++ equivalent:** `catalyst::dispatcher_inspect`.

### cattr_dispatcher_count

```c
CATALYST_NUINT cattr_dispatcher_count(const CATTRLIB_DISPATCHER* dispatcher);
```

Retrieve a synchronized snapshot of pending membership, excluding running, completed and cancelled invocations. Works in both FIFO and custom modes; the public work Set does not contain FIFO membership.

**Parameters:**

- `dispatcher`: Successfully created Dispatcher, including closed state, not being freed.

**C++ equivalent:** `catalyst::dispatcher_count`.

**Returns:** Number of pending invocations, including zero.

### cattr_dispatcher_wait

```c
CATALYST_RESULT cattr_dispatcher_wait(CATTRLIB_DISPATCHER* dispatcher, CATALYST_NUINT timeout);
```

Wait until pending work exists or admission closes. Only the associated thread may wait. Success indicates availability, not selection or execution. Readiness after monitor reacquisition wins a race with expiry; monitor acquisition is outside the timeout. Do not wait on work whose availability requires the waiting callback to return.

**Parameters:**

- `dispatcher`: Successfully created Dispatcher, not being freed.  
- `timeout`: Milliseconds; zero polls, `CATALYST_NUINT_MAX` waits indefinitely.

**C++ equivalent:** `catalyst::dispatcher_wait`.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x820D0100` | `ERROR_INVALID_STATE`: Caller not associated thread | Validate execution identity | — |
| `0x820D0200` | `ERROR_INVALID_STATE`: Native state or operation failure | Wait for availability | — |
| `0x840D0200` | `ERROR_ACCESS_DENIED`: Native operation denied | Wait for availability | — |
| `0x860D0200` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Wait for availability | — |
| `0x870D0200` | `ERROR_TIMEOUT`: Native timeout | Wait for availability | — |
| `0x890D0200` | `ERROR_INTERRUPTED`: Native operation interrupted | Wait for availability | — |
| `0x910D0200` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Wait for availability | — |
| `0x900D0200` | `ERROR_BUFFER_OVERFLOW`: Native deadline extent unrepresentable | Wait for availability | — |
| `0x890D0300` | `ERROR_INTERRUPTED`: Dispatcher closed | Observe closure | — |
| `0x000D0400` | `SUCCESS` | Report availability | — |

### cattr_dispatcher_poll

```c
CATALYST_RESULT cattr_dispatcher_poll(CATTRLIB_DISPATCHER* dispatcher);
```

On the associated thread, select and remove one pending invocation under the monitor, then invoke with the monitor released. Complete lifecycle bookkeeping under the monitor before returning. Empty open dispatcher is a successful no-op. Invalid rule selection preserves pending work. Recursive polling is invalid; callbacks must return normally.

**Parameters:**

- `dispatcher`: Successfully created Dispatcher, not being freed.

**C++ equivalent:** `catalyst::dispatcher_poll`.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x820E0100` | `ERROR_INVALID_STATE`: Caller not associated thread | Validate execution identity | — |
| `0x890E0200` | `ERROR_INTERRUPTED`: Dispatcher closed | Observe closure | — |
| `0x820E0300` | `ERROR_INVALID_STATE`: Recursive polling | Validate polling state | — |
| `0x010E0400` | `SUCCESS_NOOP`: No pending work | Keep empty collection | — |
| `0x820E0500` | `ERROR_INVALID_STATE`: Rule did not select a member | Validate selected member | — |
| `0x000E0600` | `SUCCESS` | Execute selected invocation | — |

### cattr_dispatcher_close

```c
CATALYST_RESULT cattr_dispatcher_close(CATTRLIB_DISPATCHER* dispatcher);
```

Close admission, cancel pending work and wake waiters. Running callbacks continue; awaited pending callers observe cancellation and reclaim their records. Repeat close is a no-op. Close does not join the thread or establish quiescence.

**Parameters:**

- `dispatcher`: Successfully created Dispatcher, not being freed.

**C++ equivalent:** `catalyst::dispatcher_close`.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x010F0100` | `SUCCESS_NOOP`: Already closed | Keep closed state | — |
| `0x000F0200` | `SUCCESS` | Close and notify | — |

### cattr_dispatcher_free

```c
CATALYST_RESULT cattr_dispatcher_free(CATTRLIB_DISPATCHER* dispatcher);
```

Release the active pending collection, Pool and monitor. The caller establishes quiescence: close admission, allow all submissions, waits, polls and other accesses to return, then free. Failed creation may be freed directly. Failed cleanup permits only retrying free; previously released components remain released. Successful release of owned state clears thread, rule and state; null state is a no-op.

**Parameters:**

- `dispatcher`: Initialized closed Dispatcher with no active users, or failed creation/cleanup or released state.

**C++ equivalent:** `catalyst::dispatcher_free`.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01100100` | `SUCCESS_NOOP`: No owned state | Keep empty record | — |
| `0x82100200` | `ERROR_INVALID_STATE`: Dispatcher not closed | Validate closure | — |
| `0x82100300` | `ERROR_INVALID_STATE`: Native state or operation failure | Release monitor | — |
| `0x84100300` | `ERROR_ACCESS_DENIED`: Native operation denied | Release monitor | — |
| `0x86100300` | `ERROR_SYSTEM_NOT_SUPPORTED`: Native operation unsupported | Release monitor | — |
| `0x87100300` | `ERROR_TIMEOUT`: Native timeout | Release monitor | — |
| `0x89100300` | `ERROR_INTERRUPTED`: Native operation interrupted | Release monitor | — |
| `0x91100300` | `ERROR_ALLOCATION_FAILED`: Native resources unavailable | Release monitor | — |
| `0x82100400` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Release custom-rule Set | — |
| `0x84100400` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Release custom-rule Set | — |
| `0x82100500` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Release FIFO Queue | — |
| `0x84100500` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Release FIFO Queue | — |
| `0x82100600` | `ERROR_INVALID_STATE`: Release or pending cleanup failed | Release invocation Pool | — |
| `0x84100600` | `ERROR_ACCESS_DENIED`: Release or cleanup denied | Release invocation Pool | — |
| `0x00100800` | `SUCCESS` | Clear ownership | — |

**See Also:** [cattr_dispatcher_close](#cattr_dispatcher_close), [cattr_thread_wait](#cattr_thread_wait)
