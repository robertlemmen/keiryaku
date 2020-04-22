# Keiryaku

... a toy scheme interpreter

## Status and Features

This is very incomplete, probably quite buggy and a toy anyway. That said:

* Runs basic scheme code, e.g. all the samples from "The Little Schemer"
* Direct interpreter, but with tail calls
* Compiler stage implemented in scheme (very simple and ugly)
* Precise and generational garbage collector (slow anyway)
* Strings, vectors, booleans, 32-bit ints and floats 
* Dynamic bindings

## Roadmap

Over time I would like this to be a complete and correct R7RS implementation,
while still being built as a direct interpreter. The next planned steps are in
the [ToDo List](TODO.md), and there are a lot of `XXX` fixmes in the code.

## Building

After checking out the repository, you need to pull the dependencies that reside
in submodules:

    git submodule update --init --recursive

After that you can build keiryaku and run the built-in tests:

	make
	make test

This is known to work on 64-bit linux with GCC 8.3 as well as CLang 7.0, if you 
have any interesting build experiences please let me know.

## Usage

Running keiryaku directly will give you a basic REPL with line editing and
history:

    ./keiryaku

But it can of course also read scheme code from STDIN:

    ./keiryaku < test.ss

You can skip the REPL part and execute a scheme file directly:

    ./keiryaku test.ss

Which also means that you can put a shebang line in a scheme script and execute
it directly if your keiryaku is anywhere in your $PATH, e.g.:

```
#!/usr/bin/env keiryaku

(display "hello world!")
(newline)
```

There are a handful of optional cmdline arguments, but they are only really
really required for development, e.g. to switch on interpreter debugging. You
can get a list with `--help`.

## License

Keiryaku is licensed under the [Artistic License 2.0](https://opensource.org/licenses/Artistic-2.0).

## Feedback and Contact

Please let me know what you think: Robert Lemmen <robertle@semistable.com>
