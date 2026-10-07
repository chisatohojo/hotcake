# 開発引き継ぎ

2026-10-07現在、**第1段階 I/O・センサ確認版まで完成**。
このWindows PCでは `.arduino-local/` に開発環境を分離し、Mega 2560向けクリーンcompileに成功しました。
ユーザーからCOM4へのupload、起動ログ、ToF測距等の実機確認結果を受領しています。
同日21:01（日本時間）までに、ユーザーの書き込み指示と電源切り離し確認を受け、
1kΩ修正版をCOM4へuploadし、115200bpsの起動・定期ログをエージェントが取得しました。
OLED初期化ERR、開始SWのLOW固定、D9/D12の端子電圧未実測が残っています。
ユーザーの確認結果と、エージェントが実行したcompile・upload・ログ取得を区別して記録します。

## 完了済み

- A0/A1/A2設定値の読取と平滑化、PT100抵抗・温度算出、ToF同期測距。
- I2Cスキャン、OLEDの2ページ表示、115200bpsの定期Serialログ。
- 開始SWのデバウンスと押下時ブザー。
- 起動時とloopで安全出力を維持する処理。
- 実環境を根拠にしたバージョン固定、Windows構築・検証スクリプトと構築文書。
- `.tools/` で使っていた専用環境の設定を `scripts/common.ps1` へ移植。
  setup/verifyが自動で `.arduino-local/` を選び、CLIのユーザーZIP配置も検出する。
- 汎用のVS Code設定とタスクをGit管理。Ctrl+Shift+Bで正本のverifyを直接実行。
- 固定抵抗の実物確認に基づき、PT100分圧用抵抗を1kΩ（`1000.0F`）へ修正。
- ソースと過去の手書き資料を保存。バージョン・検証条件は[ENVIRONMENT](ENVIRONMENT.md)を参照。

## 安全上の重要事項

**現在のI/O確認版では、SSRは絶対にLOW、モータPWMは絶対に0。**
D8は暫定LOWですが、上下方向の対応は未確定です。
SSRのHIGHは加熱ONなので、安全状態を切り替える試験コードをこの版へ混ぜないでください。
開始SWはブザーのみを動かし、焼印シーケンスを開始しません。

`enforceSafeOutputs()` は設定後と毎loopで安全出力を再設定します。
ただし、電源投入からスケッチ開始までの端子状態、配線誤り、SSR・ドライバ故障はソフトだけでは保証できません。
最初は加熱・モータ電源を切り離し、D9/D12を実測してから負荷側の接続を判断します。
SerialログのOFF/0だけで端子が安全と判断しないでください。

PT100は5V・1kΩ固定抵抗の直接分圧です。従来文書の100Ωは実物確認結果により訂正しました。
`PT100_SERIES_RESISTOR_OHM=1000.0F`、PT100の0℃基準抵抗は `PT100_R0_OHM=100.0F` です。
抵抗の詳細な実測値・公差、基準温度との比較、自己発熱・抵抗定格・測定精度は確認が必要です。
コードのPT100有効判定は保護制御ではありません。加熱制御へ転用する前に異常処理を設計してください。

## 次の実機確認順序

1. モータ・加熱電源を切り離し、1kΩ修正版の最新compile成果物をMegaへ人間が書き込む。
   COM4はこのPCの確認値で、次PCではボードとポートを再照合する。
2. 電源投入・リセット・開始SW押下時もD9がPWM=0、D12がLOWであることを実測。
3. OLEDの配線・電源・アドレスを確認。現在はI2C 0x29のみでOLED_INIT=ERR。
   0x3C/0x29の検出、OLED_INIT/TOF_INIT、ToFの対象条件とstatus=2発生条件を記録する。
4. A0/A1/A2の両端・中間を確認。温度150～300℃、時間1～10秒、高さ0～300mm。
5. PT100を常温で基準温度計と比較。分圧抵抗実測値、ADC、自己発熱、誤差を記録。断線・短絡時ERRも確認。
6. START_SWがLOWのままになる配線を確認。通常HIGH／押下LOW、NO接点・外付けプルアップを照合する。
   30msデバウンスと100msブザーを確認し、ブザー種別・論理を確定。
7. 以上の記録を残した後、**別の明示的なMD20A単体試験**に進む。現行I/O版はPWM=0のまま保存する。

MD20A試験は**PWM 50%から開始予定**で、現行スケッチでは実行できません。
方向・可動域・停止手段を現物確認して試験用コードを準備します。
DIR HIGH/LOWと上下方向、ToF距離の増減と移動方向、全ストローク所要時間を実測して確定します。
約60秒はユーザー申告の目安です。上端・下端距離、負荷、速度条件、経過時間を記録してください。

## 未完了・未確定

