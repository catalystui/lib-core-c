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

Keys have a single nonzero byte width per object and are compared as bytes. Expand capacity before adding; a duplicate is a no-op. The map's exposed fields are not an alternative mutation API. Borrowed members and fields are invalidated by removal, clear, resize or free attempts. Field keys additionally borrow the caller's key storage. Do not independently free borrowed members or change their ownership or size.

A method is stored by copying the bytes of a native `CATOOLIB_METHOD` function pointer; a property is stored by copying a `CATOOLIB_PROPERTY`. Callers establish the exact callable types and callback contracts before invoking; no runtime type metadata is introduced. Callback results preserve status/detail under CATOOLIB context/operation codes, except interface validation remaps invalid-argument details. A callback may invalidate a resolved field by mutating its object.

Structures have fixed keys by API contract, not compiler enforcement. Classes store default member bytes and create independent instances; references embedded in those bytes remain shared. Class extension copies disjoint layouts. Conflicting keys are intentionally unsupported until a policy is approved. No nominal inheritance, class identity tracking, or automatic lifecycle callbacks are implied.

Interfaces require a subset of the object's keys and validate each corresponding member using the supplied rule. An empty interface accepts every object; a nonempty interface requires matching key widths and a callable validator. The rule must not modify either object and must write a Catalyst Boolean on SUCCESS. Non-SUCCESS results stop without writing the caller's validity output.

## Summary

| C type | C++ type | Description |
| --- | --- | --- |
| `CATOOLIB_MEMBER` | `MEMBER` | Owned payload and byte size. |
| `CATOOLIB_OBJECT` | `OBJECT` | Addressable member map. |
| `CATOOLIB_FIELD` | `FIELD` | Borrowed object, key and member. |
| `CATOOLIB_METHOD` | `METHOD` | Callable taking an object, arguments and output. |
| `CATOOLIB_PROPERTY` | `PROPERTY` | Getter and setter callbacks. |
| `CATOOLIB_STRUCTURE` | `STRUCTURE` | Object with a fixed key set. |
| `CATOOLIB_CLASS` | `CLASS` | Object containing default member definitions. |
| `CATOOLIB_INTERFACE` | `INTERFACE` | Object containing requirements and a validation callback. |

