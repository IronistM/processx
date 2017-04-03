


# processx

> Execute and Control System Processes

[![Linux Build Status](https://travis-ci.org/r-pkgs/processx.svg?branch=master)](https://travis-ci.org/r-pkgs/processx)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/github/r-pkgs/processx?svg=true)](https://ci.appveyor.com/project/gaborcsardi/processx)
[![](https://www.r-pkg.org/badges/version/processx)](https://www.r-pkg.org/pkg/processx)
[![CRAN RStudio mirror downloads](https://cranlogs.r-pkg.org/badges/processx)](https://www.r-pkg.org/pkg/processx)
[![Coverage Status](https://img.shields.io/codecov/c/github/r-pkgs/processx/master.svg)](https://codecov.io/github/r-pkgs/processx?branch=master)

Portable tools to run system processes in the background,
read their standard output and error, kill and restart them.

`processx` can poll the standard output and error of a single process,
or multiple processes, using the operating system's polling and waiting
facilities, with a timeout.

---

  - [Features](#features)
  - [Installation](#installation)
  - [Usage](#usage)
    - [Starting processes](#starting-processes)
    - [Killing and restarting a process](#killing-and-restarting-a-process)
    - [Standard output and error](#standard-output-and-error)
    - [End of output](#end-of-output)
    - [Waiting on a process](#waiting-on-a-process)
	- [Exit statuses](#exit-statuses)
	- [Errors](#errors)
  - [License](#license)

## Features

* Start system processes in the background and find their
  process id.
* Read the standard output and error, using non-blocking R connection
  objects.
* Poll the standard output and error connections of a single process or
  multiple processes.
* Check if a background process is running.
* Wait on a background process.
* Get the exit status of a background process, if it has already
  finished.
* Kill background processes, together with their children.
* Kill background process, when its associated object is garbage
  collected.
* Restart background processes.
* Portable, works on Linux, macOS and Windows.
* Lightweight, it only depends on the also lightweight
  `R6`, `assertthat`, `crayon` and `debugme` packages.

## Installation


```r
source("https://install-github.me/r-pkgs/processx")
```

## Usage

Note: the following external commands are usually present in macOS and
Linux systems, but not necessarily on Windows.


```r
library(processx)
```

### Starting processes

`processx` provides two ways to start processes. The first one
requires a single command, and a character vector of arguments.
You don't need to quote the command or the arguments, as they are
passed directly to the operating system, without an intermediate shell.


```r
p1 <- process$new("sleep", "20")
```

The other way is to supply a full shell command line via the
`commandline` argument:


```r
p2 <- process$new(commandline = "sleep 20")
```

This methods starts up a shell, i.e. on Unix-like systems it runs
`sh -c <command>` and on Windows `cmd /c <command>`. (If you need a
different shell, then specify that directly in the `command` argument.

### Killing and restarting a process

A process can be killed via the `kill` method. This also kills
all child processes (unless they created a new process group on Unix,
or a new job object on Windows).


```r
p1$is_alive()
```

```
#> [1] TRUE
```

```r
p2$is_alive()
```

```
#> [1] TRUE
```

```r
p1$kill()
```

```
#> [1] TRUE
```

```r
p2$kill()
```

```
#> [1] TRUE
```

```r
p1$is_alive()
```

```
#> [1] FALSE
```

```r
p2$is_alive()
```

```
#> [1] FALSE
```

A process can be restarted via `restart`. This works if the process
has been killed, if it has finished regularly, or even if it is running
currently. If it is running, then it will be killed first.


```r
p1$restart()
p1$is_alive()
```

```
#> [1] TRUE
```

### Standard output and error

By default the standard output and error of the processes are ignored.
You can set the `stdout` and `stderr` constructor arguments to a file name,
and then they are redirected there, or to `"|"`, and then `processx` creates
R connections to them.

Note, that the R connections have a buffer, which can fill up, if R does
not read out the output, and then the process will stop, until R reads the
connection and the buffer is freed.

The `read_output_lines` and `read_error_lines` methods can be used
to read from the standard output or error connections. They work the same
way as the `readLines` base function.

Alternatively, you can query the connections via the `get_output_connection`
and `get_error_connection` functions, and work with them directly.

Note that the connections used for reading the output and error streams
are non-blocking text connections, so the read functions will return
immediately, even if there is no text to read from them.


```r
p <- process$new(commandline = "echo foo; >&2 echo bar; echo foobar",
                 stdout = "|", stderr = "|")
p$read_output_lines()
```

```
#> [1] "foo"    "foobar"
```

```r
p$read_error_lines()
```

```
#> [1] "bar"
```

To check if there is anything available for reading on the standard output
or error streams, you can poll the R connections:


```r
p <- process$new(commandline = "echo foo; sleep 2; echo bar", stdout = "|")

Sys.sleep(1)
## There must be output now
p$is_incomplete_output()
```

```
#> [1] TRUE
```

```r
p$read_output_lines()
```

```
#> [1] "foo"
```

```r
## There is no more output now
p$is_incomplete_output()
```

```
#> [1] TRUE
```

```r
p$read_output_lines()
```

```
#> character(0)
```

```r
Sys.sleep(2)
## There is output again
p$is_incomplete_output()
```

```
#> [1] TRUE
```

```r
p$read_output_lines()
```

```
#> [1] "bar"
```

```r
## There is no more output
p$is_incomplete_output()
```

```
#> [1] FALSE
```

```r
p$read_output_lines()
```

```
#> character(0)
```

### End of output

There is no standard way in R to signal the end of a connection,
unfortunately. Most R I/O is blocking, and the end of file is reached when
nothing can be read from the connection. This clearly does not work for
non-blocking connections.

For `processx` standard output and error streams, you can use the
`is_incomplete_output` and `is_incomplete_error` functions to check if
there is any chance that more output will arrive on them later.


### Waiting on a process

As seen before, `is_alive` checks if a process is running. The `wait`
method can be used to wait until it has finished (or a specified timeout
expires).. E.g. in the following code `wait` needs to wait about 2 seconds
for the `sleep` shell command to finish.


```r
p <- process$new(commandline = "sleep 2")
p$is_alive()
```

```
#> [1] TRUE
```

```r
Sys.time()
```

```
#> [1] "2017-04-03 22:26:35 BST"
```

```r
p$wait()
Sys.time()
```

```
#> [1] "2017-04-03 22:26:37 BST"
```

It is safe to call `wait` multiple times:


```r
p$wait() # already finished!
```

### Exit statuses

After a process has finished, its exit status can be queried via the
`get_exit_status` method. If the process is still running, then this
method returns `NULL`.


```r
p <- process$new(commandline = "sleep 2")
p$get_exit_status()
```

```
#> NULL
```

```r
p$wait()
p$get_exit_status()
```

```
#> [1] 0
```

### Errors

Errors are typically signalled via non-zero exits statuses. The `processx`
constructor fails if the external program cannot be started,
but it does not deal with errors that happen after the
program has successfully started running.


```r
p <- process$new("nonexistant-command-for-sure")
```

```
#> Error in process_initialize(self, private, command, args, commandline, : processx error
```

## License

MIT © Mango Solutions, Gábor Csárdi
