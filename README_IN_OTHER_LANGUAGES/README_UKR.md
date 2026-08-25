Вступ
============

VxKex NEXT — це набір розширень API для Windows, який дозволяє запускати деякі нові програми, призначені виключно для нових версій Windows, на старіших версіях.

Для завантаження та встановлення див. сторінку релізів ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Перед встановленням рекомендується виконати наступні дії.**

- **Видаліть**  
  - **0patch Agent** – може спричиняти збої браузерів на основі Chromium та IDE JetBrains після ввімкнення VxKex NEXT і його запуску.

- **Оновіть**  
  - **MacType → версія 2025.6.9 або новіша** – стара версія MacType може призвести до неможливості запуску всіх програм після ввімкнення VxKex NEXT.

Після встановлення використання просте: клацніть правою кнопкою миші по програмі, відкрийте діалог «Властивості», виберіть вкладку «VxKex». Потім встановіть прапорець «Увімкнути VxKex NEXT для цієї програми» і спробуйте запустити програму.

![VxKex configuration GUI](/example-screenshot.png)

Деякі програми потребують додаткового налаштування. У папці встановлення VxKex NEXT (за замовчуванням `C:\Program Files\VxKex`) знаходиться файл «**Application Compatibility List.docx**», у якому детально описані ці кроки, але в більшості випадків налаштування інтуїтивно зрозумілі.

Якщо ви розробник, вихідний код надається у вигляді 7z-архіву на сторінці релізів.

Часті запитання
===

**Запитання: Які програми підтримуються?**

**Відповідь:** Список сумісних програм включає, але не обмежується:

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

Додаткову інформацію див. у файлі **Application Compatibility List.docx**, який встановлюється разом із VxKex NEXT.

Більшість програм на Qt6 і багато програм на Electron працюватимуть.

**Запитання: VxKex NEXT змінює системні файли? Це призведе до нестабільності системи?**

**Відповідь:** VxKex NEXT не змінює жодних системних файлів. Його вплив на систему мінімальний. Не використовуються фонові служби, не встановлюються глобальні перехоплювачі, а завантажувані розширення оболонки та DLL-бібліотеки мають мінімальний вплив і можуть бути вимкнені за потреби. Ви можете бути впевнені, що ваша Windows залишиться такою ж стабільною, як і завжди.

**Запитання: Чи потрібно встановлювати певні оновлення?**

**Відповідь:** Користувачі Windows 7 без будь-яких оновлень все одно можуть використовувати VxKex NEXT, але для роботи багатьох програм потрібно Service Pack 1, KB2533623 (оновлення DllDirectories) і KB2670838 (оновлення платформи). Рекомендується встановити ці оновлення.

**Запитання: Якщо в мене встановлені ESU (розширені оновлення безпеки), чи можу я використовувати VxKex NEXT?**

**Відповідь:** Так, з ESU немає жодних проблем.

**Запитання: Які версії Windows підтримує VxKex NEXT?**

**Відповідь:** Наразі VxKex NEXT підтримує Windows 7, 8 і 8.1.

**Запитання: Чи можна видалити VxKex або VxKex NEXT після оновлення Windows?**

**Відповідь:** Так. Якщо VxKex встановлено, оновіть його до VxKex NEXT, потім видаліть через панель керування.

**Запитання: Як працює VxKex NEXT?**

**Відповідь:** VxKex NEXT завантажує DLL у кожну програму, для якої його ввімкнено. Це досягається за допомогою ключа реєстру IFEO (Image File Execution Options). Конкретно значення «VerifierDlls» вказує на DLL VxKex NEXT, яка потім завантажується в процес. Розширення API виконується шляхом редагування таблиці імпорту DLL програми, щоб замість імпорту з новіших DLL Windows використовувалися DLL VxKex NEXT, які містять реалізації функцій API, введених у нових версіях Windows.

Пожертви
=========

Якщо ви хочете підтримати розробку, розгляньте можливість зробити пожертву.

- PayPal : [paypal.me/YZR2024](https://paypal.me/YZR2024)
- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付  
  ![Scan the QR codes and donate](/donation.png)