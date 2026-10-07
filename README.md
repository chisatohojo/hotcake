# たい焼き焼印プロジェクト

Arduino Mega 2560 Rev3で、焼き印用はんだごて、MD20A経由のリニアアクチュエータ、
OLED、ToF、PT100、設定用可変抵抗を扱うプロジェクトです。
**現在は「第1段階 I/O・センサ確認版まで完成」**。自動焼印、加熱制御、モータ駆動は未実装です。
2026-10-07にこのPCの専用Arduino環境でMega 2560向けクリーンコンパイルを確認しました。
COM4への1kΩ修正版uploadと起動ログを確認しました。前回のToF正常測距はユーザー確認済みです。
OLED初期化ERR、開始SWのLOW固定が残り、D9/D12の端子電圧は未実測です。

**現行I/O版ではSSRは絶対にLOW、モータPWMは絶対に0。** 起動時と毎回のloopで維持します。
開始スイッチを押しても、ブザーのみが作動します。

## 引き継ぎの入口

- [新しいWindows PCの構築手順](docs/NEW_PC_SETUP.md)
- [開発引き継ぎ・次の実機確認](docs/HANDOFF.md)
- [実環境の調査記録](docs/ENVIRONMENT.md)
- [Codex作業ルール](AGENTS.md)
- スケッチ: [dorayaki_io_check.ino](dorayaki_io_check/dorayaki_io_check.ino)
- ピン・定数・調整値: [config.h](dorayaki_io_check/config.h)
- 固定バージョンの正本: [environment.lock.json](scripts/environment.lock.json)

フォルダ名 `dorayaki_io_check` と起動ログの `DORAYAKI` は既存ソースの名称です。
GitHubリポジトリ名は `hotcake`、対象装置はたい焼き焼印装置です。

## 実装済み機能

- A0/A1/A2を100ms周期で読み取り、指数移動平均で平滑化して設定範囲へ変換。
- PT100の分圧値から抵抗値と温度を算出。0～850℃範囲の式を使用し、不正値はERR。
- VL53L0Xを同期測距し、RangeStatus=0かつ距離が正の場合のみ有効と判定。
- 起動時のI2Cスキャン、OLED/ToF初期化結果のログ。
- OLEDは250ms周期で更新、設定・センサ画面を2.5秒ごとに切替。
- 開始SWは30msのデバウンス、押下遷移で100msのブザー。
- Serialは115200bps、1秒ごとに状態ログを出力。
- millis()で更新周期を管理。ToFライブラリ内部の同期処理は待ち時間を伴います。

## ピンアサイン

| ピン | 用途 | 現行の仕様 |
|---|---|---|
| A0 | 温度設定可変抵抗 | 150～300℃ |
| A1 | 時間設定可変抵抗 | 1～10秒 |
| A2 | 高さ設定可変抵抗 | 0～300mm |
| A7 | PT100 | 実物確認した1kΩ固定抵抗とPT100の分圧入力 |
| D4 | 開始SW | 外付けプルアップ、NO接点、通常HIGH／押下LOW |
| D5 | ブザー | 暫定でアクティブ、HIGH=ON |
| D8 | MD20A DIR | 暫定LOW。HIGH/LOWと上下方向の関係は未確定 |
| D9 | MD20A PWM | **0固定**。将来の単体試験は50%から開始予定 |
| D12 | SSR | HIGH=焼き印用はんだごてON。現行は**LOW固定** |
| D20 | I2C SDA | OLEDとToF共用 |
| D21 | I2C SCL | OLEDとToF共用 |

## ハードウェア

- MCU: Arduino Mega 2560 Rev3（ATmega2560、FQBN `arduino:avr:mega:cpu=atmega2560`）。
- OLED: 秋月電子115870、0.96インチ、128×64、SSD1306。コードのアドレスは0x3C。
- ToF: M5Stack Unit ToF、VL53L0X、コードのアドレスは0x29。I2Cクロックは400kHz。
- モータドライバ: MD20A。アクチュエータの全ストロークは約60秒との情報で、実測値は未確定。
- 加熱: SSR経由の焼き印用はんだごて。SSR・電源等の型番と配線詳細は未記録。
- リミットSWは使用せず、将来はToFのみで初期位置・高さを判断する計画。取付位置・原点距離は未確定。

PT100の入力回路（固定抵抗はユーザーの実測・実物確認で1kΩ、温度校正は未実施）:

```text
5V
 |
1kΩ
 |
+---- A7
 |
PT100
 |
GND
```

`config.h` の `PT100_SERIES_RESISTOR_OHM` は `1000.0F` に修正済みです。
PT100自体の0℃基準抵抗 `PT100_R0_OHM=100.0F` はそのままです。

可変抵抗は両端を5V/GND、摺動端子を対応するA0/A1/A2へ接続します。
開始SWはD4の外付けプルアップとGNDへのNO接点を使用します。
部品電源、I2C電圧適合、配線定格は採用品の資料と現物で確認してください。
元の[シーケンス手書きメモ](docs/references/sequence-notes.jpg)と
[Megaピン手書きメモ](docs/references/mega-pin-notes.webp)も保存しています。
これらは過去の参考案で、現行のピンと値の正本は `config.h` と本表です。

## ビルド

リポジトリルートのWindows PowerShellで実行します。
Git、Arduino CLI 1.5.1が必要です。初回のインストール手順は[新PC構築手順](docs/NEW_PC_SETUP.md)を参照。
setupとverifyは毎回、リポジトリ内の `.arduino-local/` を自動選択します。
別プロジェクトのコア・ライブラリを上書きせず、`.tools/` のコピーも不要です。

