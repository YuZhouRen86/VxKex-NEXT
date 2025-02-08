Introduction  介绍
============

VxKex NEXT is a set of API extensions for Windows 7 that allow some Windows 8, 8.1 and 10-exclusive applications to run on Windows 7.  
VxKex NEXT 是一套适用于 Windows 7 的 API 扩展，可让一些 Windows 8、8.1 和 10 独占应用程序在 Windows 7 上运行。

To download and install, see the [releases page](https://github.com/YuZhouRen86/VxKex-NEXT/releases).  
如需下载和安装，请参阅[版本页面](https://github.com/YuZhouRen86/VxKex-NEXT/releases)。

**Uninstalling the following softwares before installing is recommanded.**  
**安装前，建议卸载以下软件。**

- **0patch Agent**  
  It might cause Chromium-based browsers and JetBrains IDEs to crash after enabling VxKex NEXT and running it.  
  它可能导致基于 Chromium 的浏览器和 JetBrains IDE 在启用 VxKex NEXT 并运行后崩溃。

- **MacType**  
  It might cause all programs to fail to start after enabling VxKex NEXT.  
  它可能导致所有程序在启用 VxKex NEXT 后无法启动。

After installation, usage is easy. Here are the ways to enable VxKex NEXT:
1. Just right click on a program (.exe or .msi), open the Properties dialog, and select "VxKex" tab. Then, check the check box which says "Enable VxKex NEXT for this program", and try to run the program. For shortcuts, select "Shortcut" tab in the Properties dialog, click "Open file location" button, and then perform the actions above.
2. Find "VxKex NEXT Global Settings" from start menu and open it, click "Add" button, select a program (.exe or .msi), click "Open" button, and try to run the program.

安装后，使用很简单。以下是启用 VxKex NEXT 的方法：
1. 右键单击程序（.exe 或 .msi），打开属性对话框，选择“VxKex”选项卡。然后，选中“为此程序启用 VxKex NEXT”复选框，并尝试运行程序。对于快捷方式，需要在属性对话框中选择“快捷方式”选项卡，点击“打开文件位置”按钮，然后执行以上操作。
2. 从开始菜单中找到“VxKex NEXT Global Settings”并打开，点击“添加”按钮，选择程序（.exe 或 .msi），点击“打开”按钮，并尝试运行程序。

![VxKex configuration GUI](/example-screenshot.png)

Some programs require additional configuration. There's a file called "**Application Compatibility List.docx**" inside the VxKex NEXT installation folder (which is C:\Program Files\VxKex by default) which details these steps, but for the most part, all configuration is self-explanatory.  
有些程序需要额外配置。VxKex NEXT 安装文件夹（默认为 C:\Program Files\VxKex 文件夹）内有一个名为“**Application Compatibility List.docx**”的文件，其中详细说明了这些步骤，但大多数情况下，所有配置都是不言自明的。

If you are a developer, source code is provided as a 7z file on the releases page.  
如果您是开发人员，源代码将以 7z 文件的形式在版本页面上提供。

FAQ  常见问题
===

**Q: What applications are supported?**  
**问：哪些应用程序受支持？**

**A**: The list of compatible applications includes, but is not limited to:  
**答**：兼容的应用程序包括但不限于：

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

See the **Application Compatibility List.docx** file, which is installed together with VxKex NEXT, for more information.  
如需获取更多信息，请参阅与 VxKex NEXT 一起安装的**Application Compatibility List.docx**文件。

The majority of Qt6 applications will work, and many Electron applications will work as well.  
大多数 Qt6 应用程序都能正常运行，许多 Electron 应用程序也能正常运行。

**Q: Does VxKex NEXT modify system files? Will it make my system unstable?**  
**问：VxKex NEXT 会修改系统文件吗？它会使我的系统不稳定吗？**

**A**: VxKex NEXT does not modify any system files. Its effect on the whole system is extremely minimal. No background services are used, no global hooks are installed, and the shell extensions and DLLs that are loaded have minimal impact and can be disabled if needed. You can rest assured that your Windows 7 will remain as stable as it always is.  
**答**：VxKex NEXT 不会修改任何系统文件。它对整个系统的影响极小。不使用后台服务，不安装全局钩子，加载的 shell 扩展和 DLL 影响也很小，可以随时禁用。您可以放心，您的 Windows 7 将一如既往地保持稳定。

**Q: Do I need to have specific updates installed?**  
**问：我需要安装特定的更新吗？**

**A**: Users of Windows 7 without any updates can still use it, but many programs require Service Pack 1, KB2533623 (DllDirectories update) and KB2670838 (Platform Update) in order to run. It is a good idea to install those updates.  
**答**：没有任何更新的 Windows 7 用户仍可使用它，但许多程序需要安装 Service Pack 1、KB2533623（DllDirectories 更新）和 KB2670838（平台更新）才能运行。最好安装这些更新。

**Q: If I have ESUs (Extended Security Updates) installed, can I use VxKex NEXT?**  
**问：如果我安装了 ESU（扩展安全更新），我可以使用 VxKex NEXT 吗？**

**A**: Yes. There is no problem with ESUs.  
**答**：是的，ESU 没有问题。

**Q: Can I use this with Windows 8 or 8.1?**  
**问：可以在 Windows 8 或 8.1 中使用吗？**  

**A**: Currently, VxKex NEXT is designed for use only with Windows 7. If you use Windows 8 or 8.1, VxKex NEXT will do nothing useful.  
**答**：目前，VxKex NEXT 只适用于 Windows 7。如果您使用的是 Windows 8 或 8.1，VxKex NEXT 将毫无用处。

**Q: Can I remove VxKex or VxKex NEXT after upgrading to Windows 8/8.1/10/11?**  
**问：升级到 Windows 8/8.1/10/11 后可以删除 VxKex 或 VxKex NEXT 吗？**

**A**: Yes. If VxKex is installed, update it to VxKex NEXT, then uninstall it from control panel.  
**答**：可以。如果 VxKex 已安装，请将其更新为 VxKex NEXT，然后从控制面板卸载它。

**Q: How does VxKex NEXT work?**  
**问：VxKex NEXT 如何运作？**

**A**: VxKex NEXT works by loading a DLL into each program where VxKex NEXT is enabled. This is accomplished through using the IFEO (Image File Execution Options) registry key.  
**答**：VxKex NEXT 的工作原理是在启用 VxKex NEXT 的每个程序中加载一个 DLL。这是通过使用 IFEO（图像文件执行选项）注册表键来实现的。

Specifically, the "VerifierDlls" value is set to point to a VxKex NEXT DLL. This DLL then loads into the process.  
具体来说，“VerifierDlls”值被设置为指向 VxKex NEXT DLL。该 DLL 会加载到进程中。

API extension is accomplished by editing the program's DLL import table so that instead of importing from Windows 8/8.1/10/11 DLLs, it imports VxKex NEXT DLLs instead. These VxKex NEXT DLLs contain implementations of Windows API functions which were introduced in newer versions of Windows.  
API 扩展是通过编辑程序的动态链接库导入表来实现的，这样程序就不会从 Windows 8/8.1/10/11 动态链接库中导入，而是导入 VxKex NEXT 动态链接库。这些 VxKex NEXT 动态链接库包含较新版本 Windows 中引入的 Windows API 函数的实现。

Donations  捐赠
=========

If you would like to support development, consider making a donation.  
如果您想支持开发，请考虑捐款。
- USDT address: TLZWKpYaH3XK5KYBVWVKQkXbSHUt2XnWJr
- Alipay 支付宝 / WeChat Pay 微信支付
![Scan the QR codes and donate](/donation.png)
