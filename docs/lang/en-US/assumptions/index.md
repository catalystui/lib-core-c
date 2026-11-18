# Assumptions

Core provides foundational computing primitives. Its functions follow a common set of assumptions concerning the validity of an invocation and the outcome of an operation. These assumptions apply equally to public functions, C++ wrappers, and internal functions.

## Contents

- [Validation](#validation)
  - [Contract Validation](#contract-validation)
  - [Operational Validation](#operational-validation)
- [Results](#results)
  - [Determining the Return Type](#determining-the-return-type)
  - [Reporting Boundaries](#reporting-boundaries)

## Validation

**A function performs validation if, and only if, that validation is necessary to determine or safely produce the outcome of its operation when all caller preconditions are satisfied.**

Validation is divided into two categories: contract validation and operational validation. The distinction is whether a condition is a prerequisite for invoking a function correctly or a condition the function must evaluate to perform its intended operation.

### Contract Validation

Contract validation establishes that the preconditions of a function are satisfied. It is the responsibility of the caller.

These preconditions include, where applicable, valid readable and writable memory, accessible extents, required termination, initialized objects, live and correctly typed handles, object lifetime, alignment, non-overlap, synchronization, valid indices for direct indexed access, and nonzero configured element sizes.

Core assumes that these preconditions have been satisfied. It does not repeat contract validation or report Results for violations of the calling contract. A violation is outside the guarantees of the function.

For example, copying a specified number of bytes requires the caller to provide valid source and destination regions of sufficient size. The copy operation does not independently establish whether those regions are valid.

This design allows a precondition established by a caller to remain established throughout subsequent operations. It avoids redundant checks and reporting, and allows foundational primitives to be composed without repeatedly paying the cost of validating the same conditions. Internal invariants are established during creation and maintained through mutation rather than repeatedly inspected to detect programmer misuse.

### Operational Validation

Operational validation determines whether the intended operation can succeed, even when all caller preconditions have been satisfied. It is the responsibility of the function.

Examples include malformed text encountered during decoding, unrepresentable codepoints, insufficient supplied capacity, overflow of a requested extent, allocation failures, native I/O failures, and missing values when the requested retrieval cannot produce a value.

For example, a text conversion may receive valid source and destination pointers with accessible storage but encounter malformed encoded data or discover that the supplied destination capacity is insufficient. These are operational conditions, not violations of the calling contract.

Operational validation is performed as part of the operation, rather than as a separate preliminary validation pass. Necessary checks take place before unsafe access or irreversible changes. A function must not read beyond the accessible extent established by its contract.

This design allows Core to report conditions that genuinely affect the outcome of an operation without requiring the caller to perform the operation in advance to determine whether it will succeed. Operational failures must not be reclassified as caller preconditions merely to avoid validation or reporting.

## Results

**A function returns a Result if, and only if, its intended operation requires distinguishing a meaningful warning, error, or fatal outcome from ordinary successful behavior when all caller preconditions are satisfied.**

A Result describes the outcome of an operation. It is not a mechanism for acknowledging that a function was invoked correctly, nor is it required simply because a function produces output.

### Determining the Return Type

First, establish the natural contract of the function: its purpose, the preconditions imposed on the caller, its ordinary outputs, and the outcomes that must be distinguished. Assume that all caller preconditions have been satisfied.

Apply the following questions in order, stopping at the first applicable answer.

1. **Can the operation produce a warning, error, or fatal outcome that its contract requires the caller to distinguish?**

   If yes, return `CATALYST_RESULT` (`RESULT` in C++). Return any additional data through output arguments. Every exit from the function returns the Result by value, including successful outcomes.

   For example, memory allocation may fail despite a valid invocation, and UTF-8 decoding may encounter malformed input. Both operations require a Result.

2. **Otherwise, does the function produce a single ordinary value?**

   If yes, return that value directly using its corresponding Catalyst type. This includes computations, comparisons, predicates, accessors, and conversions that are total over their documented input domain.

   For example, a byte comparison returns an ordering value, and a collection membership predicate returns a Boolean value.

3. **Otherwise, does the function perform work without a single ordinary return value?**

   Return `void`. Any necessary outputs are written through arguments, or the function mutates existing storage. Multiple output values alone do not justify a Result.

   For example, copying bytes into valid destination storage and retrieving an array element at a valid index into an output buffer do not require a Result.

| Function | Return Type | Reason |
| --- | --- | --- |
| `catmm_alloc` | `CATALYST_RESULT` | Allocation can fail. |
| `catte_utf8_decode` | `CATALYST_RESULT` | Encoded data can be malformed. |
| `catrl_map_get` | `CATALYST_RESULT` | The requested key may be absent, preventing retrieval. |
| `catrl_map_contains` | `CATALYST_BOOL` | Presence and absence are both ordinary Boolean outcomes. |
| `catmm_compare` | `CATALYST_SBYTE` | The comparison produces an ordinary ordering value. |
| `catmm_copy` | `void` | Copying over valid storage has no reportable outcome or ordinary return value. |
| `catrl_array_get` | `void` | A valid index is a caller precondition; the element is copied into an output buffer. |

Ordinary values such as zero, false, equality, an empty collection, and absence do not inherently constitute failures. A no-op alone does not require reporting. For a function that already requires a Result, a no-op follows its documented result contract.

The distinction depends on the purpose of the operation. A membership query succeeds by determining whether a value exists, regardless of the answer. A retrieval operation that cannot produce its requested value may instead require reporting its absence as an error.

### Reporting Boundaries

Operational validation and reporting belong at the outermost function boundary responsible for the operation's observable contract and failure handling. Once the necessary conditions have been established, subsequent operations can use non-reporting primitives.

An internal function that can still produce a warning, error, or fatal outcome that its caller must distinguish also returns a Result. Required diagnostics are not suppressed because a function is small, frequently invoked, or performance-sensitive.

Reporting functions return the four-byte Result directly and document the validity of any output arguments on each outcome. Non-reporting functions do not construct or propagate Results.

The caller is responsible for the validity of the invocation. The function is responsible for the outcome of the operation.
