# VxKex-NEXT — Classic Bluetooth Compatibility Fork

[![Build](https://github.com/ArchitectOfRuin/VxKex-NEXT/actions/workflows/build.yml/badge.svg)](https://github.com/ArchitectOfRuin/VxKex-NEXT/actions/workflows/build.yml)

A focused fork of [VxKex-NEXT](https://github.com/YuZhouRen86/VxKex-NEXT)
dedicated to improving classic Bluetooth API compatibility on Windows 7.

## Fork Focus

This fork intentionally keeps its scope narrow and focuses on the classic
Bluetooth API layer.

The current changes add compatibility handling for the following APIs:

- `BluetoothFindFirstDevice`
- `BluetoothFindNextDevice`
- `BluetoothFindDeviceClose`
- `BluetoothFindFirstRadio`
- `BluetoothFindNextRadio`
- `BluetoothFindRadioClose`

The implementations are exposed through `KxUser` and resolve the corresponding
system functions from `bthprops.cpl` at runtime.

No broad changes to the rest of VxKex-NEXT are intended. Existing GATT-related
code and unrelated compatibility components are kept unchanged.

## Current Status

- ✅ Classic Bluetooth API wrapper implementation
- ✅ `KxUser` exports for the six targeted APIs
- ✅ GitHub Actions build completed successfully
- ✅ Classic Bluetooth API runtime validation completed successfully on Windows 7

## Why This Fork Exists

This repository is maintained as a focused compatibility branch for investigating
and improving classic Bluetooth API behavior in VxKex-NEXT.

The goal is to keep Bluetooth-specific work isolated, reviewable, and easy to
compare with the upstream project.

## Upstream

This project is based on
[VxKex-NEXT by YuZhouRen86](https://github.com/YuZhouRen86/VxKex-NEXT).

Upstream provides the general Windows API extension and compatibility framework.
This fork concentrates specifically on classic Bluetooth compatibility.

## Build

The project uses the existing VxKex-NEXT build system and GitHub Actions
workflow with the Windows 7.1 SDK.

See the upstream documentation for general build and installation instructions.
