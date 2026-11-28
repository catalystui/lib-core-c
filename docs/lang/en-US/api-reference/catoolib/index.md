# CATOOLIB

The Object-Oriented Library represents the composite relationships defined by FRELSPEC. Members contain addressable bytes, objects associate keys with members, and executable and accessor members are interpreted by their caller-defined contracts.

**C Header:** `catoolib.h`  
**C++ Header:** `catoolib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catoo`  
**Dependencies:** [CATALYST](../catalyst/index.md), [CATMMLIB](../catmmlib/index.md), [CATRLLIB](../catrllib/index.md)  
**Specification:** [FRELSPEC Revision 2.1](https://catalystui.org/specifications/languages/frelspec/)

## Usage Notes

Objects own their CATRLLIB map records and each nonzero-size member's separately allocated payload (CATMMLIB managed heap). A member copies bytes, not objects or resources referenced by those bytes. Initialize unused, disjoint storage, do not shallow-copy owning objects or members, and release resources through their corresponding free operations. An object can be reused after successful free. After failed free, retry free only. Synchronize accesses and lifecycle operations externally.

Keys have a single nonzero byte width per object and are compared as bytes. Expand capacity before adding; a duplicate is a no-op. The map's exposed fields are not an alternative mutation API. Reacquire borrowed members and fields after removal, clear, map expansion or shrinkage, class extension, or free attempts that may relocate or release their storage. Fields borrow the object's stored keys, not the caller's lookup buffers. Do not independently free borrowed members or directly change their ownership or size fields. Use `catoo_member_resize` on a live standalone or borrowed member to change its byte extent without changing its key. Payload addresses may change, so references into the old payload must be reacquired after successful resize. The member descriptor and resolved Field remain live.

A method is stored by copying the bytes of a native `CATOOLIB_METHOD` function pointer; a property is stored by copying a `CATOOLIB_PROPERTY`. Callers establish the exact callable types and callback contracts before invoking; no runtime type metadata is introduced. Callback results preserve status/detail under CATOOLIB context/operation codes, except interface validation remaps invalid-argument details and dispatch rejects out-of-range callback argument indices as invalid state. Receiver index zero corresponds to the public Field argument. A callback may invalidate a resolved field by mutating its object.

Structures have fixed keys by API contract, not compiler enforcement. Classes store default member bytes and create independent instances; references embedded in those bytes remain shared. Class extension copies disjoint layouts. Cloning and extension copy the key layout contiguously and publish each member only after its independent payload has been allocated and copied. Reserve anticipated total member slots through `catoo_object_expand(&class_->object, capacity)` before a series of extensions to avoid repeated map growth; extension preserves sufficient existing capacity. Conflicting keys are intentionally unsupported until a policy is approved. No nominal inheritance, class identity tracking, or automatic lifecycle callbacks are implied.

Interfaces require a subset of the object's keys and validate each corresponding member using the supplied rule. An empty interface accepts every object; a nonempty interface requires matching key widths and a callable validator. The rule must not modify either object and must write a Catalyst Boolean on SUCCESS. Validation visits requirements in their stored order, resolving matching layouts directly and searching by key otherwise. Non-SUCCESS results stop without writing the caller's validity output.

For Core-wide requirements, see [Assumptions](../../assumptions/index.md).

## Summary

| C type | C++ type | Description |
| --- | --- | --- |
| [CATOOLIB_MEMBER](#catoolib_member) | `MEMBER` | Owned payload and byte size. |
| [CATOOLIB_OBJECT](#catoolib_object) | `OBJECT` | Addressable member map. |
| [CATOOLIB_FIELD](#catoolib_field) | `FIELD` | Borrowed object, key and member. |
| [CATOOLIB_METHOD](#catoolib_method) | `METHOD` | Callable taking an object, arguments and output. |
| [CATOOLIB_PROPERTY](#catoolib_property) | `PROPERTY` | Getter and setter callbacks. |
| [CATOOLIB_STRUCTURE](#catoolib_structure) | `STRUCTURE` | Object with a fixed key set. |
| [CATOOLIB_CLASS](#catoolib_class) | `CLASS` | Object containing default member definitions. |
| [CATOOLIB_INTERFACE](#catoolib_interface) | `INTERFACE` | Object containing requirements and a validation callback. |

| Macro | Meaning |
| --- | --- |
| [CATOOLIB_API](#catoolib_api) | Native symbol export/import annotation. |

| Operation | Description |
| --- | --- |
| [`catoo_member_init`](#catoo_member_init) | Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member. |
| [`catoo_member_get`](#catoo_member_get) | Copy a live member's size bytes into sufficient writable output storage outside its descriptor; overlap with payload is supported. |
| [`catoo_member_set`](#catoo_member_set) | Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership. |
| [`catoo_member_resize`](#catoo_member_resize) | Resize a live member to a nonzero byte size, preserving the common prefix and zero-filling growth. Same size is a no-op; failure preserves the payload and size. Successful resize may invalidate payload pointers but not the member descriptor, its key, or its resolved Field. |
| [`catoo_member_free`](#catoo_member_free) | Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op. |
| [`catoo_object_init`](#catoo_object_init) | Initialize unused object storage without allocation; ksize is the nonzero byte width of every key. |
| [`catoo_object_add`](#catoo_object_add) | Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members. |
| [`catoo_object_remove`](#catoo_object_remove) | Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The key must be readable during lookup; it need not outlive the removed payload. |
| [`catoo_object_contains`](#catoo_object_contains) | Test a readable ksize-byte key against an initialized object. |
| [`catoo_object_get`](#catoo_object_get) | Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, map resize or free attempts that may invalidate the descriptor. |
| [`catoo_object_expand`](#catoo_object_expand) | Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated. |
| [`catoo_object_shrink`](#catoo_object_shrink) | Halve map capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed members and fields after map resize attempts. |
| [`catoo_object_clear`](#catoo_object_clear) | Release and remove all members, retaining map capacity. On failure, already removed members remain removed; remaining members are live. |
| [`catoo_object_free`](#catoo_object_free) | Release all members then map storage. Failure retains unreleased resources for retry with object_free; success leaves a reusable empty object. |
| [`catoo_field_init`](#catoo_field_init) | Resolve a borrowed field from an initialized object using a readable key. The field borrows the object's stored key and member; input key storage is not retained. Reacquire after invalidating map operations. Absence clears the field and returns FALSE. |
| [`catoo_field_get`](#catoo_field_get) | Copy a live resolved member into sufficient writable output storage outside its descriptor. |
| [`catoo_field_set`](#catoo_field_set) | Copy size readable bytes into a live resolved member without changing its size or key. |
| [`catoo_method_invoke`](#catoo_method_invoke) | Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field. Invalid-argument indices 0–2 identify field, arguments, and output; any larger index is an invalid callback state. |
| [`catoo_property_get`](#catoo_property_get) | Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field. Invalid-argument indices 0–1 identify field and output; any larger index is an invalid callback state. |
| [`catoo_property_set`](#catoo_property_set) | Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field. Invalid-argument indices 0–1 identify field and data; any larger index is an invalid callback state. |
| [`catoo_structure_init`](#catoo_structure_init) | Clone a live object's members into unused structure storage. Keys are fixed by contract. Failure attempts cleanup; remaining ownership is freeable via structure_free. |
| [`catoo_structure_get`](#catoo_structure_get) | Borrow the member associated with a readable key, or return null if absent. Do not change the structure's key set. |
| [`catoo_structure_free`](#catoo_structure_free) | Release the structure's owned storage; failure retains unreleased resources for retry. |
| [`catoo_class_init`](#catoo_class_init) | Clone a live object into unused class storage as member definitions/defaults. Failure attempts cleanup; remaining ownership is freeable via class_free. |
| [`catoo_class_create`](#catoo_class_create) | Clone a live class's definitions into unused object storage. Payload bytes are independent; embedded references remain shared. Failure attempts cleanup; remaining ownership is freeable via object_free. |
| [`catoo_class_extend`](#catoo_class_extend) | Copy non-overlapping member definitions from a live base class with equal key width. Empty base is a no-op; overlapping keys are not implemented. On insertion failure attempt rollback; failed rollback may retain an appended prefix. Capacity/aliases may change. |
| [`catoo_class_free`](#catoo_class_free) | Release the class's owned storage; failure retains unreleased resources for retry. Produced objects have independent payload storage. |
| [`catoo_interface_init`](#catoo_interface_init) | Clone live requirements into unused interface storage. Nonempty requirements need a callable validator. Failure attempts cleanup; remaining ownership is freeable via interface_free. |
| [`catoo_interface_validate`](#catoo_interface_validate) | Check each required key and its validator. Missing/incompatible keys produce FALSE; extra keys are permitted. Validator must not mutate either object and writes validity on SUCCESS. Non-SUCCESS callback results stop validation without writing output. |
| [`catoo_interface_free`](#catoo_interface_free) | Release the interface's owned storage; failure retains unreleased resources for retry. Success clears the validator. |

## API Reference

### CATOOLIB_API

Native C ABI export/import annotation. On Windows/Cygwin, `CATOOLIB_EXPORTS` selects `__declspec(dllexport)` and `CATOOLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the macro is empty. The build targets `catoolib_static` and `catoolib_shared` link the corresponding CATRLLIB dependency. Consumers use `Catalyst::catoo_static` or `Catalyst::catoo_shared`, respectively.

### CATOOLIB_MEMBER

A member's owned byte representation, or the empty descriptor left after release.

```c
typedef struct CATOOLIB_MEMBER {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
} CATOOLIB_MEMBER;
```

**Fields:**  
`data` — writable owned payload, null when empty.  
`size` — payload size in bytes, zero when empty.

**C++:** `MEMBER`

### CATOOLIB_OBJECT

An addressable composite with a map of distinct fixed-width keys to member descriptors.

```c
typedef struct CATOOLIB_OBJECT {
    CATRLLIB_MAP members;
} CATOOLIB_OBJECT;
```

**Fields:**  
`members` — owned `CATRLLIB_MAP`, whose value width is `sizeof(CATOOLIB_MEMBER)`. Use CATOOLIB operations rather than mutating its bookkeeping.

**C++:** `OBJECT`

### CATOOLIB_FIELD

A non-owning resolution of one key and its member within an Object.

```c
typedef struct CATOOLIB_FIELD {
    CATOOLIB_OBJECT* object;
    CATMMLIB_POINTER key;
    CATOOLIB_MEMBER* member;
} CATOOLIB_FIELD;
```

**Fields:**  
`object` — borrowed receiver.  
`key` — borrowed pointer to the canonical stored key.  
`member` — borrowed pointer to the resolved descriptor. All three pointers are null after unsuccessful resolution.

**C++:** `FIELD`

### CATOOLIB_METHOD

The native callable representation stored as a member payload. The pointer must identify a function with this exact signature and calling convention.

```c
typedef CATALYST_RESULT (*CATOOLIB_METHOD)(
    CATOOLIB_OBJECT* object,
    CATMMLIB_POINTER arguments,
    CATMMLIB_POINTERW output
);
```

**Callback parameters:**  
`object` — receiver, borrowed for the call.  
`arguments` — readable argument representation according to the callable's contract.  
`output` — writable result representation according to the callable's contract. Nullability and output validity are defined by the callable.

**C++:** `METHOD`

### CATOOLIB_PROPERTY

The native getter and setter representations stored together as a member payload.

```c
typedef struct CATOOLIB_PROPERTY {
    CATALYST_RESULT (*get)(CATOOLIB_OBJECT*, CATMMLIB_POINTERW);
    CATALYST_RESULT (*set)(CATOOLIB_OBJECT*, CATMMLIB_POINTER);
} CATOOLIB_PROPERTY;
```

**Fields:**  
`get` — callable taking the receiver and writable output.  
`set` — callable taking the receiver and readable input. The selected accessor must be callable; its contract defines argument extents, nullability and output validity.

**C++:** `PROPERTY`

### CATOOLIB_STRUCTURE

An Object whose set of keys cannot change after initialization. Member payloads remain writable.

```c
typedef struct CATOOLIB_STRUCTURE {
    CATOOLIB_OBJECT object;
} CATOOLIB_STRUCTURE;
```

**Fields:**  
`object` — owned fixed-key Object. Do not use its exposed map to add or remove keys.

**C++:** `STRUCTURE`

### CATOOLIB_CLASS

An Object containing a layout and default member representations used to produce other Objects.

```c
typedef struct CATOOLIB_CLASS {
    CATOOLIB_OBJECT object;
} CATOOLIB_CLASS;
```

**Fields:**  
`object` — owned definition Object. Instances copy its payload bytes; references contained in those bytes are not cloned.

**C++:** `CLASS`

### CATOOLIB_INTERFACE

An Object containing member requirements and a validation rule.

```c
typedef struct CATOOLIB_INTERFACE {
    CATOOLIB_OBJECT object;
    CATALYST_RESULT (*validate)(
        const CATOOLIB_MEMBER* member,
        const CATOOLIB_MEMBER* requirement,
        CATALYST_BOOL* valid
    );
} CATOOLIB_INTERFACE;
```

**Fields:**  
`object` — owned requirement Object.  
`validate` — callback accepting the candidate member, requirement member, and writable Boolean. Nonempty requirements need a callable validator; it must not mutate either Object and must write the Boolean on SUCCESS.

**C++:** `INTERFACE`

In Result patterns, `SS` is the dependency/callback status byte and `DD` is its detail byte. The public context and operation bytes are fixed as shown.

### catoo_member_init

```c
CATALYST_RESULT catoo_member_init(CATOOLIB_MEMBER* member, CATALYST_NUINT size);
```

Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member.

**Parameters:**  
`member` — unused writable member descriptor.  
`size` — payload byte count; zero is an invalid argument.

**C++:** `member_init`  
**Context:** `0x01`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x81010101` | ERROR_INVALID_ARGUMENT | Invalid argument. | Size parameter. |
| `0xSS0102DD` | Variable status | Heap allocation failure. | Dependency detail. |
| `0x00010300` | SUCCESS | Member initialized. | — |

### catoo_member_get

```c
void catoo_member_get(const CATOOLIB_MEMBER* member, CATMMLIB_POINTERW output);
```

Copy a live member's size bytes into sufficient writable output storage outside its descriptor; overlap with payload is supported.

**Parameters:**  
`member` — live readable member descriptor.  
`output` — writable buffer of at least `member->size` bytes, outside the descriptor; may overlap payload.

**C++:** `member_get`

**Returns:** No return value; the output receives the member bytes.

### catoo_member_set

```c
void catoo_member_set(CATOOLIB_MEMBER* member, CATMMLIB_POINTER data);
```

Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership.

**Parameters:**  
`member` — live writable member descriptor.  
`data` — readable buffer of `member->size` bytes; may overlap payload.

**C++:** `member_set`

**Returns:** No return value; the member payload is updated without changing size.

### catoo_member_resize

```c
CATALYST_RESULT catoo_member_resize(CATOOLIB_MEMBER* member, CATALYST_NUINT size);
```

Resize a live member to a nonzero byte size, preserving the common prefix and zero-filling growth. Same size is a no-op; failure preserves the payload and size. Successful resize may invalidate payload pointers but not the member descriptor, its key, or its resolved Field.

**Parameters:**  
`member` — live nonempty member descriptor, standalone or borrowed; the member owns its current payload.  
`size` — requested nonzero byte count.

**C++:** `member_resize`  
**Context:** `0x1E`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x811E0101` | ERROR_INVALID_ARGUMENT | Zero requested size; member unchanged. | Size parameter. |
| `0x011E0200` | SUCCESS_NOOP | Requested size equals current size. | — |
| `0xSS1E03DD` | Variable non-success status | Heap resize failed; original payload and size remain live. | Dependency detail. |
| `0x001E0400` | SUCCESS | Member resized. | — |

### catoo_member_free

```c
CATALYST_RESULT catoo_member_free(CATOOLIB_MEMBER* member);
```

Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op.

**Parameters:**  
`member` — standalone initialized owning member, including an empty descriptor; not a borrowed map member.

**C++:** `member_free`  
**Context:** `0x04`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01040100` | SUCCESS_NOOP | Already empty. | — |
| `0xSS0402DD` | Variable status | Heap release failure. | Dependency detail. |
| `0x00040300` | SUCCESS | Member released. | — |

### catoo_object_init

```c
void catoo_object_init(CATOOLIB_OBJECT* object, CATALYST_NUINT ksize);
```

Initialize unused object storage without allocation; ksize is the nonzero byte width of every key.

**Parameters:**  
`object` — unused writable object descriptor.  
`ksize` — nonzero fixed key width in bytes.

**C++:** `object_init`

**Returns:** No return value; the object has null storage, zero capacity and cardinality, and the specified key width.

### catoo_object_add

```c
CATALYST_RESULT catoo_object_add(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key, CATMMLIB_POINTER data, CATALYST_NUINT size);
```

Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members.

**Parameters:**  
`object` — initialized writable object; inserting an absent key requires available capacity or returns BUFFER_OVERFLOW.  
`key` — readable `object->members.ksize` bytes outside the object/map allocations.  
`data` — readable payload of `size` bytes when a new nonempty member is inserted.  
`size` — payload byte count; zero is invalid for an absent key.

**C++:** `object_add`  
**Context:** `0x06`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01060100` | SUCCESS_NOOP | Duplicate key. | — |
| `0x81060203` | ERROR_INVALID_ARGUMENT | Invalid argument. | Size parameter. |
| `0x90060300` | ERROR_BUFFER_OVERFLOW | Insufficient member capacity. | — |
| `0xSS0604DD` | Variable status | Member allocation failure. | Dependency detail. |
| `0x00060500` | SUCCESS | Member added. | — |

### catoo_object_remove

```c
CATALYST_RESULT catoo_object_remove(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);
```

Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The key must be readable during lookup; it need not outlive the removed payload.

**Parameters:**  
`object` — initialized writable object.  
`key` — readable `object->members.ksize` bytes during lookup; may identify bytes in the removed payload.

**C++:** `object_remove`  
**Context:** `0x07`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01070100` | SUCCESS_NOOP | Absent key. | — |
| `0xSS0702DD` | Variable status | Member release failure. | Dependency detail. |
| `0x00070300` | SUCCESS | Member removed. | — |

### catoo_object_contains

```c
CATALYST_BOOL catoo_object_contains(const CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);
```

Test a readable ksize-byte key against an initialized object.

**Parameters:**  
`object` — initialized readable object.  
`key` — readable `object->members.ksize` bytes.

**C++:** `object_contains`

**Returns:** `TRUE` if the key exists; `FALSE` otherwise.

### catoo_object_get

```c
CATOOLIB_MEMBER* catoo_object_get(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);
```

Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, map resize or free attempts that may invalidate the descriptor.

**Parameters:**  
`object` — initialized writable object.  
`key` — readable `object->members.ksize` bytes.

**C++:** `object_get`

**Returns:** Borrowed writable member descriptor if present; null otherwise. Do not release it separately.

### catoo_object_expand

```c
CATALYST_RESULT catoo_object_expand(CATOOLIB_OBJECT* object, CATALYST_NUINT capacity);
```

Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated.

**Parameters:**  
`object` — initialized writable object.  
`capacity` — requested pair capacity, measured in member slots, not bytes.

**C++:** `object_expand`  
**Context:** `0x0A`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000A0100` | SUCCESS | Capacity expanded. | — |
| `0x010A0100` | SUCCESS_NOOP | Capacity unchanged. | — |
| `0xSS0A01DD` | Variable status | Map expansion failure. | Dependency detail. |

### catoo_object_shrink

```c
CATALYST_RESULT catoo_object_shrink(CATOOLIB_OBJECT* object);
```

Halve map capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed members and fields after map resize attempts.

**Parameters:**  
`object` — initialized writable object; target capacity is the greater of cardinality and half the current capacity rounded down.

**C++:** `object_shrink`  
**Context:** `0x0B`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000B0100` | SUCCESS | Capacity reduced. | — |
| `0x010B0100` | SUCCESS_NOOP | Capacity unchanged. | — |
| `0xSS0B01DD` | Variable status | Map shrink failure. | Dependency detail. |

### catoo_object_clear

```c
CATALYST_RESULT catoo_object_clear(CATOOLIB_OBJECT* object);
```

Release and remove all members, retaining map capacity. On failure, already removed members remain removed; remaining members are live.

**Parameters:**  
`object` — initialized writable object; retains capacity and key width.

**C++:** `object_clear`  
**Context:** `0x0C`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x010C0100` | SUCCESS_NOOP | Already empty. | — |
| `0xSS0C02DD` | Variable status | Member release failure. | Dependency detail. |
| `0x000C0300` | SUCCESS | Members removed. | — |

### catoo_object_free

```c
CATALYST_RESULT catoo_object_free(CATOOLIB_OBJECT* object);
```

Release all members then map storage. Failure retains unreleased resources for retry with object_free; success leaves a reusable empty object.

**Parameters:**  
`object` — initialized owning object or partially released object awaiting cleanup.

**C++:** `object_free`  
**Context:** `0x0D`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS0D01DD` | Variable status | Member cleanup failure. | Dependency detail. |
| `0x000D0200` | SUCCESS | Map released. | — |
| `0x010D0200` | SUCCESS_NOOP | Already released. | — |
| `0xSS0D02DD` | Variable status | Map release failure. | Dependency detail. |

### catoo_field_init

```c
CATALYST_BOOL catoo_field_init(CATOOLIB_FIELD* field, CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);
```

Resolve a borrowed field from an initialized object using a readable key. The field borrows the object's stored key and member; input key storage is not retained. Reacquire after invalidating map operations. Absence clears the field and returns FALSE.

**Parameters:**  
`field` — writable destination descriptor, disjoint from the object and lookup bytes.  
`object` — initialized writable receiver.  
`key` — readable lookup key of the object's fixed width; its storage is not retained.

**C++:** `field_init`

**Returns:** `TRUE` and a resolved borrowed field if present; `FALSE` and a zeroed (all-null) field otherwise.

### catoo_field_get

```c
void catoo_field_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);
```

Copy a live resolved member into sufficient writable output storage outside its descriptor.

**Parameters:**  
`field` — live resolved field.  
`output` — writable buffer of at least `field->member->size` bytes, outside the member descriptor; may overlap payload.

**C++:** `field_get`

**Returns:** No return value; the output receives the member bytes.

### catoo_field_set

```c
void catoo_field_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);
```

Copy size readable bytes into a live resolved member without changing its size or key.

**Parameters:**  
`field` — live resolved field.  
`data` — readable buffer of `field->member->size` bytes; may overlap payload.

**C++:** `field_set`

**Returns:** No return value; the member payload is updated without changing its key or size.

### catoo_method_invoke

```c
CATALYST_RESULT catoo_method_invoke(const CATOOLIB_FIELD* field, CATMMLIB_POINTER arguments, CATMMLIB_POINTERW output);
```

Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field. Invalid-argument indices 0–2 identify field, arguments, and output; any larger index is an invalid callback state.

**Parameters:**  
`field` — live resolved field storing the exact callable representation.  
`arguments` — readable argument representation as required by the callable.  
`output` — writable result representation as required by the callable; output validity and nullability follow its contract.

**C++:** `method_invoke`  
**Context:** `0x11`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1101DD` | Variable status except INVALID_ARGUMENT | Callback outcome. | Callback detail. |
| `0x81110100` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Field (receiver) parameter. |
| `0x81110101` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Arguments parameter. |
| `0x81110102` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Output parameter. |
| `0x821102DD` | ERROR_INVALID_STATE | Callback reported an out-of-range argument index. | Rejected callback index (3–255). |

### catoo_property_get

```c
CATALYST_RESULT catoo_property_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);
```

Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field. Invalid-argument indices 0–1 identify field and output; any larger index is an invalid callback state.

**Parameters:**  
`field` — live resolved field storing the exact property representation with a callable getter.  
`output` — writable destination required by the getter; output validity and nullability follow its contract.

**C++:** `property_get`  
**Context:** `0x12`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1201DD` | Variable status except INVALID_ARGUMENT | Callback outcome. | Callback detail. |
| `0x81120100` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Field (receiver) parameter. |
| `0x81120101` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Output parameter. |
| `0x821202DD` | ERROR_INVALID_STATE | Callback reported an out-of-range argument index. | Rejected callback index (2–255). |

### catoo_property_set

```c
CATALYST_RESULT catoo_property_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);
```

Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field. Invalid-argument indices 0–1 identify field and data; any larger index is an invalid callback state.

**Parameters:**  
`field` — live resolved field storing the exact property representation with a callable setter.  
`data` — readable input required by the setter; extent and nullability follow its contract.

**C++:** `property_set`  
**Context:** `0x13`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1301DD` | Variable status except INVALID_ARGUMENT | Callback outcome. | Callback detail. |
| `0x81130100` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Field (receiver) parameter. |
| `0x81130101` | ERROR_INVALID_ARGUMENT | Callback rejects an argument. | Data parameter. |
| `0x821302DD` | ERROR_INVALID_STATE | Callback reported an out-of-range argument index. | Rejected callback index (2–255). |

### catoo_structure_init

```c
CATALYST_RESULT catoo_structure_init(CATOOLIB_STRUCTURE* structure, const CATOOLIB_OBJECT* object);
```

Clone a live object's members into unused structure storage. Keys are fixed by contract. Failure attempts cleanup; remaining ownership is freeable via structure_free.

**Parameters:**  
`structure` — unused writable destination disjoint from the source.  
`object` — live readable source object.

**C++:** `structure_init`  
**Context:** `0x14`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1401DD` | Variable status | Map expansion failure. | Dependency detail. |
| `0xSS1402DD` | Variable status | Member copy failure. | Dependency detail. |
| `0xSS1403DD` | Variable status | Cleanup failure after expansion failure (fatal original outcome takes precedence). | Dependency detail. |
| `0xSS1404DD` | Variable status | Cleanup failure after copy failure (fatal original outcome takes precedence). | Dependency detail. |
| `0x00140500` | SUCCESS | Independent copy created. | — |

### catoo_structure_get

```c
CATOOLIB_MEMBER* catoo_structure_get(CATOOLIB_STRUCTURE* structure, CATMMLIB_POINTER key);
```

Borrow the member associated with a readable key, or return null if absent. Do not change the structure's key set.

**Parameters:**  
`structure` — initialized writable structure.  
`key` — readable key of `structure->object.members.ksize` bytes.

**C++:** `structure_get`

**Returns:** Borrowed writable member descriptor if present; null otherwise. Do not release it separately or change the structure's key set.

### catoo_structure_free

```c
CATALYST_RESULT catoo_structure_free(CATOOLIB_STRUCTURE* structure);
```

Release the structure's owned storage; failure retains unreleased resources for retry.

**Parameters:**  
`structure` — initialized owning structure or partially released structure awaiting cleanup.

**C++:** `structure_free`  
**Context:** `0x16`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00160100` | SUCCESS | Storage released. | — |
| `0x01160100` | SUCCESS_NOOP | Already released. | — |
| `0xSS1601DD` | Variable status | Object release failure. | Dependency detail. |

### catoo_class_init

```c
CATALYST_RESULT catoo_class_init(CATOOLIB_CLASS* class_, const CATOOLIB_OBJECT* definition);
```

Clone a live object into unused class storage as member definitions/defaults. Failure attempts cleanup; remaining ownership is freeable via class_free.

**Parameters:**  
`class_` — unused writable destination disjoint from the definition.  
`definition` — live readable source object defining default member bytes.

**C++:** `class_init`  
**Context:** `0x17`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1701DD` | Variable status | Map expansion failure. | Dependency detail. |
| `0xSS1702DD` | Variable status | Member copy failure. | Dependency detail. |
| `0xSS1703DD` | Variable status | Cleanup failure after expansion failure (fatal original outcome takes precedence). | Dependency detail. |
| `0xSS1704DD` | Variable status | Cleanup failure after copy failure (fatal original outcome takes precedence). | Dependency detail. |
| `0x00170500` | SUCCESS | Independent copy created. | — |

### catoo_class_create

```c
CATALYST_RESULT catoo_class_create(const CATOOLIB_CLASS* class_, CATOOLIB_OBJECT* object);
```

Clone a live class's definitions into unused object storage. Payload bytes are independent; embedded references remain shared. Failure attempts cleanup; remaining ownership is freeable via object_free.

**Parameters:**  
`class_` — live readable class.  
`object` — unused writable destination disjoint from the class.

**C++:** `class_create`  
**Context:** `0x18`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1801DD` | Variable status | Map expansion failure. | Dependency detail. |
| `0xSS1802DD` | Variable status | Member copy failure. | Dependency detail. |
| `0xSS1803DD` | Variable status | Cleanup failure after expansion failure (fatal original outcome takes precedence). | Dependency detail. |
| `0xSS1804DD` | Variable status | Cleanup failure after copy failure (fatal original outcome takes precedence). | Dependency detail. |
| `0x00180500` | SUCCESS | Independent copy created. | — |

### catoo_class_extend

```c
CATALYST_RESULT catoo_class_extend(CATOOLIB_CLASS* class_, const CATOOLIB_CLASS* base);
```

Copy non-overlapping member definitions from a live base class with equal key width. Empty base is a no-op; overlapping keys are not implemented. On insertion failure attempt rollback; failed rollback may retain an appended prefix. Capacity/aliases may change.

**Parameters:**  
`class_` — initialized writable destination class.  
`base` — live readable source class with equal key width when nonempty. Overlapping keys are rejected before mutation.

**C++:** `class_extend`  
**Context:** `0x19`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01190100` | SUCCESS_NOOP | Empty base. | — |
| `0x81190201` | ERROR_INVALID_ARGUMENT | Different key widths. | Base parameter. |
| `0x85190300` | ERROR_NOT_IMPLEMENTED | Overlapping definitions unsupported. | — |
| `0x90190400` | ERROR_BUFFER_OVERFLOW | Member count overflow. | — |
| `0xSS1905DD` | Variable status | Map expansion failure. | Dependency detail. |
| `0xSS1906DD` | Variable status | Member copy failure; appended members rolled back. | Dependency detail. |
| `0xSS1907DD` | Variable status | Rollback failure; fatal original outcome takes precedence. | Dependency detail. |
| `0x00190800` | SUCCESS | Definitions appended. | — |

### catoo_class_free

```c
CATALYST_RESULT catoo_class_free(CATOOLIB_CLASS* class_);
```

Release the class's owned storage; failure retains unreleased resources for retry. Produced objects have independent payload storage.

**Parameters:**  
`class_` — initialized owning class or partially released class awaiting cleanup.

**C++:** `class_free`  
**Context:** `0x1A`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001A0100` | SUCCESS | Storage released. | — |
| `0x011A0100` | SUCCESS_NOOP | Already released. | — |
| `0xSS1A01DD` | Variable status | Object release failure. | Dependency detail. |

### catoo_interface_init

```c
CATALYST_RESULT catoo_interface_init(
    CATOOLIB_INTERFACE* interface_,
    const CATOOLIB_OBJECT* requirements,
    CATALYST_RESULT (*validate)(const CATOOLIB_MEMBER*, const CATOOLIB_MEMBER*, CATALYST_BOOL*)
);
```

Clone live requirements into unused interface storage. Nonempty requirements need a callable validator. Failure attempts cleanup; remaining ownership is freeable via interface_free.

**Parameters:**  
`interface_` — unused writable destination disjoint from requirements.  
`requirements` — live readable requirements object.  
`validate` — callable validation rule for nonempty requirements; may be null for an empty interface. It must write `valid` on SUCCESS without modifying either object.

**C++:** `interface_init`  
**Context:** `0x1B`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1B01DD` | Variable status | Map expansion failure. | Dependency detail. |
| `0xSS1B02DD` | Variable status | Member copy failure. | Dependency detail. |
| `0xSS1B03DD` | Variable status | Cleanup failure after expansion failure (fatal original outcome takes precedence). | Dependency detail. |
| `0xSS1B04DD` | Variable status | Cleanup failure after copy failure (fatal original outcome takes precedence). | Dependency detail. |
| `0x001B0500` | SUCCESS | Independent copy created. | — |

### catoo_interface_validate

```c
CATALYST_RESULT catoo_interface_validate(const CATOOLIB_INTERFACE* interface_, const CATOOLIB_OBJECT* object, CATALYST_BOOL* valid);
```

Check each required key and its validator. Missing/incompatible keys produce FALSE; extra keys are permitted. Validator must not mutate either object and writes validity on SUCCESS. Non-SUCCESS callback results stop validation without writing output.

**Parameters:**  
`interface_` — initialized readable interface with a callable validator if nonempty.  
`object` — initialized readable candidate object.  
`valid` — writable Boolean outside both objects and their storage; written on successful validation, unchanged on a non-SUCCESS callback outcome.

**C++:** `interface_validate`  
**Context:** `0x1C`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001C0100` | SUCCESS | Nonempty requirements have different key width. | — |
| `0x001C0200` | SUCCESS | Required key absent. | — |
| `0x811C0301` | ERROR_INVALID_ARGUMENT | Validator rejects candidate member argument. | Object parameter. |
| `0x811C0400` | ERROR_INVALID_ARGUMENT | Validator rejects requirement argument. | Interface parameter. |
| `0x821C0500` | ERROR_INVALID_STATE | Validator reports an invalid argument index of 2 or greater (not a public argument). | — |
| `0xSS1C06DD` | Variable status | Other non-SUCCESS validator outcome. | Callback detail. |
| `0x001C0700` | SUCCESS | Member did not satisfy requirement. | — |
| `0x001C0800` | SUCCESS | All requirements satisfied. | — |

### catoo_interface_free

```c
CATALYST_RESULT catoo_interface_free(CATOOLIB_INTERFACE* interface_);
```

Release the interface's owned storage; failure retains unreleased resources for retry. Success clears the validator.

**Parameters:**  
`interface_` — initialized owning interface or partially released interface awaiting cleanup.

**C++:** `interface_free`  
**Context:** `0x1D`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001D0100` | SUCCESS | Storage released. | — |
| `0x011D0100` | SUCCESS_NOOP | Already released. | — |
| `0xSS1D01DD` | Variable status | Object release failure. | Dependency detail. |