| Operation | Description |
| --- | --- |
| [`catoo_member_init`](#catoo_member_init) | Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member. |
| [`catoo_member_get`](#catoo_member_get) | Copy a live member's size bytes into sufficient writable output storage outside its descriptor; overlap with payload is supported. |
| [`catoo_member_set`](#catoo_member_set) | Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership. |
| [`catoo_member_free`](#catoo_member_free) | Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op. |
| [`catoo_object_init`](#catoo_object_init) | Initialize unused object storage without allocation; ksize is the nonzero byte width of every key. |
| [`catoo_object_add`](#catoo_object_add) | Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members. |
| [`catoo_object_remove`](#catoo_object_remove) | Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The readable key must remain live through removal, including after payload release. |
| [`catoo_object_contains`](#catoo_object_contains) | Test a readable ksize-byte key against an initialized object. |
| [`catoo_object_get`](#catoo_object_get) | Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, resize or free attempts. |
| [`catoo_object_expand`](#catoo_object_expand) | Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated. |
| [`catoo_object_shrink`](#catoo_object_shrink) | Halve capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed fields after resize attempts. |
| [`catoo_object_clear`](#catoo_object_clear) | Release and remove all members, retaining map capacity. On failure, already removed members remain removed; remaining members are live. |
| [`catoo_object_free`](#catoo_object_free) | Release all members then map storage. Failure retains unreleased resources for retry with object_free; success leaves a reusable empty object. |
| [`catoo_field_init`](#catoo_field_init) | Resolve a borrowed field from an initialized object using a readable key. The caller retains the key; reacquire after invalidating map operations. Absence clears the field and returns FALSE. |
| [`catoo_field_get`](#catoo_field_get) | Copy a live resolved member into sufficient writable output storage outside its descriptor. |
| [`catoo_field_set`](#catoo_field_set) | Copy size readable bytes into a live resolved member without changing its size or key. |
| [`catoo_method_invoke`](#catoo_method_invoke) | Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field. |
| [`catoo_property_get`](#catoo_property_get) | Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field. |
| [`catoo_property_set`](#catoo_property_set) | Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field. |
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

In Result patterns, `SS` is the dependency/callback status byte and `DD` is its detail byte. The public context and operation bytes are fixed as shown.

### catoo_member_init

```c
CATALYST_RESULT catoo_member_init(CATOOLIB_MEMBER* member, CATALYST_NUINT size);
```

Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member.

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

**C++:** `member_get`

### catoo_member_set

```c
void catoo_member_set(CATOOLIB_MEMBER* member, CATMMLIB_POINTER data);
```

Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership.

**C++:** `member_set`

### catoo_member_free

```c
CATALYST_RESULT catoo_member_free(CATOOLIB_MEMBER* member);
```

Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op.

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

**C++:** `object_init`

### catoo_object_add

```c
CATALYST_RESULT catoo_object_add(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key, CATMMLIB_POINTER data, CATALYST_NUINT size);
```

Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members.

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

Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The readable key must remain live through removal, including after payload release.

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

**C++:** `object_contains`

### catoo_object_get

```c
CATOOLIB_MEMBER* catoo_object_get(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);
```

Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, resize or free attempts.

**C++:** `object_get`

### catoo_object_expand

```c
CATALYST_RESULT catoo_object_expand(CATOOLIB_OBJECT* object, CATALYST_NUINT capacity);
```

Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated.

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

Halve capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed fields after resize attempts.

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

Resolve a borrowed field from an initialized object using a readable key. The caller retains the key; reacquire after invalidating map operations. Absence clears the field and returns FALSE.

**C++:** `field_init`

### catoo_field_get

```c
void catoo_field_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);
```

Copy a live resolved member into sufficient writable output storage outside its descriptor.

**C++:** `field_get`

### catoo_field_set

```c
void catoo_field_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);
```

Copy size readable bytes into a live resolved member without changing its size or key.

**C++:** `field_set`

### catoo_method_invoke

```c
CATALYST_RESULT catoo_method_invoke(const CATOOLIB_FIELD* field, CATMMLIB_POINTER arguments, CATMMLIB_POINTERW output);
```

Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field.

**C++:** `method_invoke`  
**Context:** `0x11`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1101DD` | Variable status | Callback outcome. | Callback detail. |

### catoo_property_get

```c
CATALYST_RESULT catoo_property_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);
```

Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field.

**C++:** `property_get`  
**Context:** `0x12`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1201DD` | Variable status | Callback outcome. | Callback detail. |

### catoo_property_set

```c
CATALYST_RESULT catoo_property_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);
```

Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field.

**C++:** `property_set`  
**Context:** `0x13`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1301DD` | Variable status | Callback outcome. | Callback detail. |

### catoo_structure_init

```c
CATALYST_RESULT catoo_structure_init(CATOOLIB_STRUCTURE* structure, const CATOOLIB_OBJECT* object);
```

Clone a live object's members into unused structure storage. Keys are fixed by contract. Failure attempts cleanup; remaining ownership is freeable via structure_free.

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

**C++:** `structure_get`

### catoo_structure_free

```c
CATALYST_RESULT catoo_structure_free(CATOOLIB_STRUCTURE* structure);
```

Release the structure's owned storage; failure retains unreleased resources for retry.

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

**C++:** `interface_validate`  
**Context:** `0x1C`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001C0100` | SUCCESS | Nonempty requirements have different key width. | — |
| `0x001C0200` | SUCCESS | Required key absent. | — |
| `0x811C0301` | ERROR_INVALID_ARGUMENT | Validator rejects candidate member argument. | Object parameter. |
| `0x811C0400` | ERROR_INVALID_ARGUMENT | Validator rejects requirement argument. | Interface parameter. |
| `0x821C0500` | ERROR_INVALID_STATE | Validator reports invalid internal output argument. | — |
| `0xSS1C06DD` | Variable status | Other non-SUCCESS validator outcome. | Callback detail. |
| `0x001C0700` | SUCCESS | Member did not satisfy requirement. | — |
| `0x001C0800` | SUCCESS | All requirements satisfied. | — |

### catoo_interface_free

```c
CATALYST_RESULT catoo_interface_free(CATOOLIB_INTERFACE* interface_);
```

Release the interface's owned storage; failure retains unreleased resources for retry. Success clears the validator.

**C++:** `interface_free`  
**Context:** `0x1D`  
**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001D0100` | SUCCESS | Storage released. | — |
| `0x011D0100` | SUCCESS_NOOP | Already released. | — |
| `0xSS1D01DD` | Variable status | Object release failure. | Dependency detail. |

