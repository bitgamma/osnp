## Introduction

**OSNP** (Open Sensor Network Protocol) is a protocol stack enabling creation of sensor networks and home automation systems.

This stack defines the application layer which is transport agnostic and device type independent and a collection of transport protocols which can be wired or wireless. 

Currently we are working on a transport protocol based on IEEE 802.15.4. This repository contains a C implementation of this transport protocol which is being developed using PIC18 as development target, but should be pretty much platform-agnostic.

The protocol specifications and API document of this implementation are open an can be accesed from the [project wiki](https://github.com/briksoftware/osnp/wiki).

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
* Notifications
* BER-TLV parser and encoder

The entire protocol stack is very small and can be used on 8-bit microcontroller with 16k program memory, at least 512 bytes of RAM and optionally (but recommended) a 128-byte EEPROM.

The example project, [Gradusnik](https://github.com/briksoftware/gradusnik) is just above 9k when compiled with xc8 (Pro) for the PIC18LF24K22 and includes the OSNP-stack, the MRF24J40 driver and the device-specific logic.

## Getting started

If you want to make your own OSNP-compatible device, first read the [specifications](https://github.com/briksoftware/osnp/wiki).

Then you can download the example project, [Gradusnik](https://github.com/briksoftware/gradusnik), an OSNP-compatible thermometer. Follow there the instruction on how to compile or just use it as the base to develop your own device.

Although the example project is PIC18 based, the stack and the radio driver do not use any PIC-specific code and should be usable on any platform with a C compiler.

## Key architectural concepts

The high-level network architecture of OSNP is a star-network, where a hub controls all associated devices and has the ability to discover new ones. Devices never speak to each other, only with the hub, which knows what to do with them and how to communicate with them. The devices can be anything ranging from sensors (temperature, moisture, etc) to remote-controlled switches, control panels, water pumps, HVAC.

For this hub to be useful, it should of course act as an application server with virtual appliances controlling one or more devices according to their functionality. Virtual appliances could be anything from plant irrigation systems (controlling water pumps in relation to time, moisture sensors, temperature sensors, etc) to HVAC control systems.

We are developing such a hub with project [Uzel](https://github.com/briksoftware/uzel).

The important thing is that all OSNP-compatible devices have a specific device type (and can be composed of sub-devices if they provide functions from more than one category) and they must communicate using exactly the same way, so an appliance using temperature sensors can work with any OSNP-compatible temperature sensor, regardless of the manufacturer.

To achieve this level of interoperability the application level protocol is very flexible and allows devices to declare their capabilities allowing them to implement only a subset of what is defined for a specific category. It also is very robust in relation to addition of new features because being based on ASN.1 (BER-TLV) encoding it does not rely on fixed field sequences or even fixed length fields.

The specifications for each device type will also be published here and any addition proposed by the community will be evaluated and added.