- D9/D12の端子電圧実測、OLEDの配線・電源・アドレス、開始SWのLOW固定の解消。
- PT100の基準温度計との常温比較・校正。1kΩ修正版のuploadと数値表示は確認済み。
- ToF取付位置、測定対象面、原点距離、距離から高さへの換算・有効測距範囲。
- リミットSWなしでの原点復帰、測距異常時停止、移動タイムアウト、再起動・異常復帰。
- DIR極性、実ストローク・移動時間・適切なPWM。50%は試験予定値。
- PT100精度・校正・入力回路の妥当性、ブザーの種別。
- SSR型番、電源・駆動側配線、温度制御・温度異常時停止。
- 押し付け時間・高さを使う自動焼印シーケンス。

## 再開方法

[NEW_PC_SETUP](NEW_PC_SETUP.md)の順に構築し、setupとverifyが終了コード0になることを確認。
CLI 1.5.1、AVR Boards 1.8.8、指定4ライブラリは `scripts/environment.lock.json` が正本です。
次PCへはGitHub mainをcloneし、キャッシュをコピーせず専用環境を再構築します。
`.tools/` はローカル互換用のみで引き継ぎ不要です。正本のコマンド:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1 -InstallExtensions
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_environment.ps1
```

2026-10-07に、共有ファイルだけを空白を含む別フォルダへコピーし、`.tools/` なし・空の
`.arduino-local/` からsetupを実行して、固定パッケージの導入とclean compileが終了コード0になることを確認。
CLIとVS CodeはこのPCの導入済みツールを使用した検証で、Windows自体の新規インストール試験ではありません。
既存のグローバルArduino環境を保持し、Flash 34,146 bytes／静的RAM 1,488 bytesでした。

新PCでCodexへ最初に渡す指示:

```text
https://github.com/chisatohojo/hotcake.git のmainをこのPCへ引き継ぎます。
リポジトリを取得し、AGENTS.md、README.md、docs/HANDOFF.md、docs/NEW_PC_SETUP.md、
scripts/environment.lock.jsonとgit status・git diffを確認してください。
現在の第1段階I/O版のSSR=LOW、モータPWM=0を維持してください。
PT100の分圧用固定抵抗は実物確認済みの1kΩで、設定は1000.0Fです。
実環境を調査し、不足する環境をNEW_PC_SETUP.mdとenvironment.lock.jsonに従って構築し、
VS Code推奨拡張を導入してscripts/setup_windows.ps1とscripts/verify_environment.ps1を実行してください。
両スクリプトが自動選択する.arduino-local/を使い、既存Arduino環境を上書きしないでください。
.tools/やPC固有のキャッシュは不要です。Ctrl+Shift+Bのタスクも確認してください。
管理者操作とログインが必要なら具体的な手順を示してください。
ソースを変更せず、Mega 2560向けクリーンcompile結果と版番号を報告してください。
COM4は前PCのポートです。実機uploadやモータ駆動・加熱動作は行わないでください。
OLED初期化ERR、START_SWのLOW固定、端子電圧未実測、PT100校正未実施の残件を引き継いでください。
```

実機試験の記録を得たら、下表とREADMEの現在段階・未確定項目を更新してください。

| 項目 | 状態 | 日時・測定条件・結果 |
|---|---|---|
| このPCのcompile | 成功 | 2026-10-07（日本時間）、Windows x64／PowerShell 5.1。CLI 1.5.1、AVR 1.8.8、固定4ライブラリを `.arduino-local/` に導入。1kΩ修正後に正本のverifyでclean compile成功、終了コード0。Flash 34,146 bytes／静的RAM 1,488 bytes。 |
| Mega upload / 起動 | 修正版確認済み | 2026-10-07 21:01（日本時間）までに、commit `25ca1f47b65dd31bf45c5661865212611dcabde7` の1kΩ版をclean compileし、Mega 2560 COM4へupload成功、終了コード0。モータ・加熱電源切り離しはユーザー確認済み。115200bpsで約12秒の起動・定期ログを取得。前回ユーザー確認分の書き込み元の版・測定日時は未記録。 |
| D9 / D12 | ログのみ確認 | SSRログ上OFF、モータPWMログ上0。端子電圧実測は未実施。 |
| OLED / I2C | 要確認 | OLED初期化ERR。I2C scanは0x29のみ検出。OLED配線・電源・アドレス確認が必要。 |
| ToF | 一部確認済み | 前回ユーザー確認では初期化OK、62～65mm・status=0、対象条件によってstatus=2。今回修正版のログもTOF_INIT=OKで、取得した定期ログはTOF_STATUS=2（TOF=ERR）。対象材・距離・角度等の詳細条件は未記録。 |
| A0 / A1 / A2 | 未記録 | 両端・中間の表示 |
| PT100 | 修正版数値表示確認 | 前回ユーザー確認はPT100_ADC=100、実測・実物確認した固定抵抗は1kΩ。設定を1000.0Fへ修正後、今回ログはADC=98～99、R=105.95～107.14Ω、TEMP_ACT=15.2～18.3℃。コードの算出・表示値であり、基準温度計との比較・校正は未実施。 |
| SW / ブザー | 要確認 | START_SWはログ上LOWのまま。配線・外付けプルアップ・NO接点確認が必要。押下遷移・ブザー動作は未記録。 |
| MD20A単体試験 | 未実施 | DIR、ToF増減、ストローク時間、PWM、停止方法 |
