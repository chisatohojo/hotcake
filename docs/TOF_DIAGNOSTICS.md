# VL53L0X 測距不良の診断・比較手順

第1段階I/O確認版に追加したSerial診断です。D2/D12=LOW、D9のPWM=0を維持します。
測距の成功条件は従来どおり `RangeStatus == 0 && RangeMilliMeter > 0` です。
品質チェック、タイミングバジェット、VCSEL設定、補償・校正値は変更していません。
診断400kHz版はユーザーの指示後、2026-10-21に実機upload・フラッシュ検証成功、2026-10-28に起動ログを確認しました（[記録](HANDOFF.md#tof診断400khz版の書き込みと起動確認)）。距離・速度比較と負荷通電試験は未実施です。100kHz版など追加の書き込みは別途の指示後に行います。

## 100kHz / 400kHz の切り替え

`dorayaki_io_check/config.h` の1か所だけを変更します。既定値は400kHzです。

```cpp
constexpr uint32_t I2C_CLOCK_HZ = 400000UL;  // 400kHz
// 100kHzを試すときは右辺のみ100000ULに変更する
```

変更ごとに `scripts/verify_environment.ps1` でクリーンcompileし、その条件の成果物を使用します。
この定数は共有バス全体に適用されます。OLEDの表示内容・更新周期は同じですが、100kHzでは転送時間が長くなります。
起動ログと各1秒ログの `I2C_CLOCK_HZ` が予定した条件と一致することを確認してください。
このログはソフト設定値です。実際のSCL周波数・立ち上がり波形はロジアナやオシロスコープで別途確認します。

固定版ライブラリを確認した結果、従来の `Wire.setClock(400000UL)` 一度だけでは速度を維持できない経路がありました。

- Adafruit_VL53L0X 1.2.5の `begin()` は内部で `Wire.begin()` を呼び、AVR Wireの速度設定を初期化します。
  今回は成功・失敗のどちらでも、その直後に `Wire.setClock(I2C_CLOCK_HZ)` を再適用します。
  ライブラリ内部の初期化通信は100kHzになる経路を残し、比較対象は初期化完了後の測距通信です。ライブラリ本体は変更していません。
- Adafruit_SSD1306 2.5.17の既定値は転送中400kHz・転送後100kHzです。
  今回はコンストラクタの `clkDuring` と `clkAfter` の両方へ同じ `I2C_CLOCK_HZ` を渡し、OLED更新による速度変更を防ぎます。
  OLEDの `begin()` には `periphBegin=false` を渡し、既に開始したWireを再初期化させません。

根拠: [VL53L0X 1.2.5初期化](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/Adafruit_VL53L0X.cpp)、
[SSD1306 2.5.17のクロック引数](https://github.com/adafruit/Adafruit_SSD1306/blob/2.5.17/Adafruit_SSD1306.h)、
[SSD1306の転送前後処理](https://github.com/adafruit/Adafruit_SSD1306/blob/2.5.17/Adafruit_SSD1306.cpp)、
[AVR 1.8.8のWire初期化](https://github.com/arduino/ArduinoCore-avr/blob/1.8.8/libraries/Wire/src/utility/twi.c)。

## Serial診断値と単位

115200bps、既存の1秒周期・項目・順序を維持し、行末へ項目を追加します。
従来の `TOF` は有効距離または `ERR`、追加の `TOF_DIST` は直近API成功時の診断用返値です。

| 項目 | 形式・単位・意味 |
|---|---|
| `TOF_STATUS` | 従来項目。API成功時のRangeStatus。初期化未完了は`INIT_ERR`、API失敗時の255は未取得用の値。 |
| `TOF_API` | 測距APIの符号付き戻り値。0=API成功、例:-20=通信インターフェースエラー、-7=タイムアウト。未実行は`NOT_RUN`。 |
| `TOF_DIAG` | `AVAILABLE`=API成功・診断構造体取得済み、`API_ERR`=測距API失敗、`NOT_FOUND`=起動時0x29未検出、`INIT_ERR`=検出後の初期化失敗、`NO_SAMPLE`=API未実行。 |
| `TOF_VALID` | 1=従来の成功条件を満たす、0=満たさない。`AVAILABLE`やAPI=0だけで距離を有効扱いしない。 |
| `TOF_DIST` | `RangeMilliMeter`、整数mm。RangeStatusが非0の数値は有効距離ではなく、位置や成功判定に使用しない。 |
| `TOF_SIGNAL_MCPS` | `SignalRateRtnMegaCps / 65536.0`。uint32の符号なし16.16固定小数点をMCPSへ換算。1 MCPS=1,000,000 counts/s。 |
| `TOF_AMBIENT_MCPS` | `AmbientRateRtnMegaCps / 65536.0`。同じ16.16形式、MCPS。周囲光由来の計数率。 |
| `TOF_SPAD` | `EffectiveSpadRtnCount / 256.0`。uint16の符号なし8.8形式を有効SPAD数へ換算。小数部を含む。 |
| `TOF_SIGNAL_RAW` / `TOF_AMBIENT_RAW` / `TOF_SPAD_RAW` | 上記API構造体の未換算整数。センサレジスタの直接ダンプではない。 |
| `I2C_CLOCK_HZ` | 共有Wireバスに適用するソフト設定値、Hz。 |

例: rateのRAW=65536は1.000000 MCPS、SPADのRAW=384は1.5000です。
MCPSは小数6桁、SPADは小数4桁で出力します。AVRのfloatと表示丸めで失われる桁は、RAW値からPC側で再計算できます。
`SignalRateRtnMegaCps` はクロストーク補償が有効ならその補正を反映します。今回、補償の設定は変更していません。

測距APIが非0なら構造体は読まず、診断値を毎回破棄してすべて `NA` とします。未実行の場合も `NA` で、理由は `TOF_DIAG` と `TOF_API` で区別します。
前回の診断値を再利用しません。API成功でもS2などの不良判定時は診断値を出しますが、従来の `TOF=ERR` とOLED診断表示は維持します。

根拠: [ST UM2039 §5.3.2・Range Status表（16ページ）](https://www.st.com/resource/en/user_manual/um2039-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf)、
[同梱ST APIの測定構造体定義](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/vl53l0x_def.h)、
[16.16型の定義](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/vl53l0x_types.h)、
[ST APIのSPAD 8.8形式の取得処理](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/core/src/vl53l0x_api.cpp)。

## 全測距試行の集計

1秒ログの診断値は直近1回のスナップショットです。一方、以下の集計は約100ms周期の**すべての測距API呼び出し**を数えます。
同期測距・OLED転送・Serial送信の待ちがあるため、実際の呼び出し回数を10回/秒と決めつけないでください。

| 項目 | 意味 |
|---|---|
| `TOF_WINDOW` | 集計区間ID。起動時は1。 |
| `TOF_WINDOW_MS` | 区間開始からの経過時間、ms。 |
| `TOF_N` | 測距APIを呼んだ回数。未初期化で呼べなかった回は含まない。 |
| `TOF_OK` | API=0かつRangeStatus=0かつ距離>0の成功回数。 |
| `TOF_API_ERRS` | 測距APIの戻り値が非0だった回数。 |
| `TOF_BAD_RANGE` | API=0だが従来の成功条件を満たさない回数。 |
| `TOF_STATUS_COUNTS` | API=0のRangeStatus分布。例:`0:80\|2:19`。発生したstatusのみ出力、0～255に対応。未取得時の代用255は数えない。空なら`NONE`。 |

`TOF_N = TOF_OK + TOF_API_ERRS + TOF_BAD_RANGE`、status分布の合計は `TOF_N - TOF_API_ERRS` です。
status=0でも距離0は成功に数えません。成功率は `100 × TOF_OK / TOF_N`、N=0なら算出不能です。
数値は累積値なので、1秒ごとのNやhistogramを足し合わせないでください。

Serialへ `r` または `R` を送ると、次の通常測距の直前に集計だけをリセットします。
`TOF_STATS_RESET=<新しい区間ID>` の応答後に、そのIDの行を採用します。ToFの再初期化や設定変更、出力ピン操作は行いません。
改行は無視されます。入力処理は1loopあたり最大8文字で、既存の開始SW・ブザー処理は変更していません。

追加項目の架空例（実測値ではありません、直近のS2は有効距離として扱いません）:

```text
TOF=ERR,TOF_STATUS=2,...,I2C_CLOCK_HZ=400000,TOF_DIAG=AVAILABLE,TOF_API=0,TOF_VALID=0,TOF_DIST=150mm,TOF_SIGNAL_MCPS=0.199997,TOF_AMBIENT_MCPS=0.009995,TOF_SPAD=15.0000,TOF_SIGNAL_RAW=13107,TOF_AMBIENT_RAW=655,TOF_SPAD_RAW=3840,TOF_WINDOW=2,TOF_WINDOW_MS=10000,TOF_N=100,TOF_OK=80,TOF_API_ERRS=1,TOF_BAD_RANGE=19,TOF_STATUS_COUNTS=0:80|2:19
```

## 白いA4コピー用紙による比較

書き込み・測定は別途の指示後に実施します。最初はモータ・ヒーター・両SSRの負荷電源を切り離し、USBのみとし、端子状態も別途確認します。

1. 400kHz版をクリーンcompileし、許可された書き込み後に起動ログのバージョン・速度・0x29検出・TOF_INIT=OKを確認する。
2. 白いA4コピー用紙をセンサ正面に平行に固定する。距離の基準は光学窓とし、対象面で視野を十分に覆う。
   紙の種類・向き、照明、センサ・ジグ・OLED・ケーブル・電源・測距設定を固定して記録する。
3. 50mmに固定して安定後 `r` を送り、応答を含むSerialログを保存する。最低30秒、かつNが100回以上になるまで記録する。
   最終行の同一区間のN・OK・API_ERRS・BAD_RANGE・status分布と、区間内の複数の距離・信号・周囲光・SPADのスナップショットを残す。
4. 同じ手順を80、100、150、300mmで行う。移動中の試行は各距離のリセット前に除外する。
5. `I2C_CLOCK_HZ` のみ100000ULへ変更し、クリーンcompile・許可された書き込み後、同じ5距離・同じ条件で再実施する。
   比較中に測距プロファイル、タイミングバジェット、判定しきい値、VCSEL、校正・補償を変更しない。
6. 条件差を確認するため、必要なら400kHzへ戻して同じ条件を再測定する。初期化中に光学窓を指や物で覆わない。

記録表（未実測）:

| I2C | 基準距離 | 区間ID・ms | N / OK / 成功率 | API_ERRS / BAD_RANGE | RangeStatus分布 | 距離・SIGNAL・AMBIENT・SPADの複数ログ |
|---|---|---|---|---|---|---|
| 400kHz | 50mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 400kHz | 80mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 400kHz | 100mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 400kHz | 150mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 400kHz | 300mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 100kHz | 50mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 100kHz | 80mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 100kHz | 100mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 100kHz | 150mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |
| 100kHz | 300mm | 未実施 | 未実施 | 未実施 | 未実施 | 未実施 |

併せてfirmware commit、configの値、ライブラリ版、日時、電源端子電圧、SDA/SCLのアイドル電圧と波形、照明・温度、
紙の型・寸法・姿勢、窓からの実距離、ジグと光学窓の写真、配線長、OLED接続、ログファイル名を残します。
診断版のSerial行は長くなるため、1秒周期はmillis()で維持しますが、送信・同期測距による待ち時間はあります。
最新値のスナップショットを全試行の距離・信号分布と混同しないでください。

## 結果の読み方と次の切り分け

以下は候補で、今回の実装だけでは原因は特定していません。400/100kHzの比較完了後、1項目ずつ別試験として変更します。

- 400kHzでAPI通信エラーが増え100kHzで減る場合は、配線・GND・プルアップ・SCL/SDA波形・バス容量を確認する。
  -20は通信エラーです。-7はタイムアウトで、通信速度だけを原因と断定しない。アイドル3.3Vだけでは波形品質は分からない。
- API=0で両速度ともS2が多い場合、S2はSTのSignal Failであり、信号しきい値またはRIT（Range Ignore Threshold）で発生する。
  「遠すぎる」「通信不良」とは確定しない。信号・周囲光・SPADを同じ距離で比較し、速度変更による改善がなければ他の候補へ進む。
- 光学窓: 保護フィルムの有無を斜光・拡大で確認し、汚れ・傷・曇り、送受光窓や視野の遮蔽を確認する。
  ジグが窓より前へ出ていなくても、視野や反射への影響は未評価。ジグを外した単体状態との比較、光沢面や周辺物からの反射を別条件として調べる。
- 周囲光: 直射日光・強い照明などを記録し、AMBIENTの増加と不良率を比較する。照明変更はバス速度比較と混ぜない。
- 電源: 約4.78VというDC測定値だけでは測距時の降下・リップルは分からない。ユニット側とGNDを基準に波形を測り、
  コネクタ・ケーブル・レギュレータ・デカップリングを採用品の仕様に沿って確認する。ユニット入力と裸のVL53L0Xの電源仕様を混同しない。
- 測距設定: 実際のタイミングバジェット、signal/sigma/RITの有効状態と値、VCSEL、校正・クロストーク補償を固定版の実装と照合する。
  AdafruitのDEFAULTはRITを有効化するため、STの表の一般的な既定値と同一と決めつけない。S2を消すために品質判定を無効化しない。
- 共用バス: 電源を切ったうえでOLEDを外す比較は、速度比較とは別試験にする。OLED・プルアップ・配線の影響を一つずつ確認する。
- 個体差: 同じ紙・照明・配線・設定で既知良品と交換比較する。型番・外観・購入ロット・窓状態も記録し、差だけで故障原因を確定しない。

資料: [M5Stack Unit ToF（U010）](https://docs.m5stack.com/en/unit/Unit_ToF)、
[ST VL53L0Xデータシート](https://www.st.com/resource/en/datasheet/vl53l0x.pdf)、
[ST UM2039のRange Status・制限値・校正](https://www.st.com/resource/en/user_manual/um2039-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf)、
[Adafruit 1.2.5のDEFAULT設定](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/Adafruit_VL53L0X.cpp)。
