Введение
============

VxKex NEXT — это набор расширений API для Windows, который позволяет запускать некоторые новые приложения, предназначенные исключительно для новых версий Windows, на более старых версиях.

Для загрузки и установки см. страницу релизов ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Перед установкой рекомендуется выполнить следующие действия.**

- **Удалите**  
  - **0patch Agent** – может вызывать сбои браузеров на базе Chromium и IDE JetBrains после включения VxKex NEXT и его запуска.

- **Обновите**  
  - **MacType → версия 2025.6.9 или новее** – старая версия MacType может привести к невозможности запуска всех программ после включения VxKex NEXT.

После установки использование простое: щёлкните правой кнопкой мыши по программе, откройте диалог «Свойства», выберите вкладку «VxKex». Затем установите флажок «Включить VxKex NEXT для этой программы» и попробуйте запустить программу.

![VxKex configuration GUI](/example-screenshot.png)

Некоторые программы требуют дополнительной настройки. В папке установки VxKex NEXT (по умолчанию `C:\Program Files\VxKex`) находится файл «**Application Compatibility List.docx**», в котором подробно описаны эти шаги, но в большинстве случаев настройка интуитивно понятна.

Если вы разработчик, исходный код предоставляется в виде 7z-архива на странице релизов.

Часто задаваемые вопросы
===

**Вопрос: Какие приложения поддерживаются?**

**Ответ:** Список совместимых приложений включает, но не ограничивается:

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

Дополнительные сведения см. в файле **Application Compatibility List.docx**, который устанавливается вместе с VxKex NEXT.

Большинство приложений на Qt6 и многие приложения на Electron будут работать.

**Вопрос: VxKex NEXT изменяет системные файлы? Это приведёт к нестабильности системы?**

**Ответ:** VxKex NEXT не изменяет никакие системные файлы. Его влияние на систему минимально. Не используются фоновые службы, не устанавливаются глобальные перехватчики, а загружаемые расширения оболочки и DLL-библиотеки оказывают минимальное воздействие и могут быть отключены при необходимости. Вы можете быть уверены, что ваша Windows останется такой же стабильной, как и всегда.

**Вопрос: Нужно ли устанавливать определённые обновления?**

**Ответ:** Пользователи Windows 7 без каких-либо обновлений всё равно могут использовать VxKex NEXT, но для работы многих программ требуется Service Pack 1, KB2533623 (обновление DllDirectories) и KB2670838 (обновление платформы). Рекомендуется установить эти обновления.

**Вопрос: Если у меня установлены ESU (расширенные обновления безопасности), могу ли я использовать VxKex NEXT?**

**Ответ:** Да, с ESU нет никаких проблем.

**Вопрос: Какие версии Windows поддерживает VxKex NEXT?**

**Ответ:** В настоящее время VxKex NEXT поддерживает Windows 7, 8 и 8.1.

**Вопрос: Можно ли удалить VxKex или VxKex NEXT после обновления Windows?**

**Ответ:** Да. Если VxKex установлен, обновите его до VxKex NEXT, затем удалите через панель управления.

**Вопрос: Как работает VxKex NEXT?**

**Ответ:** VxKex NEXT загружает DLL в каждую программу, для которой он включён. Это достигается с помощью ключа реестра IFEO (Image File Execution Options). Конкретно значение «VerifierDlls» указывает на DLL VxKex NEXT, которая затем загружается в процесс. Расширение API выполняется путём редактирования таблицы импорта DLL программы, чтобы вместо импорта из более новых DLL Windows использовались DLL VxKex NEXT, содержащие реализации функций API, введённых в новых версиях Windows.

Пожертвования
=========

Если вы хотите поддержать разработку, рассмотрите возможность пожертвования.

- PayPal : [paypal.me/YZR2024](https://paypal.me/YZR2024)
- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付  
  ![Scan the QR codes and donate](/donation.png)