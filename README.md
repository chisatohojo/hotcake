# たい焼き焼印プロジェクト

Arduino Mega 2560 Rev3で、焼き印用はんだごて、MD20A経由のリニアアクチュエータ、
OLED、ToF、PT100、設定用可変抵抗を扱うプロジェクトです。
**現在はI/O確認版を基に、MD20Aの上下1サイクル制御を実装済みです。** ユーザー確認（HIGH=上昇／LOW=下降、上昇時ToF距離増加、300mmまで移動可能）を受け、開始SWによる自動上下運転を有効化しました。加熱制御は未実装です。
2026-10-07にこのPCの専用Arduino環境でMega 2560向けクリーンコンパイルを確認しました。
COM4への1kΩ修正版uploadと起動ログを確認しました。前回のToF正常測距はユーザー確認済みです。
最新のユーザー確認でOLEDの誤配線を修正し、OLEDとToFの同一I2Cバスでの共存・初期化を確認しました。
開始SWの外付けプルアップ抵抗が未接続と判明し、ユーザー指定でD4を内蔵プルアップへ変更しました。
内蔵プルアップ版の書き込み後、離すとHIGH・押すとLOW、押下時のブザーをユーザーが確認しました。
D9/D12の端子電圧実測とPT100の校正は未実施です。
2026-10-14にOLEDの通常表示を6行の1画面へ変更し、COM4への書き込み・フラッシュ検証に成功しました。
OLED初期化OKをログで確認しました。6行表示の目視確認は未実施です。
ToFのOLED表示は単なるERRから、通信・初期化・測距条件別の診断ラベルへ変更しました。
2026-10-21にホットプレート用SSRの制御ピンD2を追加し、同日20:05（日本時間）にCOM4への書き込み・フラッシュ検証に成功しました。
115200bpsの起動・定期ログでHOTPLATE_SSR=OFF、SSR=OFF、MOTOR_PWM=0を確認しました。20:06の起動ではOLEDは初期化OK、ToFの0x29は未検出でした。
その後の確認では0x29検出・TOF_INIT=OK、API=0でS2が多発し、有効距離が安定していません。過去の目測距離は測定限界や精度の根拠には使用しません。
測距不良の診断ログと100kHz/400kHz比較機能を追加しました。診断400kHz版はCOM4へ書き込み・フラッシュ検証済みです。2026-10-28の起動ログでも400kHz設定、OLED・ToF初期化OKを確認しました。

**D2/D12は常にLOW。`ACTUATOR_SETUP_CONFIRMED=true` で開始SWの新しい押下により上下1サイクルを実行します。** 起動時・待機・保持・ToF無効時はPWM=0です。

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

## アクチュエータ上下1サイクル（開始SWで実行）

D4の新しい押下で `IDLE → MOVE_UP_INITIAL → MOVE_DOWN → HOLD → MOVE_UP_RETURN → IDLE` を実行します。
開始時にToFを新規取得し、A1の保持時間（1～10秒）とA2の下降目標を固定。動作中の設定変更・開始SW再押下はそのサイクルへ反映しません。
上昇は300mm以上、下降は固定した目標以下でPWM=0。すでに上昇位置なら上昇せず、保持はmillis()ベース・PWM=0です。
ToFが無効になればPWM=0で中断してIDLEへ戻り、復旧だけでは再開しません。起動時に押されていたSWも、一度離して押すまで開始しません。

`config.h` の設定:

| 定数 | 既定値・意味 |
|---|---|
| `ACTUATOR_SETUP_CONFIRMED` | true。ユーザー指示により有効化。起動直後は動かず、開始SWの新しい押下が必要。 |
| `MOTOR_PWM_RUN` | 255/255、Duty 100%。停止・保持時は0。 |
| `MOTOR_DIR_UP_LEVEL` | HIGH=上昇、LOW=下降（ユーザーの短時間駆動で確認済み）。 |
| `ACTUATOR_UPPER_MM` | 300mm。 |
| `ACTUATOR_LOWER_MIN_MM` | 暫定50mm。A2の0～300mm設定を、この下限～300mmへ制限して使用する。 |

