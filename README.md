# たい焼き焼印プロジェクト

Arduino Mega 2560 Rev3で、焼き印用はんだごて、MD20A経由のリニアアクチュエータ、
OLED、ToF、PT100、設定用可変抵抗を扱うプロジェクトです。
**現在は「第1段階 I/O・センサ確認版まで完成」**。自動焼印、加熱制御、モータ駆動は未実装です。
2026-10-06に現行ソースのクリーンコンパイルを確認しました。実機確認の記録はまだありません。

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
| A7 | PT100 | 100Ω抵抗とPT100の分圧入力 |
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

PT100の入力回路（ユーザー指定、抵抗の実測・校正は未実施）:

```text
5V
 |
100Ω
 |
+---- A7
 |
PT100
 |
GND
```

可変抵抗は両端を5V/GND、摺動端子を対応するA0/A1/A2へ接続します。
開始SWはD4の外付けプルアップとGNDへのNO接点を使用します。
部品電源、I2C電圧適合、配線定格は採用品の資料と現物で確認してください。
元の[シーケンス手書きメモ](docs/references/sequence-notes.jpg)と
[Megaピン手書きメモ](docs/references/mega-pin-notes.webp)も保存しています。
これらは過去の参考案で、現行のピンと値の正本は `config.h` と本表です。

## ビルド

リポジトリルートのWindows PowerShellで実行します。
Git、Arduino CLI 1.5.1が必要です。初回のインストール手順は[新PC構築手順](docs/NEW_PC_SETUP.md)を参照。

```powershell
# 固定コア・ライブラリを導入し、環境検証とクリーンcompileまで実行
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1 -InstallExtensions

# 導入済み環境の確認とクリーンcompile
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_environment.ps1
```

ビルド生成物は無視対象の `build/verify-<一意ID>` に保存されます。
手動で書き込み用のビルドを作る場合:

```powershell
arduino-cli compile --clean --fqbn arduino:avr:mega:cpu=atmega2560 --build-path .\build\mega2560 .\dorayaki_io_check
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

実施順は、Megaへ書き込み → D9/D12安全状態の実測 → OLED/ToF → A0/A1/A2 →
PT100常温 → 開始SW/ブザー → MD20A単体試験です。
詳細と記録方法は[HANDOFF](docs/HANDOFF.md)を参照してください。

DIR極性、ToF距離と上下方向、取付位置・原点距離、全ストローク所要時間、
ブザー種別、PT100の実測精度・自己発熱・校正が未確定です。
自動運転、温度制御、測距異常時停止、移動タイムアウト、異常復帰は未実装です。
