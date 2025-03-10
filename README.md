# Handel Software API

Handel provides a high level interface for a number of XIA’s products. The basic idea
is to provide an API that requires as little knowledge about the hardware as is
reasonable. We wrote the API using ANSI C for ease of use and compatibility.

## Documentation

The [project documentation is available on the web](https://handel.xia.com).

## API Update Policy

By necessity, we will be making changes to the public API calls and the SDK. We will
adopt a process of depreciating interfaces then removing them. We will provide notice of

* no less than 6 months before removing an interface, and
* no less than 3 months before changing an interface.

Notices will appear in the source code, documentation, and release notes. We will
notify users via email 30 days prior to the deprecation date.

Deprecated interfaces will receive bug fixes during their deprecation phase, but will
receive no added functionality. This policy may not cover all situations, and will
evolve over time. For situations that don’t fit neatly into this policy, we will
ensure that the information is available in all relevant Release Notes.