2026-10-28、ユーザーからDIR極性、上昇時の距離増加、300mmまでの機械的移動可能を確認済みとの報告を受け、有効化しました。下降下限50mmは指定どおり暫定値を維持します。追加の試験機能はありません。
距離急変監視、連続正常判定、タイムアウト等は追加していません。別系統の非常停止を前提とし、停止判定の周期は既存100ms測距とライブラリの同期処理・OLED/Serialの待ち時間を伴います。
OLEDの6行と設定・測定値は維持し、最下行のPWMだけ実際の指令値を表示します。Serialも既存項目を維持し、ACTUATOR_STATE、ACTUATOR_SETUP_CONFIRMED、固定したCYCLE_TARGET_MM/CYCLE_HOLD_MSを追記します。
RIT無効・I2C 400kHz、PT100、A0/A1/A2、開始SWデバウンス・ブザーは維持。D2/D12は全状態でLOW固定です。
書き込みはユーザーの安全状態確認後に行い、最初の開始SW操作はユーザー自身が行います。Codexから開始操作・駆動指示は送信しません。結果はHANDOFFへ記録します。

## 実装済み機能

- A0/A1/A2を100ms周期で読み取り、指数移動平均で平滑化して設定範囲へ変換。
- PT100の分圧値から抵抗値と温度を算出。0～850℃範囲の式を使用し、不正値はERR。
- VL53L0Xを同期測距し、RangeStatus=0かつ距離が正の場合のみ有効と判定。
- 起動時のI2Cスキャン、OLED/ToF初期化結果のログ。
- OLEDは約250ms周期で更新。設定値・センサ値・SW・安全出力を1画面に常時表示し、ページ切替は行わない。
- 開始SWは30msのデバウンス、押下遷移で100msのブザー。
- Serialは115200bps、1秒ごとに状態ログを出力。
- millis()で更新周期を管理。ToFライブラリ内部の同期処理は待ち時間を伴います。

## OLEDの表示仕様

SSD1306（128×64、0x3C）の通常表示は、Adafruit標準フォントの `setTextSize(1)` を使った6行です。
設定温度・高さは整数、設定時間・PT100実測温度は小数1桁、ToF距離は整数で表示します。
設定値はA0/A1/A2の既存の読取・平滑化処理を経た値を使い、次のOLED更新に反映します。

```text
Tset 175C  2.8s
Hset  68mm
Temp 18.3C
ToF  65mm
SW HIGH
SSR OFF  PWM 0
```

文字の送り幅6ピクセル・高さ8ピクセルで、各行の開始座標を固定し、折り返しを無効にしています。
正常時は最長16文字＝96ピクセル幅、診断表示を含めても最長18文字＝108ピクセル幅です。
最下行の下端はY=57で、画面内に収まります。

| 行のY座標 | 最大桁数の表示例 | 文字数／幅 |
|---|---|---|
| 0 | `Tset 300C  10.0s` | 16文字／96px |
| 10 | `Hset  300mm` | 11文字／66px |
| 20 | `Temp 850.0C` | 11文字／66px |
| 30 | `ToF  65535mm` | 12文字／72px |
| 30（異常時） | `ToF  SENSOR ERR S5` | 18文字／108px |
| 40 | `SW HIGH` | 7文字／42px |
| 50 | `SSR OFF  PWM 0` | 14文字／84px |

ToFの最大桁数は格納型 `uint16_t` の上限を使った表示幅の確認値で、センサの有効測距範囲を示しません。
PT100が異常の場合は `Temp ERR`、ToFが異常の場合は下表の診断ラベルを表示し、前回の数値を表示しません。
SWはデバウンス後のHIGH／LOWです。SSR OFFは固定指令、PWMは現在の指令値です。端子電圧の実測値ではありません。
PT100抵抗・ADC・ToF status・初期化結果は従来どおりSerialで確認できます。
Serialの内容・1秒周期、温度計算、設定範囲、ピン、安全処理は変更していません。
2026-10-14に正本のverifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 33,872 bytes／静的RAM 1,483 bytes）。
同日20:32（日本時間）、ユーザーの書き込み指示・モータと加熱電源の切り離し確認後に、
commit `647daec490491eaf9ba732d4306038257d48216b` の1画面表示版をMega 2560 COM4へ書き込みました。
uploadの `--verify` 付き実行が終了コード0となり、フラッシュ検証に成功しました。
115200bpsで約12秒のSerialログを取得し、0x29/0x3Cの検出、OLED_INIT=OK、TOF_INIT=OK、SSR=OFF、MOTOR_PWM=0を確認。
コード上のレイアウト確認と実機の初期化・ログ確認は完了し、OLEDの6行表示・重なり・更新の目視確認は未実施です。

