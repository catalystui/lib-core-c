# Core

The purpose of CatalystUI Core is to provide a common foundation for working with digital data, regardless of the programming language, operating system, or hardware on which an application runs.

Different languages and systems often represent and operate on the same information in different ways. When libraries rely on those differences, their functionality becomes tied to particular environments, and developers must repeatedly solve the same underlying problems. Core exists to separate those fundamental operations from the environments in which they are performed.

Core implements the [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/) through a native C ABI. By defining shared representations and contracts independently of any particular programming language, Core provides a foundation that other libraries can build upon without requiring a shared language runtime.

This independence extends to hardware. Core is designed to be implementable even on an 8-bit system, without requiring wider native arithmetic. Platform-specific operations are isolated where necessary, rather than allowing the capabilities of a particular system to define the library's fundamental contracts.

Core is not an implementation of any particular layer or domain of the CatalystUI model. It exists to support all of them. By establishing a consistent foundation for digital data and its manipulation, higher-level libraries can focus on their own responsibilities rather than redefining the fundamentals.

The [API Reference](api-reference/index.md) documents the public contracts provided by Core.
