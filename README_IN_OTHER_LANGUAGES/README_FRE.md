# [GitHub](https://github.com/YuZhouRen86/VxKex-NEXT) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT)

Présentation
============

VxKex NEXT est un ensemble d'extensions pour l'API de Windows 7 qui permettent de faire fonctioner des programmes exclusivement conçus pour Windows 8, 8.1 et Windows 10 sur Windows 7.\
Pour le télécharger et l'installer, rendez-vous sur la page des `releases` (binaires compilées) ([GitHub](https://github.com/YuZhouRen86/VxKex-NEXT/releases) | [Gitee](https://gitee.com/YuZhouRen86/VxKex-NEXT/releases)).

**Il est fortement recommandé d'effectuer les démarches suivantes avant l'installation.**

- **Veuillez désinstaller:**
  - **L'agent 0patch**
    Il peut entraîner le plantage des navigateurs Web basés sur Chromium ainsi que des `IDE` JetBrains lorsque VxKex NEXT est activé pour ces programmes.

- **Veuillez mettre à jour:**
  - **MacType vers la version 2025.6.9 ou ultérieure**
    Les anciennes versions de MacType antérieures à la **2025.6.9** peuvent entraîner le plantage de tous les programmes après l'activation de VxKex NEXT.

Une fois installé, l'utilisation de VxKex NEXT est très simple. Voici comment activer VxKex NEXT:

1. Faites un clic-droit sur le programme, sélectionnez **Propriétés**, et consultez l'onglet **VxKex**. Ensuite cliquez simplement sur la case `Enable VxKex NEXT for this program` puis relancez-le.
2. Recherchez le raccourci `VxKex NEXT Global Settings` dans le menu démarrer puis lancez-le, cliquez sur le bouton `Add` et sélectionez le programme (bouton "Ouvrir"). Relancez ensuite le programme.

![Interface graphique de configuration pour VxKex](/example-screenshot.png)

Certains programmes nécessitent des modifications supplémentaires. Vous trouverez un document nommé `Application Compatibility List.docx` dans le dossier d'installation de VxKex NEXT (par défaut `C:\Program Files\VxKex`) qui explique en détail ce qu'il faut faire, mais pour la plupart des programmes la configuration est simple et intuitive.\
Pour les développeurs, vous pourrez trouver le code-source dans l'archive **7z** sur la page des `releases` (page de téléchargement des binaires compilées).

FAQ (foire aux questions)
=========================

**Quest.: Quels-sont les programmes compatibles?**

**Rép.**: La liste des programmes compatibles inclue les programmes suivants (voire-même plus que ceux-là):
- Bespoke Synth
- Blender
- Blockbench
- Calibre
- Chromium (inclue égalemet `Ungoogled Chromium`)
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
- WinDbg (version classique du SDK Windows 11 ainsi que la version `preview`)
- Yuzu (l'expérience en jeu n'a pas été vérifiée)
- Zig

Pensez à consulter le document `Application Compatibility List.docx`, qui est fourni avec VxKex NEXT lors de l'installation pour plus d'informations.
La plupart des applications Qt6 fonctionneront, ainsi que la plupart des applications Electron.

**Quest.: Est-ce que VxKex NEXT modifie des fichiers système? Est-ce qu'il peut rendre mon système instable?**

**Rép.**: VxKex NEXT ne modifie aucun fichier sur votre système. Son effet sur votre système est très minime. Il n'y a aucun service en arrière-plan, aucune interception (`hook`) n'est effectuée, et les fichiers DLL ainsi que l'extension Windows ont un effet minimal et peuvent être désactivés si nécessaire. Soyez donc rassurés, votre système Windows 7 restera stable comme il l'a toujours été.

**Quest.: Y-a t'il des mises à jour à installer en particulier?**

**Rép.**: Les utilisateurs de Windows 7 peuvent utiliser VxKex NEXT sans aucune mise à jour, néanmoins certains programmes requièrent le `Service Pack 1`, la mise à jour `KB2533623` (DllDirectories) ainsi que la `KB2670838` (Platform Update) pour fonctionner. Il est donc toujours utile d'installer ces mises à jour.

**Quest.: Est-ce que je peux utiliser VxKex NEXT avec les mises à jour de sécurité étendues (`ESU`)?**

**Rép.**: Oui, il n'y a aucun problème avec les mises à jour `ESU`.

**Quest.: Est-ce que je peux utiliser ce programme avec Windows 8 ou 8.1?**

**Rép.**: VxKex NEXT ne fonctionne actuellement de manière confirmée qu'avec Windows 7. Par contre, pour certains programmes VxKex NEXT peut fonctionner sur Windows 8 et 8.1. Nous avons prévu d'éventuellement ajouter la compatibilité avec Windows 8 et 8.1.

**Quest.: Est-ce que je peux désinstaller VxKex ou VxKex NEXT après la mise à niveau vers Windows 8/8.1/10/11?**

**Rép.**: Oui. Si VxKex est installé, mettez-le à jour vers VxKex NEXT, puis désinstallez-le à partir du panneau de configuration.

**Quest.: Comment fonctionne VxKext NEXT?**

**Rép.**: VxKex NEXT fonctionne en injectant des fichiers DLL dans chaque programme pour lequel VxKex NEXT est activé. Cela se fait par l'utilisation d'une clé de registre `IFEO` (`Image File Execution Options`).

Plus précisément, c'est la valeur `VerifierDlls` qui est modifiée pour pointer vers la DLL de VxKex NEXT. Cette DLL est alors chargée en même temps que le programme.\
L'extension des API de Windows se fait en modifiant l'`import table` du programme pour qu'il charge les DLLs de VxKex NEXT au lieu des DLLs d'origine de Windows 8/8.1/10/11. Ces DLLs de VxKex NEXT sont une réimplementation des API Windows qui ont été introduites dans des versions plus récentes de Windows.

Donations
=========

Si vous souhaitez contribuer pour soutenir le développement de VxKex NEXT, vous pouvez envisager de faire un don.

- ERC20 (ETC/USDT) : 0xaF1AfBDE5F226FB229267D8591D757C3E6E0e1A0
- Bitcoin (BTC/USDT) : 32XgoYcRVy3CTcga3DUBtua5QCToRtS78G
- Cosmos (ATOM) : cosmos1fs2twk3du55gz3cllwm76cey5rrtnu2v5gcrmr
- TRC10/TRC20 (TRX/USDT) : TEyobAt82WMJN2sXvRTKNrXPf3sVHE2KQT
- Alipay 支付宝 / WeChat Pay 微信支付
  ![Scannez les codes QR pour faire un don](/donation.png)
