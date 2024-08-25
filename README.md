# uCal

This is a small library to deal with calendar-related tasks. It supports the following calendars:

 + Gregorian Calendar (because it's what is widely used in software)
 + Julian Calendar (mostly for the fun of it)
 + GPS calendar (because it can be so tricky)

## What it does

This little library supports basic calendar stuff, like converting a `time_t`value into a civil date, or find the next Thursday after a given date, or convert a date into a `time_t`value.  Then there's functionality to convert two-digit years to a full date if you have not only the YY-MM-DD information but also the day-of-week.

It also provides mappings between the supported calendars, which can be surprisingly tricky to do consistently.  The basic periods of the calendars differ, and without some additional information about the eras in use the mappings may have many solutions.  The library makes no assumptions here (unless explicitely stated) but enforces providing the necessary data.  This should ensure the priciple of the least surprise.


## What it does not

It doesn't deal with string formatting or parsing, and it does not deal with time zones.


## Target constraints

First, a decent C99 compiler (or later) must be available. Rewriting to older standards is possible, but requiring a C99 compiler in the 2020 years seems reasonable.

While the library should be useable even on constrained targets (embedded or IOT systems), it is not limited to that.  The algorithms employed are essentially loop-free, unless division and/or multiplication by/with constants require loops in the runtime.  But even then, the algorithms and calculations employed should behave comparably well.
 
The library uses 32-bit integer arithmetic most of the time, with a few exceptions where 64-bit `time_t` values are part of the function API.  Divisions are division by constants visible to the compiler to gain speed. So the performance is basically bound by the performance of `int32_t` arithmetic for add/sub/mul on your target system.

And you can ignore all of the above if want to use it for apps running under an operating system. It's mostly of interest for people who write software for bare-metal targets. 
 
 
## How to build

A CMake project file is available.  If `doxygen`is available, the API docs will be generated.  If `pdflatex` is available, the `doxygen` docs and the algorithm descriptions and hints from the `TexDoc` folder will be processed to create the corresponding PDF docs.

So it's basically

    $ cmake <path-to-source>
    $ cmake --build .
    $ ctest

Taylor the initial call to `cmake` to your needs, of course. The main build product is a static library.