ToFの診断表示（`S`はRangeStatus、負の数は測距APIの戻り値）:

| OLEDのToF行 | 意味・確認する条件 |
|---|---|
| `ToF  65mm` | API成功・status=0・距離が正の正常測距。 |
| `ToF  NOT FOUND` | 起動時に0x29を検出できなかった。配線・電源・接続を確認。 |
| `ToF  INIT ERR` | 0x29は検出したが、ToF初期化が失敗。 |
| `ToF  I2C ERR -20` | 測距APIが通信インターフェースエラーを返した。 |
| `ToF  TIMEOUT -7` | 測定処理が時間切れ。距離の近い／遠いを示すコードではない。 |
| `ToF  READ ERR -6` | その他のAPIエラー。末尾の実際の戻り値で切り分ける。 |
| `ToF  UNSTABLE S1` | 測定のノイズ・ばらつきが大きい（Sigma判定）。 |
| `ToF  WEAK/FAR? S2` | 反射信号が弱い。遠い・対象なし・反射しにくい等の可能性があり、遠すぎるとは断定しない。 |
| `ToF  NEAR? S3` | 最小範囲判定に失敗。近距離側の条件を確認する目安で、近すぎるとは断定しない。 |
| `ToF  RANGE? S4` | 距離の確定に失敗（位相判定）。距離の曖昧さ・ノイズ等で発生し、遠すぎるとは断定しない。 |
| `ToF  SENSOR ERR S5` | センサが内部ハードウェア異常を報告。 |
| `ToF  NO DATA S255` | 測距データの更新なし。 |
| `ToF  BAD DATA S0` / `ToF  UNKNOWN S...` | status=0でも距離が0、または未知のstatus。数値を有効扱いしない。 |