```powershell
# 固定コア・ライブラリを導入し、環境検証とクリーンcompileまで実行
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1 -InstallExtensions

# 導入済み環境の確認とクリーンcompile
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_environment.ps1
```

共有の `.vscode/tasks.json` により、VS Codeでこのフォルダを開いて **Ctrl+Shift+B** でも
同じverify・クリーンcompileを実行できます。補完・新規ターミナルの専用環境設定も共有済みです。

ビルド生成物は無視対象の `build/verify-<一意ID>` に保存されます。
手動でArduino CLIを使う場合は、VS Codeの新規ターミナルを使うか、同じPowerShellで専用環境を設定します。
子PowerShellのsetup/verifyで設定した環境変数は、親シェルには戻りません。
書き込み用ビルドを作る例:

```powershell
$localArduinoRoot = Join-Path (Get-Location).Path '.arduino-local'
$env:ARDUINO_DIRECTORIES_DATA = Join-Path $localArduinoRoot 'data'
$env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $localArduinoRoot 'downloads'
$env:ARDUINO_DIRECTORIES_USER = Join-Path $localArduinoRoot 'user'
arduino-cli compile --clean --fqbn arduino:avr:mega:cpu=atmega2560 --build-path .\build\mega2560 .\dorayaki_io_check
if ($LASTEXITCODE -ne 0) { throw 'Compile failed; do not upload old artifacts' }
```

固定対象はCLI 1.5.1、AVR Boards 1.8.8、BusIO 1.17.4、GFX 1.12.6、SSD1306 2.5.17、
`Adafruit_VL53L0X` 1.2.5です。Wire/SPI等はAVRコア付属を使用します。

## 実機書き込み

モータ・加熱電源を切った状態で、配線と対象ボードを確認してから実施します。
セットアップ・検証スクリプトはuploadしません。

```powershell
arduino-cli board list
$megaPort = 'COM3' # 例。実際に検出されたMegaのポートに置き換える
arduino-cli upload --port $megaPort --fqbn arduino:avr:mega:cpu=atmega2560 --input-dir .\build\mega2560 .\dorayaki_io_check
arduino-cli monitor --port $megaPort --config baudrate=115200
```

直前の手動compileが成功した `build/mega2560` の成果物を使用してください。
Serial Monitor終了はCtrl+C。書き込み時は他アプリのモニタを閉じます。
起動時に `DORAYAKI_IO_CHECK_START`、`SAFETY: SSR=OFF, MOTOR_PWM=0`、
`OLED_INIT=OK`、`TOF_INIT=OK` を確認します。
定期ログの `SSR=OFF,MOTOR_PWM=0` はコード上の状態表示で、端子電圧の実測ではありません。

## 次の実機テスト・未確定項目

2026-10-07に受領したユーザーの実機確認記録（エージェントが今回再測定した結果ではありません）:

| 項目 | 確認結果・残件 |
|---|---|
| Mega 2560 | COM4へのupload成功、起動ログ確認済み。次PCではCOM番号を再確認する。 |
| 安全出力 | ログ上SSR=OFF、MOTOR_PWM=0。D9/D12の端子電圧実測は未実施。 |
| OLED / I2C | OLED初期化ERR。I2C scanで0x29のみ検出。OLED配線・電源・アドレスを確認する。 |
| ToF | 初期化OK。62～65mm、status=0で正常測距。対象条件によってstatus=2も発生。 |
| PT100 | PT100_ADC=100。固定抵抗を実測・実物確認した結果1kΩだったため設定を修正。温度校正は未実施。 |
| 開始SW | START_SWはログ上LOWのまま。D4の外付けプルアップ・NO接点・配線を確認する。 |

前回ユーザー確認分の書き込み元の版・詳しい測定日時、ToFの対象条件、PT100の基準温度は未記録です。
同日21:01（日本時間）までに、commit `25ca1f47b65dd31bf45c5661865212611dcabde7` の1kΩ版を
clean compileし、COM4のMega 2560へupload成功（終了コード0）。モータ・加熱電源の切り離しはユーザー確認済みです。
115200bpsで約12秒のSerialログを取得し、起動ログ、SSR=OFF、MOTOR_PWM=0を確認しました。
PT100_ADC=98～99、PT100_R=105.95～107.14Ω、TEMP_ACT=15.2～18.3℃と表示されました。
これはコードの算出・表示値で、基準温度計との比較・校正と端子電圧実測は未実施です。
今回もI2C 0x29のみ、OLED_INIT=ERR、TOF_INIT=OK。取得した定期ログではTOF_STATUS=2（TOF=ERR）、
START_SW=LOWのままでした。今回の測距対象条件は未記録です。
次はD9/D12の実測、OLEDと開始SWの配線確認、1kΩ修正版でのPT100常温比較を行います。
モータ・加熱の電源を切り離して進め、MD20A単体試験は別途明示した作業として扱います。
詳細と記録方法は[HANDOFF](docs/HANDOFF.md)を参照してください。

DIR極性、ToF距離と上下方向、取付位置・原点距離、全ストローク所要時間、
ブザー種別、PT100の実測精度・自己発熱・校正が未確定です。
自動運転、温度制御、測距異常時停止、移動タイムアウト、異常復帰は未実装です。
