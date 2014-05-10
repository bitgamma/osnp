## Introduction

**OSNP** (Open Sensor Network Protocol) is a protocol stack enabling creation of sensor networks and home automation systems.

This stack defines the application layer which is transport agnostic and device type independent and a collection of transport protocols which can be wired or wireless. 

Currently we are working on a transport protocol based on IEEE 802.15.4. This repository contains a C implementation of this transport protocol which is being developed using PIC18 as development target, but should be pretty much platform-agnostic.

The protocol specifications and API document of this implementation are open an can be accesed from the [project Wiki](https://github.com/briksoftware/osnp/wiki)

## Goals
**In short:** 
Open Source + Open Hardware + Open Specifications

**More in details:**
The ultimate goal is to provide a flexible and open standard which anybody can use to create smart devices and applications interacting with them. The protocol won't be patent encumbered and the specifications are here, available to read freely without filling any form or other formalities.

The protocol is meant for hobbyists or companies who want to realize a remote controlled device without having to build the entire infrastructure and having to write yet another custom protocol. Project [Uzel](https://github.com/briksoftware/uzel) provides an extensible application server which takes care of managing devices (discovery, pairing, un-pairing, security, etc) and provides a plugin interface to develop your own application. It is an important part of the OSNP ecosystem.

The development of the protocol itself is a community effort, and proposal of new transport protocols, additions to the application layer protocol, etc are very welcome.

## Status

Currently the most important protocol features are already implemented. This includes:

* Device Discovery
* Pairing / Unpairing 
* Security (Integrity + Authentication + Confidentiality + Replay protection)
* Power saving operating modes (Data polling)
* Command/Response handling

Notably, notifications are still missing, but they are trivial to implement and will be implemented as soon as a device using them is specified or as soon as there is nothing else more urgent to do.

The entire protocol stack is very small and can be used on 8-bit microcontroller with 16k program memory, at least 512 bytes of RAM and optionally (but recommended) a 128-byte EEPROM.

The example project, [Gradusnik](https://github.com/briksoftware/gradusnik) is just above 9k when compiled with xc8 (Pro) for the PIC18F14K50 and includes the OSNP-stack, the MRF24J40 driver and the device-specific logic.

## Getting started

If you want to make your own OSNP-compatible device, first read the [specifications](https://github.com/briksoftware/osnp/wiki).

Then you can download the example project, [Gradusnik](https://github.com/briksoftware/gradusnik), an OSNP-compatible thermometer. Follow there the instruction on how to compile or just use it as the base to develop your own device.

Although the example project is PIC18 based, the stack and the radio driver do not use any PIC-specific code and should be usable on any platform with a C compiler.