`?` は原因の推定を表します。VL53L0Xのstatusだけでは近すぎる／遠すぎるを常に区別できません。
仕様の根拠は[ST UM2039のRange Status表（16ページ）](https://www.st.com/resource/en/user_manual/um2039-world-smallest-timeofflight-ranging-and-gesture-detection-sensor-application-programming-interface-stmicroelectronics.pdf)と
[使用ライブラリ1.2.5のエラー定義](https://github.com/adafruit/Adafruit_VL53L0X/blob/1.2.5/src/vl53l0x_def.h)です。
測距APIの戻り値を確認し、API失敗時は測定構造体や前回値を使いません。
Serialの既存フォーマット・1秒周期は維持し、詳細なAPIコードはOLEDへ表示します。
API失敗時のSerialのTOF_STATUSは255（値未取得）となるため、OLEDの診断表示も併せて確認してください。
距離の近い／遠いを決め打ちするしきい値や、測距設定・有効範囲は追加・変更していません。
2026-10-14にこの診断版もclean compile成功（Flash 34,258 bytes／静的RAM 1,483 bytes）し、
21:06（日本時間）にCOM4へ書き込み・フラッシュ検証成功。約12秒のログでOLED_INIT=OK、TOF_INIT=OK、
TOF_STATUS=2、SSR=OFF、MOTOR_PWM=0を確認しました。通信エラーなど他の異常状態の実機再現試験は未実施です。

## ToFの標準設定: RIT無効（400kHz）

2026-10-28、ユーザーの指示により `TOF_RIT_ENABLED=false` を正式な標準設定として採用しました。
`I2C_CLOCK_HZ=400000UL` と現在の測距処理・既存診断・OLED等の機能を維持します。D2/D12=LOW、D9のPWM=0も維持します。
距離急変監視、連続正常判定、追加停止ロジック、試験機能は追加しません。
ToFの測距不良対応はいったん完了とし、次の製作工程へ進みます。今回、追加の実機uploadは行いません。
既存のRIT無効版は20:06に書き込み・フラッシュ検証済みで、起動時の要求0・読み戻し一致・TOF_SETTINGS=OKを確認しています。
実機確認の履歴・設定値は[HANDOFF](docs/HANDOFF.md)、既存の診断項目・単位は[診断資料](docs/TOF_DIAGNOSTICS.md)を参照してください。

## ToF測距診断と別試験の100kHz/400kHz比較

切り替えは `config.h` の `I2C_CLOCK_HZ` を `400000UL`（既定）または `100000UL` に変更します。
ToF初期化後に設定を再適用し、OLEDの転送中・転送後のクロックも揃えます。
従来はライブラリの初期化やOLED更新で100kHzへ戻る経路があり、400kHz設定の記述だけでは測距時の速度を保証できませんでした。

既存の1秒周期Serial項目へ、診断用距離、API戻り値、信号・周囲光（MCPS）、有効SPAD数、未換算整数、取得可否の理由を追記します。
rateは16.16固定小数点を65536で、SPADは8.8形式を256で割ります。API失敗・未実行時の診断値は `NA` です。
`TOF_N` / `TOF_OK` / `TOF_API_ERRS` / `TOF_BAD_RANGE` / `TOF_STATUS_COUNTS` はすべての測距API呼び出しを集計します。
距離ごとにSerialへ `r` を送り、`TOF_STATS_RESET` 後の区間を記録できます。測距設定や出力ピンは変更しません。
成功条件は従来どおりstatus=0かつ距離>0で、OLEDの6行表示と異常診断ラベルも維持しています。

白いA4コピー用紙を50/80/100/150/300mmで平行に固定し、各速度で最低30秒かつ100試行以上を記録する計画です。
診断値・集計の読み方、10条件の記録表、固定すべき条件、光学窓・電源ノイズ・測距設定・個体差など次の切り分けは
[ToF診断手順](docs/TOF_DIAGNOSTICS.md)を参照してください。
100kHz・400kHzの両設定で正本のverifyによるクリーンcompile成功（Flash 36,272 bytes／静的RAM 2,541 bytes）。
診断400kHz版は2026-10-21にCOM4へ書き込み・フラッシュ検証成功。2026-10-28の起動ログでも `I2C_CLOCK_HZ=400000`、診断値・集計、安全出力のOFF/0を確認しました。対象条件を揃えた距離・速度比較と負荷通電試験は未実施で、原因は未特定です。詳細は[引き継ぎ記録](docs/HANDOFF.md#tof診断400khz版の書き込みと起動確認)を参照してください。

## ピンアサイン

| ピン | 用途 | 現行の仕様 |
|---|---|---|
| A0 | 温度設定可変抵抗 | 150～300℃ |
| A1 | 時間設定可変抵抗 | 1～10秒 |
| A2 | 高さ設定可変抵抗 | 0～300mm |
| A7 | PT100 | 実物確認した1kΩ固定抵抗とPT100の分圧入力 |
| D2 | ホットプレート用SSR | `PIN_SSR_HOTPLATE=2`。HIGH=ON／LOW=OFF想定（極性未確認）、現行は**LOW固定** |
| D4 | 開始SW | 内蔵プルアップ（INPUT_PULLUP）、NO接点、通常HIGH／押下LOW |
| D5 | ブザー | 暫定でアクティブ、HIGH=ON |
| D8 | MD20A DIR | HIGH=上昇、LOW=下降（実機確認済み）。起動時LOW、PWM=0 |
| D9 | MD20A PWM | 開始SWによる移動時のみDuty 100%、起動・待機・停止・保持・ToF無効時0 |
| D12 | 焼印用SSR | `PIN_SSR=12`。HIGH=ON／LOW=OFF想定（極性は実機確認対象）、現行は**LOW固定** |
| D20 | I2C SDA | OLEDとToF共用 |
| D21 | I2C SCL | OLEDとToF共用 |

## ホットプレート用SSR（D2）

ピン定義は `config.h` に集約しています。起動時は `digitalWrite(PIN_SSR_HOTPLATE, LOW)` で
出力ラッチをLOWにしてから `pinMode(PIN_SSR_HOTPLATE, OUTPUT)` を実行し、
`enforceSafeOutputs()` を初期化時と毎loopで呼び出してLOWを維持します。
**今回のI/O確認版は通常待機中もD2=LOW固定です。正常待機中ONや状態管理、自動焼印シーケンスは未実装です。**
起動・定期Serialログに `HOTPLATE_SSR=OFF` を追加し、既存項目と1秒周期を維持しています。
OLEDの既存6行表示は変更していません。ログのOFFは制御出力の設定で、実測結果ではありません。

制御側の配線割り当てはD2→ホットプレート用SSR入力、D12→焼印用SSR入力、D9→MD20A PWMです。
HIGH有効のDC入力で直接駆動できると確認できた場合の配線案は、D2→SSR入力＋、Mega GND→SSR入力−です。
入力端子・GND接続・駆動回路の要否、負荷側の配線と定格は採用SSRの仕様と現物で確認し、未確認のまま直結・通電しません。

将来の状態管理で実装する仕様（今回は有効化しません）:

| 動作状態 | ホットプレート用SSR |
|---|---|
| 電源投入直後・初期化中 | OFF |
| 初期化完了後の正常待機 | ON |
| 焼印下降中 | OFF |
| 焼印押し付け中 | OFF |
| 焼印上昇中 | OFF |
| 正常終了して待機状態に復帰 | ON |
| 異常停止・制御状態不明 | OFF |

焼印シーケンス開始時、下降する前にOFFへ切り替え、下降・押し付け・上昇の全期間でONにしません。
正常終了を確認し、正常待機状態へ復帰してからONにします。OFF/ONの条件は将来の状態管理に基づいて実装します。
Arduinoはホットプレートの電源ON/OFFのみを担当し、温度調節はホットプレート本体に任せます。
PT100によるホットプレートの温度制御は行いません。

LOW設定だけでは物理的な安全性を保証できません。次を実機確認の残件とします。

- 電源投入・リセット・ブートローダ動作中から初期化完了までのD2/D12の出力状態、D9のPWM=0。
  OUTPUT設定前の入力状態も含めて測定し、外付けプルダウンの要否・抵抗値と実装を確認する。
- SSRの入力極性、入力電圧・駆動電流とMega出力の適合、負荷電圧・電流・突入電流・放熱を含む負荷定格。
- ホットプレートが電源遮断後の再投入で自動復帰するか。未確認であり、将来の電源ON/OFF運用の成立条件として確認する。

2026-10-21、固定版の専用環境で `scripts/verify_environment.ps1` によるMega 2560向けクリーンcompile成功
（終了コード0、Flash 34,316 bytes／静的RAM 1,483 bytes）。コード上でピン競合なし、D2/D12=LOW・D9のPWM=0、
既存のOLED・センサ・開始SW・ブザー・周期処理とSerial既存項目の維持を確認しました。
同日20:05（日本時間）、ユーザーの指示と全負荷電源の切り離し確認後、commit `e3545ef689b5b924adb3a997e282bfdca6b632cb` の
検証済みHEXをMega 2560 COM4へ `--verify` 付きでupload成功（終了コード0、34,316 bytesのフラッシュ照合成功）。
USBシリアルの初回通信エラーはUSBの再接続後に解消し、コア・ライブラリ・ドライバは変更していません。
20:06から115200bpsで約12秒の起動・定期ログを取得し、HOTPLATE_SSR=OFF、SSR=OFF、MOTOR_PWM=0を11行で確認しました。
OLED_INIT=OK、I2C検出は0x3CのみでTOF_INIT=ERRでした。0x29未検出時の接続・電源条件は未確認です。
モータ・ヒーター・SSR負荷の通電試験とD2/D9/D12の端子電圧実測は行っていません。詳細は[HANDOFF](docs/HANDOFF.md#d2追加版の書き込み起動確認2026-10-21)を参照してください。

## ハードウェア

- MCU: Arduino Mega 2560 Rev3（ATmega2560、FQBN `arduino:avr:mega:cpu=atmega2560`）。
- OLED: 秋月電子115870、0.96インチ、128×64、SSD1306。コードのアドレスは0x3C。
- ToF: M5Stack Unit ToF、VL53L0X、コードのアドレスは0x29。共有I2Cは `I2C_CLOCK_HZ` で400kHz/100kHzを選択（既定400kHz）。
- モータドライバ: MD20A。アクチュエータの全ストロークは約60秒との情報で、実測値は未確定。
- 加熱: SSR経由の焼き印用はんだごて。SSR・電源等の型番と配線詳細は未記録。
- ホットプレート: 専用SSRをD2へ追加する割り当て。温度調節は本体側。SSRの型番・配線適合と電源再投入時の自動復帰は未確認。
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
開始SWはD4の内蔵プルアップ（`INPUT_PULLUP`）とGNDへのNO接点を使用します。外付け抵抗は不要です。
2026-10-14に「離しても押してもLOW」との報告を受け、外付け抵抗が入っていないことをユーザーが確認しました。
従来の外付けプルアップ前提の `INPUT` 設定では、開放時の入力が不定になるため、ユーザー指定で内蔵プルアップへ変更。
スイッチの一端をD4、もう一端をGNDへ接続し、離すとHIGH、押すとLOWです。
30msデバウンス・押下時100msブザー、SSR=LOW・PWM=0の安全処理は変更していません。
同日20:53（日本時間）に内蔵プルアップ版のclean compile・COM4へのuploadとフラッシュ検証に成功しました。
Flash 33,872 bytes／静的RAM 1,483 bytes。115200bpsで約15秒のログを取得し、START_SWのHIGH/LOW切替、
OLED_INIT=OK、TOF_INIT=OK、SSR=OFF、MOTOR_PWM=0を確認しました。
ユーザーがOLEDで「離すとHIGH・押すとLOW」、押下時にブザーが鳴ることを確認し、開始SWのLOW固定問題は解決済みです。
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
起動時に `DORAYAKI_IO_CHECK_START`、`SAFETY: SSR=OFF, MOTOR_PWM=0, HOTPLATE_SSR=OFF`、
`OLED_INIT=OK`、`TOF_INIT=OK` を確認します。
定期ログの `SSR=OFF,MOTOR_PWM=0,HOTPLATE_SSR=OFF` はコード上の状態表示で、端子電圧の実測ではありません。

## 次の実機テスト・未確定項目

2026-10-07に受領したユーザー実機確認記録と、2026-10-14・2026-10-21のupload・Serialログ取得結果を反映しています。

| 項目 | 状態 | 確認結果・残件 |
|---|---|---|
| Mega 2560 | 診断400kHz版のupload・起動確認済み | 2026-10-21にCOM4へ診断400kHz版を書き込み、36,272 bytesのフラッシュ検証成功。2026-10-28に起動ログ確認。次PCではCOM番号を再確認する。 |
| OLED / I2C | 配線修正済み・初期化OK | SDA→D20、SCK→D21へ修正。2026-10-28は0x29/0x3Cの共存・初期化OK、400kHzのソフト設定を確認。SCL波形は未実測。 |
| PT100 | 1kΩ修正版の数値表示確認済み・校正未実施 | 固定抵抗は実物確認済みの1kΩ、設定1000.0Fは正しい。内蔵プルアップ版のログはADC=99～101、R=107.14～109.54Ω、TEMP_ACT=18.3～24.5℃。 |
| ToF | 初期化OK・S2多発・距離条件未確定 | 20:06のログは0x29未検出。その後のユーザー確認は0x29検出・TOF_INIT=OK、電源約4.78V、SDA/SCLアイドル約3.3V。過去の距離申告は実測条件が未記録で、測定限界としては未確定。診断400kHz版は書き込み済み。2026-10-28の起動ログは初期化OK、APIエラー0、status 0と2が混在。対象条件は未記録で、距離・速度比較は未実施。 |
| 開始SW | LOW固定問題は解決済み | 外付け抵抗の未接続を確認し、D4をINPUT_PULLUPへ変更。ユーザーがOLEDで離すとHIGH・押すとLOW、押下時ブザーを確認。SerialでもHIGH/LOW切替を記録。 |
| D9 / D12 | ログ上安全・端子電圧実測待ち | SSR=OFF、MOTOR_PWM=0を確認済み。現行I/O版のD12=LOW、D9のPWM=0を維持。実端子電圧測定は未実施。 |
| D2 / ホットプレート用SSR | 書き込み・起動ログ確認済み、端子電圧実測待ち | 現行I/O版はLOW固定。2026-10-21にCOM4へupload・フラッシュ検証成功、HOTPLATE_SSR=OFFを確認。通電・加熱試験は未実施。起動時出力、外付けプルダウン、入力極性・駆動電流・負荷定格、本体の電源再投入時自動復帰を確認する。 |
| A0 / A1 / A2 | 実機結果未記録 | 両端・中間の表示を確認する。 |
| MD20A方向確認 | ユーザー確認済み | HIGH=上昇、LOW=下降。約50%では遅いとの報告を受け本番PWMを255へ変更。その後、上昇時の距離増加と300mmまでの移動可能をユーザーが確認し、自動運転を有効化。下限は指定の暫定50mm。 |

OLEDが認識されなかった原因は、OLED SDAとSCKが両方Mega D20へ接続されていた誤配線です。
Mega 2560のD20=SDA、D21=SCLに合わせ、OLED SDA→D20、OLED SCK→D21へ修正しました。
OLEDは秋月電子115870（SSD1306、128×64、0x3C）、ToFはVL53L0X（0x29）です。
配線修正後の起動ログ:

```text
I2C_DEVICE=0x29
I2C_DEVICE=0x3C
I2C_DEVICE_COUNT=2
OLED_INIT=OK
TOF_INIT=OK
```

OLED修正後ログを含むPT100の数値表示から、読み取り・抵抗換算・温度換算はI/O確認用途として正常動作と判断します。
表示温度はコードの算出値です。基準温度計との比較・校正、最終温度制御精度の確認は未実施です。
ToFのstatus=2／ERRはセンサ故障とは扱わず、取付位置・対象面・距離条件を決めた後に測距条件を評価します。
2026-10-07に受領した配線修正・ログ確認結果の詳しい測定日時、ToFの対象条件、PT100の基準温度は未記録です。

2026-10-14の1画面表示版のログではPT100_ADC=98～100、PT100_R=105.95～108.34Ω、TEMP_ACT=15.2～21.4℃を確認しました。
TOF_INIT=OKですが、取得した定期ログはTOF_STATUS=2（TOF=ERR）で、測距対象条件は未記録です。
START_SWはHIGH/LOWの両状態を記録しましたが、スイッチの実際の操作との対応は記録していません。
SSR=OFF、MOTOR_PWM=0は起動・定期ログで確認済みです。端子電圧実測、PT100校正、OLED目視確認は未実施です。

配線修正前の履歴として、2026-10-07 21:01（日本時間）までに、commit `25ca1f47b65dd31bf45c5661865212611dcabde7` の1kΩ版を
clean compileし、COM4のMega 2560へupload成功（終了コード0）。モータ・加熱電源の切り離しはユーザー確認済みです。
115200bpsで約12秒のSerialログを取得し、起動ログ、SSR=OFF、MOTOR_PWM=0を確認しました。
PT100_ADC=98～99、PT100_R=105.95～107.14Ω、TEMP_ACT=15.2～18.3℃と表示されました。
この時点ではI2C 0x29のみ、OLED_INIT=ERR、TOF_INIT=OK、TOF_STATUS=2（TOF=ERR）、START_SW=LOWでした。
OLED・開始SWの配線修正後の状態は上表のとおりです。

次の実機確認はモータ・加熱の電源を切り離し、以下の順序で進めます。

1. D2/D9/D12の端子電圧実測。D2追加版は書き込み・フラッシュ検証・起動ログ確認済みだが、実端子電圧は未測定。
2. START_SWの通常HIGH／押下LOWの遷移ログとブザー動作確認。
3. A0/A1/A2の両端・中間確認。
4. PT100を基準温度計と比較。
5. ToFはRIT無効を標準設定として採用し、測距不良対応はいったん完了。次の製作工程へ進む。
6. 以上を記録した後、別途明示したMD20A単体試験。

MD20AのHIGH=上昇／LOW=下降はユーザーの単体動作で確認済みです。上下制御はDuty 100%、確認済みフラグtrueです。起動・待機はPWM=0で、最初の開始SW操作はユーザー自身が行います。
詳細と記録方法は[HANDOFF](docs/HANDOFF.md)を参照してください。

DIR極性、ToF距離と上下方向、取付位置・原点距離、全ストローク所要時間、
ブザー種別、PT100の実測精度・自己発熱・校正が未確定です。
上下1サイクルとToF無効時の停止は実装済み・既定無効です。温度制御、移動タイムアウト、自動異常復帰は未実装です。
