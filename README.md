# libzinc
Just a utility library full of various tools I often find myself
wanting that aren't in the C++ standard library. 

Without this, I'd end up with 5-6 different implementations of 
the same things with even more questionable quality than this library.

# Usage
It's header only, so you can just copy the headers if you'd like.
`CMakeLists.txt` also defines an interface library named `libzinc`
that will make any target linking it include the right headers, 
so that's usable as well. 

# License
Licensed entirely under Apache-2.0, found in the `LICENSE` file
in the root of this project.
