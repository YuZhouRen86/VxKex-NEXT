Úvod
============

VxKex NEXT je sada rozšíření API pro Windows, která umožňuje spouštět některé nové programy určené výhradně pro novější verze Windows na starších verzích.

Pro stažení a instalaci navštivte stránku vydání ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Před instalací se doporučuje provést následující kroky.**

- **Odstraňte**  
  - **0patch Agent** – může způsobovat pády prohlížečů založených na Chromiu a IDE JetBrains po zapnutí VxKex NEXT a jeho spuštění.

- **Aktualizujte**  
  - **MacType → verze 2025.6.9 nebo novější** – stará verze MacType může způsobit, že po zapnutí VxKex NEXT nepůjde spustit žádný program.

Po instalaci je použití jednoduché: klepněte pravým tlačítkem na program, otevřete dialog „Vlastnosti“, vyberte kartu „VxKex“. Poté zaškrtněte políčko „Povolit VxKex NEXT pro tento program“ a zkuste program spustit.

![VxKex configuration GUI](/example-screenshot.png)

Některé programy vyžadují dodatečnou konfiguraci. V instalační složce VxKex NEXT (ve výchozím nastavení `C:\Program Files\VxKex`) se nachází soubor „**Application Compatibility List.docx**“, který podrobně popisuje tyto kroky, ale ve většině případů je konfigurace intuitivní.

Pokud jste vývojář, zdrojový kód je poskytován jako 7z archiv na stránce vydání.

Často kladené otázky
===

**Otázka: Které aplikace jsou podporovány?**

**Odpověď:** Seznam kompatibilních aplikací zahrnuje, ale není omezen na:

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

Další podrobnosti naleznete v souboru **Application Compatibility List.docx**, který se instaluje spolu s VxKex NEXT.

Většina aplikací na Qt6 a mnoho aplikací na Electron budou fungovat.

**Otázka: Mění VxKex NEXT systémové soubory? Způsobí to nestabilitu systému?**

**Odpověď:** VxKex NEXT nemění žádné systémové soubory. Jeho vliv na systém je minimální. Nepoužívají se žádné služby na pozadí, neinstalují se žádné globální háky a načítaná rozšíření prostředí a DLL knihovny mají minimální dopad a mohou být v případě potřeby zakázány. Můžete si být jisti, že vaše Windows zůstanou stejně stabilní jako vždy.

**Otázka: Je nutné nainstalovat konkrétní aktualizace?**

**Odpověď:** Uživatelé Windows 7 bez jakýchkoli aktualizací mohou VxKex NEXT stále používat, ale pro fungování mnoha programů je vyžadován Service Pack 1, KB2533623 (aktualizace DllDirectories) a KB2670838 (aktualizace platformy). Doporučuje se tyto aktualizace nainstalovat.

**Otázka: Pokud mám nainstalovány ESU (rozšířené bezpečnostní aktualizace), mohu používat VxKex NEXT?**

**Odpověď:** Ano, s ESU nejsou žádné problémy.

**Otázka: Které verze Windows VxKex NEXT podporuje?**

**Odpověď:** V současné době VxKex NEXT podporuje Windows 7, 8 a 8.1.

**Otázka: Lze VxKex nebo VxKex NEXT odinstalovat po upgradu Windows?**

**Odpověď:** Ano. Pokud je VxKex nainstalován, aktualizujte jej na VxKex NEXT a poté jej odinstalujte přes Ovládací panely.

**Otázka: Jak VxKex NEXT funguje?**

**Odpověď:** VxKex NEXT načítá DLL do každého programu, pro který je zapnutý. Toho je dosaženo pomocí klíče registru IFEO (Image File Execution Options). Konkrétně hodnota „VerifierDlls“ ukazuje na DLL VxKex NEXT, která se poté načte do procesu. Rozšíření API se provádí úpravou importní tabulky DLL programu tak, aby místo importu z novějších DLL Windows používal DLL VxKex NEXT, které obsahují implementace funkcí API zavedených v nových verzích Windows.

Příspěvky
=========

Pokud chcete podpořit vývoj, zvažte možnost přispět.

- PayPal : [paypal.me/YZR2024](https://paypal.me/YZR2024)
- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付  
  ![Scan the QR codes and donate](/donation.png)