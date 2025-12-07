# Вступ

VxKex NEXT — це набір розширень API для Windows 7, які дозволяють запускати деякі програми, доступні тільки в Windows 8, 8.1 і 10, в Windows 7.

Для завантаження та встановлення див. сторінку випусків ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Перед установкою рекомендується виконати наступні операції.**

- **Видалити**
  
  - **0patch Agent**  
    Це може призвести до збою браузерів на базі Chromium і JetBrains IDE після увімкнення VxKex NEXT і його запуску.

- **Оновлення**
  
  - **MacType → 2025.6.9+**  
    Стара версія MacType може призвести до збою запуску всіх програм після увімкнення VxKex NEXT.

Після встановлення використання дуже просте. Ось способи увімкнення VxKex NEXT:

1. Просто клацніть правою кнопкою миші на програмі, відкрийте діалогове вікно «Властивості» і виберіть вкладку «VxKex». Потім встановіть прапорець «Увімкнути VxKex NEXT для цієї програми» і спробуйте запустити програму.
2. Знайдіть «VxKex NEXT Global Settings» в меню «Пуск» і відкрийте його, натисніть кнопку «Додати», виберіть програму, натисніть кнопку «Відкрити» і спробуйте запустити програму.

![VxKex configuration GUI](/example-screenshot.png)

Деякі програми вимагають додаткового налаштування. У папці інсталяції VxKex NEXT (за замовчуванням C:\Program Files\VxKex) знаходиться файл «**Application Compatibility List.docx**», в якому детально описані ці кроки, але в більшості випадків всі налаштування зрозумілі без додаткових пояснень.

Якщо ви є розробником, вихідний код надається у вигляді файлу 7z на сторінці випусків.

Часто задавані питання
===

**Питання: Які програми підтримуються?**

**Відповідь**: Список сумісних програм включає, але не обмежується:

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

Більшість додатків Qt6 працюватимуть, а також багато додатків Electron.

**Питання: VxKex NEXT змінює системні файли? Чи призведе це до нестабільної роботи моєї системи?**

**Відповідь**: VxKex NEXT не змінює жодних системних файлів. Його вплив на всю систему є мінімальним. Не використовуються фонові служби, не встановлюються глобальні хуки, а завантажувані розширення оболонки та DLL-бібліотеки мають мінімальний вплив і при необхідності можуть бути відключені. Ви можете бути впевнені, що ваша Windows 7 залишиться такою ж стабільною, як і завжди.

**Питання: Чи потрібно встановлювати певні оновлення?**

**Відповідь**: Користувачі Windows 7 без будь-яких оновлень як і раніше можуть використовувати цю систему, але для роботи багатьох програм потрібно Service Pack 1, KB2533623 (оновлення DllDirectories) і KB2670838 (оновлення платформи). Рекомендується встановити ці оновлення.

**Питання: Якщо у мене встановлені ESU (розширені оновлення безпеки), чи можу я використовувати VxKex NEXT?**

**Відповідь**: Так. З ESU немає жодних проблем.

**Питання: Чи можна використовувати це з Windows 8 або 8.1?**

**Відповідь**: Наразі VxKex NEXT офіційно підтримує тільки Windows 7. Однак для деяких програм VxKex NEXT працює в Windows 8 і 8.1. Ми плануємо забезпечити офіційну підтримку Windows 8 і 8.1.

**Питання: Чи можу я видалити VxKex або VxKex NEXT після оновлення до Windows 8/8.1/10/11?**

**Відповідь**: Так. Якщо VxKex встановлено, оновіть його до VxKex NEXT, а потім видаліть з панелі керування.

**Питання: Як працює VxKex NEXT?**

**Відповідь**: VxKex NEXT працює шляхом завантаження DLL в кожну програму, в якій включений VxKex NEXT. Це досягається за допомогою ключа реєстру IFEO (Image File Execution Options).

Зокрема, значення «VerifierDlls» встановлюється так, щоб вказувати на DLL VxKex NEXT. Потім ця DLL завантажується в процес.

Розширення API здійснюється шляхом редагування таблиці імпорту DLL програми, щоб замість імпорту з DLL Windows 8/8.1/10/11 імпортувалися DLL VxKex NEXT. Ці DLL VxKex NEXT містять реалізації функцій Windows API, які були введені в новіших версіях Windows.

Пожертви
=========

Якщо ви хочете підтримати розвиток, розгляньте можливість зробити пожертву.

- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付
  ![Scan the QR codes and donate](/donation.png)
