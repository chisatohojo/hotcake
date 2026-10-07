# 新しいWindows PCの構築手順

Windows PowerShell 5.1以降を使用します。現在PCと同じWindows x64を基本とします。
Git、VS Code、Codexのログイン・OS承認は人間が実施します。
コア・ライブラリ導入とcompileはスクリプトで自動化できます。compileまでは実機不要です。
ダウンロードとパッケージ導入にはインターネット接続が必要です。

2026-10-07の引き継ぎから、setup/verifyは `.arduino-local/` の専用環境を自動で使用します。
ローカルの `.tools/run.ps1 -Action Verify` にあった環境設定は `scripts/common.ps1` へ移植済みです。
次PCへ `.tools/` や `.arduino-local/` のキャッシュをコピーする必要はありません。

## 1. Gitをインストール

[Git for Windows公式](https://git-scm.com/install/windows)からインストール。
PowerShellから使えるPATH設定とGit Credential Managerを有効にします。
インストーラのOS承認を手動で行い、PowerShellを開き直します。

```powershell
git --version
```

## 2. VS Codeをインストール

[VS Code Windows公式手順](https://code.visualstudio.com/docs/setup/windows)に従い、
User Installerで「Add to PATH」を有効にします。PowerShellを開き直して確認します。

```powershell
code --version
```

Git/VS Codeは新PCの対応版を導入できます。旧PCの観測版は[ENVIRONMENT](ENVIRONMENT.md)に記録。

## 3. Codexを利用可能にする

旧PCで確認した正確な拡張IDを使用します。

```powershell
code --install-extension openai.chatgpt
```

VS Codeを再起動し、Codexサイドバーを開き、自分のアカウントでログインします。
ログイン・契約・組織アクセスの確認は人間が実施します。
操作は[公式Codex IDE手順](https://learn.chatgpt.com/docs/codex/ide)を参照。
既に別のCodexクライアントを利用できる場合はcloneしたフォルダをそこで開いても構いません。

## 4. GitHub認証

リポジトリへアクセスできる自分のGitHubアカウントを使用します。
GitHub CLIを[公式配布](https://cli.github.com/)から導入し、PowerShellを開き直して実行。

```powershell
gh auth login --hostname github.com --git-protocol https --web
gh auth setup-git
gh auth status
```

ブラウザ認証は人間が完了させます。トークンをこのリポジトリへ保存しないでください。
Git Credential Managerで認証済みならその認証も利用できます。
公開リポジトリのcloneだけなら認証は不要ですが、非公開化や将来のpushには認証が必要です。
詳細: [gh auth login](https://cli.github.com/manual/gh_auth_login)、
[gh auth setup-git](https://cli.github.com/manual/gh_auth_setup-git)。

commitする利用者は自身のGit author情報も設定します。

```powershell
$gitAuthorName = Read-Host 'Git author name'
$gitAuthorEmail = Read-Host 'Git author email (GitHub noreply address is also supported)'
git config --global user.name $gitAuthorName
git config --global user.email $gitAuthorEmail
```

## 5. cloneしてフォルダを開く

自分の開発用保存先へ移動してから実行します。

```powershell
git clone https://github.com/chisatohojo/hotcake.git
cd hotcake
git status
git remote -v
code .
```

以降は `hotcake` のルートで実行。既存の別フォルダは上書きしません。
Codexには[HANDOFFの最初の指示](HANDOFF.md#再開方法)を渡します。

## 6. VS Code推奨拡張を導入

正本は [.vscode/extensions.json](../.vscode/extensions.json) です。
ワークスペース推奨拡張を導入するか、次を実行します。

```powershell
code --install-extension ms-vscode.cpptools
code --install-extension openai.chatgpt
code --install-extension ms-ceintl.vscode-language-pack-ja
code --list-extensions --show-versions
```

C/C++、Codex、日本語UI（任意）を推奨。拡張は旧PCの版に固定しません。
PlatformIOや旧Arduino拡張はビルドに不要です。
手動導入を省略したい場合、手順8の `-InstallExtensions` で同じ一覧を導入できます。

共有の `.vscode/tasks.json` は **Ctrl+Shift+B** で `scripts/verify_environment.ps1` を直接実行します。
`.vscode/settings.json` はワークスペース・環境変数を使ったターミナルとC/C++補完の設定です。
PC固有の絶対パスは含みません。フォルダをVS Codeで開き、ターミナルを新しく作成してください。
変数の記法は[VS Code公式の変数リファレンス](https://code.visualstudio.com/docs/reference/variables-reference)を参照。

## 7. Arduino CLI 1.5.1を導入

[公式インストール手順](https://arduino.github.io/arduino-cli/1.5/installation/)と
[v1.5.1公式リリース](https://github.com/arduino/arduino-cli/releases/tag/v1.5.1)を使用。
最新版へ置き換えず、旧PCで実測した1.5.1を導入します。

x64用ZIPは管理者権限なしでユーザー用フォルダへ配置できます。
公式リリースのZIPをダウンロードし、SHA-256を検査する例:

```powershell
$cliVersion = '1.5.1'
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$cliAsset = "arduino-cli_${cliVersion}_Windows_64bit.zip"
$cliFolder = Join-Path $env:LOCALAPPDATA "ArduinoCLI\$cliVersion"
$cliArchive = Join-Path $env:TEMP $cliAsset
$checksumPath = Join-Path $env:TEMP "$cliVersion-checksums.txt"
$releaseBase = "https://github.com/arduino/arduino-cli/releases/download/v$cliVersion"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
New-Item -ItemType Directory -Path $cliFolder -Force | Out-Null
Invoke-WebRequest "$releaseBase/$cliAsset" -OutFile $cliArchive -UseBasicParsing
Invoke-WebRequest "$releaseBase/$cliVersion-checksums.txt" -OutFile $checksumPath -UseBasicParsing
$checksums = Get-Content -LiteralPath $checksumPath -Encoding UTF8 -Raw
$checksumLine = @($checksums -split "`n" | Where-Object { $_.Trim().EndsWith($cliAsset) })
if ($checksumLine.Count -ne 1) { throw 'Official checksum entry missing or ambiguous' }
$expectedHash = ($checksumLine[0].Trim() -split '\s+')[0]
if ((Get-FileHash -LiteralPath $cliArchive -Algorithm SHA256).Hash -ne $expectedHash) {
    throw 'Arduino CLI archive checksum mismatch'
}
Expand-Archive -LiteralPath $cliArchive -DestinationPath $cliFolder -Force
$env:Path = "$cliFolder;$env:Path"
arduino-cli version
Get-Command arduino-cli
```

Windowsの「環境変数」で**ユーザーのPath**へ `$cliFolder` の実際の値を追加し、
PowerShellとVS Code/Codexを開き直します。`setx PATH` で既存Path全体を上書きしないでください。
32bit Windowsは同リリースの32bit ZIPを選択。x64以外は現PCでは未検証です。
MSIを使う場合も1.5.1を選択し、OS承認を手動で行います。

```powershell
arduino-cli version # Version: 1.5.1 を確認
```

見つからない場合はPathとPowerShell再起動を確認。
異版が出る場合は `Get-Command arduino-cli -All` で優先される実行ファイルを確認します。
setup/verifyは `$env:LOCALAPPDATA/ArduinoCLI/1.5.1/arduino-cli.exe` があればその配置を使用します。
別の配置の場合はPATHから検出し、どちらもCLIの版を厳密に確認します。

## 8. AVRコアを導入（自動セットアップ開始）

以下の1コマンドで**手順8～10（コア、ライブラリ、検証、compile）**を順に自動実行します。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1 -InstallExtensions
if ($LASTEXITCODE -ne 0) { throw 'Setup failed; read the [FAIL] message' }
```

`-InstallExtensions` は任意。手動導入済みなら省略できます。
ExecutionPolicy Bypassは子PowerShellに限定し、PC全体のポリシーを変更しません。
組織ポリシーで禁止されている場合は組織の許可された手順で実行します。
スクリプトはCLI 1.5.1を確認し、不足している `arduino:avr@1.8.8` を導入します。
`scripts/common.ps1` がリポジトリ位置から次の専用ディレクトリを毎回選択します。

| CLI設定 | リポジトリ内の保存先 |
|---|---|
| ARDUINO_DIRECTORIES_DATA | `.arduino-local/data`（コア・ツール・index） |
| ARDUINO_DIRECTORIES_DOWNLOADS | `.arduino-local/downloads` |
| ARDUINO_DIRECTORIES_USER | `.arduino-local/user`（ライブラリ） |

グローバルのArduinoコア・ライブラリは保持します。専用環境に指定版があれば再インストールせず、
専用環境内の異版は変更前に停止します。既存の異版を無断で削除・上書きしないでください。
setup/verifyを別の作業ディレクトリから呼んでも、保存先は呼び出したリポジトリ内です。

Arduino CLIを直接使う場合は、リポジトリルートの同じPowerShellで以下の3変数を先に設定します。
VS Codeの新規ターミナルでは共有設定により設定されます。
子PowerShellで実行したsetup/verifyの環境変数は、親PowerShellには戻りません。

```powershell
$localArduinoRoot = Join-Path (Get-Location).Path '.arduino-local'
$env:ARDUINO_DIRECTORIES_DATA = Join-Path $localArduinoRoot 'data'
$env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $localArduinoRoot 'downloads'
$env:ARDUINO_DIRECTORIES_USER = Join-Path $localArduinoRoot 'user'
# 手動導入時のみ。通常はsetupに任せる
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.8
```

## 9. Arduinoライブラリを導入

setupが以下の4種を導入します。依存更新を防ぐため全種を明示して `--no-deps` を使用します。

```powershell
# 手動導入時のみ。指定版がある場合は実行不要
arduino-cli lib update-index
arduino-cli lib install --no-deps "Adafruit BusIO@1.17.4"
arduino-cli lib install --no-deps "Adafruit GFX Library@1.12.6"
arduino-cli lib install --no-deps "Adafruit SSD1306@2.5.17"
arduino-cli lib install --no-deps "Adafruit_VL53L0X@1.2.5"
```

DFRobotDFPlayerMiniは現行ソースで未使用なので不要。Wire/SPIはコア付属。
固定版の正本は [environment.lock.json](../scripts/environment.lock.json) です。

setup/verifyは環境変数の事前設定なしで専用環境を選びます。
手動CLIのlib/compile/upload/monitorでは、手順8の3変数を同じセッションに設定してください。
`.arduino-local/`、`.tools/`、build成果物はGit無視対象です。

## 10. Mega 2560向けcompileと検証

setupはverifyを呼び出し、毎回新しいビルドフォルダと `--clean` でcompileします。
再確認用コマンド:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_environment.ps1
if ($LASTEXITCODE -ne 0) { throw 'Verification failed' }
```

Git、CLI版、AVRコア版、ライブラリ版、`.ino` と `config.h` の存在、Mega compileを検証します。
成功は `[PASS]` と終了コード0、失敗は `[FAIL]` と終了コード1。
このPCの構築時はFlash 34,146 bytes／静的RAM 1,488 bytes。ビルド成功は実機確認の完了を意味しません。
このPCでは空の専用環境への導入と、既存環境を保持した再実行を確認しています。
VS CodeのCtrl+Shift+Bも同じ正本のverifyを使用します。

手順12のupload用には、手順8の3変数を設定した同じシェルで、決まった出力先へ改めてcompileします。

```powershell
arduino-cli compile --clean --fqbn arduino:avr:mega:cpu=atmega2560 --build-path .\build\mega2560 .\dorayaki_io_check
if ($LASTEXITCODE -ne 0) { throw 'Compile failed; do not upload old artifacts' }
```

## 11. Mega接続後にCOMポートを確認

加熱・モータ電源を切り離し、データ通信対応USBケーブルで接続。

```powershell
arduino-cli board list
```

接続前後の差分やデバイスマネージャーで対象COM番号を照合します。
このPCではCOM4へのupload成功がユーザー確認済みですが、次PCではその番号を流用せず照合します。
出ない場合はケーブル、USB、デバイスマネージャーを確認。
互換品のUSBチップ用ドライバが必要なら、現物のチップを確認してメーカー公式のものを手動導入します。
実機未接続なら手順11以降は保留し、compileまでで環境構築完了です。

## 12. upload

人間がボード・ポート・配線を確認して実行します。setup/verifyは自動uploadしません。

```powershell
$megaPort = 'COM3' # 例。確認した実機ポート名へ変更
arduino-cli upload --port $megaPort --fqbn arduino:avr:mega:cpu=atmega2560 --input-dir .\build\mega2560 .\dorayaki_io_check
if ($LASTEXITCODE -ne 0) { throw 'Upload failed' }
```

手順10で成功した最新成果物を使います。書き込み前に他のSerial Monitorを終了。
upload後にD9=0、D12=LOWを実測し、[HANDOFFの順序](HANDOFF.md#次の実機確認順序)へ進みます。

## 13. Serial Monitorで確認

```powershell
arduino-cli monitor --port $megaPort --config baudrate=115200
```

必要なら接続後にMegaをリセットし、`DORAYAKI_IO_CHECK_START`、
`SAFETY: SSR=OFF, MOTOR_PWM=0`、I2C検出、`OLED_INIT=OK`、`TOF_INIT=OK`を確認。
定期ログには設定値、PT100抵抗・ADC・温度、ToF距離・status、SW状態が出ます。
未接続や不正値はERR。終了はCtrl+C。
実測結果をHANDOFFへ記録し、MD20A単体試験を別工程として準備します。

OLEDはSDA→Mega D20、SCK→Mega D21への誤配線修正で解決済みです。
I2C scanで0x29と0x3Cを検出し、OLED_INIT=OK、TOF_INIT=OK。同一I2Cバスで正常に共存しています。
開始SWは配線修正済み・再ログ確認待ちで、通常HIGH／押下LOWの遷移とブザー動作結果は未記録です。
D9/D12の端子電圧実測は未実施です。A0/A1/A2の両端・中間の実機確認結果は未記録です。
PT100の固定抵抗は実物確認済みの1kΩ、コードは `PT100_SERIES_RESISTOR_OHM=1000.0F` です。
1kΩ修正版の数値表示は確認済みですが、基準温度計との比較・校正と最終温度制御精度の確認は未実施です。
ToFは通信・正常測距実績があり、取付条件は未確定です。MD20A単体試験は未実施、将来の初期PWMは50%予定です。
詳細な実機記録と次の確認順序は[HANDOFF](HANDOFF.md)を参照してください。

## 人間が行う作業

- Git、VS Code、必要ならGitHub CLIのインストールとOS承認。
- Codex/GitHubログイン、アクセス権確認、Git author設定。
- Arduino CLI 1.5.1導入、ユーザーPath設定、シェル・アプリ再起動。
- USB接続、必要な公式ドライバ、COM照合、uploadの実行判断。
- D9/D12とセンサ等の実測、結果記録。モータ・加熱の試験は別工程。

ネットワークやパッケージ配布が利用できない場合は `[FAIL]` の原因を解消して再実行します。
版番号を変更して回避しないでください。指定版が揃っていればsetupはインデックス更新を行いません。
