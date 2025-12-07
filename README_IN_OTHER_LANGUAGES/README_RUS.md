# Введение

VxKex NEXT — это набор расширений API для Windows 7, которые позволяют запускать некоторые приложения, доступные только в Windows 8, 8.1 и 10, в Windows 7.

Для загрузки и установки см. страницу выпусков ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Перед установкой рекомендуется выполнить следующие операции.**

- **Удалить**
  
  - **0patch Agent**  
    Это может привести к сбою браузеров на базе Chromium и JetBrains IDE после включения VxKex NEXT и его запуска.

- **Обновление**
  
  - **MacType → 2025.6.9+**  
    Старая версия MacType может привести к сбою запуска всех программ после включения VxKex NEXT.

После установки использование очень простое. Вот способы включения VxKex NEXT:

1. Просто щелкните правой кнопкой мыши на программе, откройте диалоговое окно «Свойства» и выберите вкладку «VxKex». Затем установите флажок «Включить VxKex NEXT для этой программы» и попробуйте запустить программу.
2. Найдите «VxKex NEXT Global Settings» в меню «Пуск» и откройте его, нажмите кнопку «Добавить», выберите программу, нажмите кнопку «Открыть» и попробуйте запустить программу.

![VxKex configuration GUI](/example-screenshot.png)

Некоторые программы требуют дополнительной настройки. В папке установки VxKex NEXT (по умолчанию C:\Program Files\VxKex) находится файл «**Application Compatibility List.docx**», в котором подробно описаны эти шаги, но в большинстве случаев все настройки понятны без дополнительных пояснений.

Если вы являетесь разработчиком, исходный код предоставляется в виде файла 7z на странице выпусков.

Часто задаваемые вопросы
===

**В: Какие приложения поддерживаются?**

**О**: Список совместимых приложений включает, но не ограничивается:

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

Большинство приложений Qt6 будут работать, а также многие приложения Electron.

**В: VxKex NEXT изменяет системные файлы? Это приведет к нестабильной работе моей системы?**

**О**: VxKex NEXT не изменяет никакие системные файлы. Его влияние на всю систему крайне минимально. Не используются фоновые службы, не устанавливаются глобальные хуки, а загружаемые расширения оболочки и DLL-библиотеки оказывают минимальное влияние и при необходимости могут быть отключены. Вы можете быть уверены, что ваша Windows 7 останется такой же стабильной, как и всегда.

**В: Нужно ли устанавливать определенные обновления?**

**О**: Пользователи Windows 7 без каких-либо обновлений по-прежнему могут использовать эту систему, но для работы многих программ требуется Service Pack 1, KB2533623 (обновление DllDirectories) и KB2670838 (обновление платформы). Рекомендуется установить эти обновления.

**В: Если у меня установлены ESU (расширенные обновления безопасности), могу ли я использовать VxKex NEXT?**

**О**: Да. С ESU нет никаких проблем.

**В: Можно ли использовать это с Windows 8 или 8.1?**

**О**: В настоящее время VxKex NEXT официально поддерживает только Windows 7. Однако для некоторых программ VxKex NEXT работает в Windows 8 и 8.1. Мы планируем обеспечить официальную поддержку Windows 8 и 8.1.

**В: Могу ли я удалить VxKex или VxKex NEXT после обновления до Windows 8/8.1/10/11?**

**О**: Да. Если VxKex установлен, обновите его до VxKex NEXT, а затем удалите из панели управления.

**В: Как работает VxKex NEXT?**

**О**: VxKex NEXT работает путем загрузки DLL в каждую программу, в которой включен VxKex NEXT. Это достигается с помощью ключа реестра IFEO (Image File Execution Options).

В частности, значение «VerifierDlls» устанавливается так, чтобы указывать на DLL VxKex NEXT. Затем эта DLL загружается в процесс.

Расширение API осуществляется путем редактирования таблицы импорта DLL программы, чтобы вместо импорта из DLL Windows 8/8.1/10/11 импортировались DLL VxKex NEXT. Эти DLL VxKex NEXT содержат реализации функций Windows API, которые были введены в более новых версиях Windows.

Пожертвования
=========

Если вы хотите поддержать развитие, рассмотрите возможность сделать пожертвование.

- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付
  ![Scan the QR codes and donate](/donation.png)
