Hobbit
======
This project is meant to be a DL acceleration framework that allows someone to build operations on top of it.
The idea is that LLVM has many impressive optimizations already (via its regular optimizer or [Polly](https://polly.llvm.org/docs/))
and so we should be trying to make it easy for them to do their jobs. In this spirit, we create `llvm::Module`s and 
run optimizations and compilation against those.

This project is very much a work in progress, and since everything I know about LLVM comes from reading the docs and 
seeing what produces (in my mind) correct IR through trial and error, I welcome feedback and PRs.

TODO
----
- Use dot basis to build up gemm/conv
- Finalize buffer sub-indexing and explore other options
 