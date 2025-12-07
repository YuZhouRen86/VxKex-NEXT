介紹
============

VxKex NEXT 是一套適用於 Windows 7 的 API 擴展，可讓一些 Windows 8、8.1 和 10 獨佔應用程式在 Windows 7 上執行。

如需下載和安裝，請參閱發行版頁面（[GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)）。

**安裝前，建議執行以下操作。**

- **解除安裝**  
  
  - **0patch Agent**  
    它可能導致基於 Chromium 的瀏覽器和 JetBrains IDE 在啟用 VxKex NEXT 並執行後崩潰。

- **更新**  
  
  - **MacType → 2025.6.9+**  
    舊版 MacType 可能導致所有程式在啟用 VxKex NEXT 後無法啟動。

安裝後，使用很簡單。以下是啟用 VxKex NEXT 的方法：

1. 右鍵單擊程式，打開屬性對話框，選擇「VxKex」選項卡。然後，選中「為此程式啟用 VxKex NEXT」複選框，並嘗試執行程式。
2. 從開始功能表中找到「VxKex NEXT Global Settings」並打開，點擊「添加」按鈕，選擇程式，點擊「打開」按鈕，並嘗試執行程式。

![VxKex configuration GUI](/example-screenshot.png)

有些程式需要額外配置。VxKex NEXT 安裝資料夾（預設為 C:\Program Files\VxKex 資料夾）內有一個名為「**Application Compatibility List.docx**」的檔案，其中詳細說明了這些步驟，但大多數情況下，所有配置都是不言自明的。

如果您是開發人員，原始碼將以 7z 檔案的形式在發行版頁面上提供。

常見問題
===

**問：哪些應用程式受支持？**

**答**：兼容的應用程式包括但不限於：

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

如需獲取更多信息，請參閱與 VxKex NEXT 一起安裝的**Application Compatibility List.docx**檔案。

大多數 Qt6 應用程式都能正常執行，許多 Electron 應用程式也能正常執行。

**問：VxKex NEXT 會修改系統檔案嗎？它會使我的系統不穩定嗎？**

**答**：VxKex NEXT 不會修改任何系統檔案。它對整個系統的影響極小。不使用後臺服務，不安裝全局鉤子，加載的 shell 擴展和 DLL 影響也很小，可以隨時禁用。您可以放心，您的 Windows 7 將一如既往地保持穩定。

**問：我需要安裝特定的更新嗎？**

**答**：沒有任何更新的 Windows 7 用戶仍可使用它，但許多程式需要安裝 Service Pack 1、KB2533623（DllDirectories 更新）和 KB2670838（平臺更新）才能執行。最好安裝這些更新。

**問：如果我安裝了 ESU（擴展安全更新），我可以使用 VxKex NEXT 嗎？**

**答**：是的，ESU 沒有問題。

**問：可以在 Windows 8 或 8.1 中使用嗎？**  

**答**：目前，VxKex NEXT 僅為 Windows 7 提供官方支持。但是，對於某些程式，VxKex NEXT 在 Windows 8 和 8.1 上起作用。我們計劃為 Windows 8 和 8.1 提供官方支持。

**問：升級到 Windows 8/8.1/10/11 後可以刪除 VxKex 或 VxKex NEXT 嗎？**

**答**：可以。如果 VxKex 已安裝，請將其更新為 VxKex NEXT，然後從控制面板卸載它。

**問：VxKex NEXT 如何運作？**

**答**：VxKex NEXT 的工作原理是在啟用 VxKex NEXT 的每個程式中加載一個 DLL。這是通過使用 IFEO（圖像檔案執行選項）登錄檔鍵來實現的。

具體來說，「VerifierDlls」值被設置為指向 VxKex NEXT DLL。該 DLL 會加載到進程中。

API 擴展是通過編輯程式的動態連結庫導入表來實現的，這樣程式就不會從 Windows 8/8.1/10/11 動態連結庫中導入，而是導入 VxKex NEXT 動態連結庫。這些 VxKex NEXT 動態連結庫包含較新版本 Windows 中引入的 Windows API 函數的實現。

捐贈
=========

如果您想支持開發，請考慮捐款。

- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付寶 / WeChat Pay 微信支付
  ![Scan the QR codes and donate](/donation.png)
