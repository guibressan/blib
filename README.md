# BLIB
---

## Components
* Test Runner
```
A simple way to run your tests.
```
* Slice (dynamic array)
```
A Slice that integrates beautifully with the allocators. Used to build the other
components.
```
* Allocators
```
The Allocator interface turns possible to use dependecy injection for allocators,
need to switch the allocator implementation? No problem, just inject another
allocator.
```
* Growing Arena Allocator
```
Each call to malloc gives you another lifetime to take care of, use arenas and
start solving problems instead of spending your time with memory management.
```
* Heap Allocator
```
A simple malloc wrapper that when BLIB_DEBUG is defined tells you about memory
leaks, double frees and also produce reports about the allocations.
```
* Errors
```
Wrap error messages and print them when is needed.
```
---

```
- Do not try and bend the spoon, that's impossible. Instead, only try to realize
the truth.  

- What truth? 

- There is no spoon.
```
