# Úvod

VxKex NEXT je sada rozšíření API pro Windows 7, která umožňují spouštět některé aplikace dostupné pouze ve Windows 8, 8.1 a 10 na Windows 7.

Ke stažení a instalaci přejděte na stránku vydání ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Před instalací se doporučuje provést následující kroky.**

- **Odstranit**
  
  - **0patch Agent**  
    To může způsobit selhání prohlížečů založených na Chromiu a IDE JetBrains po zapnutí a spuštění VxKex NEXT.

- **Aktualizovat**
  
  - **MacType → 2025.6.9+**  
    Starší verze MacType může způsobit selhání spuštění všech programů po zapnutí VxKex NEXT.

Po instalaci je použití velmi jednoduché. Zde jsou způsoby, jak zapnout VxKex NEXT:

1. Jednoduše klikněte pravým tlačítkem myši na program, otevřete dialogové okno „Vlastnosti“ a vyberte záložku „VxKex“. Poté zaškrtněte políčko „Povolit VxKex NEXT pro tento program“ a zkuste program spustit.
2. V nabídce Start vyhledejte „VxKex NEXT Global Settings“ a otevřete jej, klikněte na tlačítko „Přidat“, vyberte program, klikněte na tlačítko „Otevřít“ a zkuste program spustit.

![VxKex configuration GUI](/example-screenshot.png)

Některé programy vyžadují dodatečnou konfiguraci. V instalační složce VxKex NEXT (standardně C:\Program Files\VxKex) se nachází soubor „**Application Compatibility List.docx**“, který tyto kroky podrobně popisuje, ale ve většině případů je konfigurace srozumitelná bez dalších vysvětlení.

Pokud jste vývojář, zdrojový kód je poskytován jako soubor 7z na stránce vydání.

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

Většina aplikací Qt6 bude fungovat, stejně jako mnoho Electron aplikací.

**Otázka: Mění VxKex NEXT systémové soubory? Způsobí to nestabilitu mého systému?**

**Odpověď:** VxKex NEXT nemění žádné systémové soubory. Jeho vliv na celý systém je extrémně minimální. Nepoužívají se žádné služby na pozadí, neinstalují se žádné globální hooky a načítaná rozšíření shellu a DLL knihovny mají minimální dopad a mohou být v případě potřeby deaktivovány. Můžete si být jisti, že vaše Windows 7 zůstanou stejně stabilní jako vždy.

**Otázka: Je potřeba nainstalovat konkrétní aktualizace?**

**Odpověď:** Uživatelé Windows 7 bez jakýchkoli aktualizací stále mohou tento systém používat, ale pro fungování mnoha programů je vyžadován Service Pack 1, KB2533623 (aktualizace DllDirectories) a KB2670838 (aktualizace platformy). Doporučuje se tyto aktualizace nainstalovat.

**Otázka: Pokud mám nainstalovány ESU (rozšířené bezpečnostní aktualizace), mohu používat VxKex NEXT?**

**Odpověď:** Ano. S ESU nejsou žádné problémy.

**Otázka: Lze to používat s Windows 8 nebo 8.1?**

**Odpověď:** V současné době VxKex NEXT oficiálně podporuje pouze Windows 7. Pro některé programy však VxKex NEXT funguje i ve Windows 8 a 8.1. Plánujeme poskytnout oficiální podporu pro Windows 8 a 8.1.

**Otázka: Mohu VxKex nebo VxKex NEXT odinstalovat po upgradu na Windows 8/8.1/10/11?**

**Odpověď:** Ano. Pokud je VxKex nainstalován, aktualizujte jej na VxKex NEXT a poté odinstalujte z Ovládacích panelů.

**Otázka: Jak VxKex NEXT funguje?**

**Odpověď:** VxKex NEXT funguje tak, že načítá DLL do každého programu, kde je VxKex NEXT povolen. Toho je dosaženo pomocí klíče registru IFEO (Image File Execution Options).

Konkrétně je nastavena hodnota „VerifierDlls“, aby odkazovala na DLL VxKex NEXT. Tato DLL je poté načtena do procesu.

Rozšíření API je provedeno úpravou importní tabulky DLL programu, aby se místo importu z DLL Windows 8/8.1/10/11 importovaly DLL VxKex NEXT. Tyto DLL VxKex NEXT obsahují implementace funkcí Windows API, které byly zavedeny v novějších verzích Windows.

Příspěvky
=========

Pokud chcete podpořit vývoj, zvažte možnost přispět.

- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付
  ![Scan the QR codes and donate](/donation.png)