# Changes
- https://github.com/dotexe1337/VxKex/ changes merged.
- https://github.com/tahaghafuri/vxkex changes merged.
- https://github.com/YuZhouRen86/VxKex-NEXT changes merged.

# Changes pending merge
- https://github.com/BeitDina/VxKex
- https://github.com/Aqua-Vision/MgKex
- https://github.com/OneForgottenMan/VxKex
- https://github.com/winrevive/project-2600
- https://github.com/kernel-extenders/VxKex
- https://github.com/NulAsh/VxKex
- https://github.com/tester9071348/VxKex-Extended
- https://github.com/CopperFr/VxKex
* If there is another repository that is not here, please let me know.

# Goal
Collecting and merging all vxkex forks in one complete edition

Introduction
============

VxKex is a set of API extensions for Windows 7 that allow some Windows 8, 8.1 and 10-exclusive applications to run on Windows 7.

Community
=========

- Join us on Telegram: [t.me/vxkexdev](https://t.me/vxkexdev/)

**Performing the following operations before installing is recommended.**

- **Uninstall**
  - **0patch Agent**
    It might cause Chromium-based browsers and JetBrains IDEs to crash after enabling VxKex and running it.

- **Update**
  - **MacType → 2025.6.9+**
    An old version of MacType might cause all programs to fail to start after enabling VxKex.

After installation, usage is easy. Here are the ways to enable VxKex:
1. Just right click on a program, open the Properties dialog, and select "VxKex" tab. Then, check the check box which says "Enable VxKex for this program", and try to run the program.
2. Find "VxKex Global Settings" from start menu and open it, click "Add" button, select a program, click "Open" button, and try to run the program.

![VxKex configuration GUI](/example-screenshot.png)

Some programs require additional configuration. There's a file called "**Application Compatibility List.docx**" inside the VxKex installation folder (which is C:\Program Files\VxKex by default) which details these steps, but for the most part, all configuration is self-explanatory.

If you are a developer, source code is provided as a 7z file on the releases page.

FAQ
===

**Q: What applications are supported?**

**A**: The list of compatible applications includes, but is not limited to:

- Bespoke Synth
- Blender
- Blockbench
- Calibre
- Chromium (including Ungoogled Chromium)
- Citra
- Commander Wars
- Cygwin
- Dasel
- Discord Canary
- ElectronMail
- Firefox
- GIMP
- GitHub Desktop
- HandBrake
- Kodi
- Life is Strange: True Colors 4.25
- Listary
- MKVToolNix
- MongoDB
- MPC-Qt
- MPV
- MPV.NET
- Opera
- osu!lazer
- Python
- qBittorrent
- QMMP
- Qt Creator
- Rufus
- Steel Bank Common Lisp
- SpaceEngine
- Spotify
- Steinberg SpectraLayers
- TeamTalk
- Universe Sandbox
- VSCode and VSCodium
- WinDbg (classic from Windows 11 SDK, and preview)
- Yuzu (gameplay was not tested)
- Zig

See the **Application Compatibility List.docx** file, which is installed together with VxKex, for more information.

The majority of Qt6 applications will work, and many Electron applications will work as well.

**Q: Does VxKex modify system files? Will it make my system unstable?**

**A**: VxKex does not modify any system files. Its effect on the whole system is extremely minimal. No background services are used, no global hooks are installed, and the shell extensions and DLLs that are loaded have minimal impact and can be disabled if needed. You can rest assured that your Windows 7 will remain as stable as it always is.

**Q: Do I need to have specific updates installed?**

**A**: Users of Windows 7 without any updates can still use it, but many programs require Service Pack 1, KB2533623 (DllDirectories update) and KB2670838 (Platform Update) in order to run. It is a good idea to install those updates.

**Q: If I have ESUs (Extended Security Updates) installed, can I use VxKex?**

**A**: Yes. There is no problem with ESUs.

**Q: Can I use this with Windows 8 or 8.1?**

**A**: Currently, VxKex is designed for use only with Windows 7. If you use Windows 8 or 8.1, VxKex will do nothing useful.

**Q: Can I remove VxKex or VxKex after upgrading to Windows 8/8.1/10/11?**

**A**: Yes. If VxKex is installed, update it to VxKex, then uninstall it from control panel.

**Q: How does VxKex work?**

**A**: VxKex works by loading a DLL into each program where VxKex is enabled. This is accomplished through using the IFEO (Image File Execution Options) registry key.

Specifically, the "VerifierDlls" value is set to point to a VxKex DLL. This DLL then loads into the process.

API extension is accomplished by editing the program's DLL import table so that instead of importing from Windows 8/8.1/10/11 DLLs, it imports VxKex DLLs instead. These VxKex DLLs contain implementations of Windows API functions which were introduced in newer versions of Windows.

# Disclaimer

This project is not affiliated with Microsoft Corporation. Windows is a registered trademark of Microsoft Corporation. 
