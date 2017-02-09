bomalloc
========

(c) Benjamin O'Halloran

[![Build Status](https://travis-ci.org/benohalloran/bomalloc.svg?branch=master)](https://travis-ci.org/benohalloran/bomalloc)

# About

A speculative malloc(3) package for use in [BOP](https://github.com/bop-langs/cbop).
This

The previous allocators used in BOP allocated a large chunk of memory before
speculation and
divided up this region to speculative tasks or aborting the speculation
when memory ran out.
Bomalloc improves upon these allocators by allowing for arbitrary heap growth while
speculating and synchronizing this growth in order to avoid artificial conflicts.
