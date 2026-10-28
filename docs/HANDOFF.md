# 開発引き継ぎ

## 自動運転有効版の書き込み・起動確認（2026-10-28）

commit `624c905ccf79f0df3201f525f2dad9a67905a2df` を通常push後、ユーザーのヒーター・両SSR負荷切り離し、可動部周囲の安全、非常停止使用可能の確認を受けてCOM4のMega 2560へ書き込みました。
コンパイル済みconfigとの一致を確認し、upload --verify成功（終了コード0、ATmega2560署名1E 98 01、39,024 bytesのフラッシュ照合一致）。当初COM4は使用中でしたが、ユーザーがモニタを閉じた後に書き込みを1回実行しました。
20:44:20（日本時間）から115200bpsで約12秒の起動ログを取得。OLED_INIT=OK、TOF_INIT=OK、RIT_REQUEST=0／RIT_MATCH=YES、TOF_SETTINGS=OK、I2C_CLOCK_HZ=400000を確認しました。
全11行でACTUATOR_SETUP_CONFIRMED=1、ACTUATOR_STATE=IDLE、MOTOR_PWM=0、SSR=OFF、HOTPLATE_SSR=OFF。最後の完全行はN=82、OK=82、API_ERRS=0、BAD_RANGE=0。距離スナップショットは300～306mmでした。
開始SWはHIGH、設定表示は時間3.6秒・高さ149mm。未開始のCYCLE_TARGET_MM=50／CYCLE_HOLD_MS=0は初期値で、サイクル開始時にA1/A2から固定されます。
Codexは開始SW操作・H/L等の駆動指示を一切行っていません。最初の自動運転と実動作確認はユーザー自身が行います。Serialポートは閉じています。端子電圧は今回実測していません。
現在の実機は一時方向確認スケッチから本番の自動上下運転有効版へ更新済みです。生ログ・書き込み結果はGit管理外の `build/verify-52254bae8558439e9e4e21ddb22a81c2/` に保存しました。


## 現在: 開始SWによる自動上下運転を有効化（2026-10-28）

ユーザー確認済みの報告: HIGH=上昇、LOW=下降、上昇時にToF距離が増加、300mmまで機械的に移動可能、Duty 100%を採用。
ユーザー指示によりACTUATOR_SETUP_CONFIRMED=trueへ変更。PWM255、上昇HIGH／下降LOW、上限300mm、暫定下限50mmを維持します。
既存5状態、開始時のA1/A2固定、ToF無効時停止、停止・保持PWM=0は変更なし。RIT無効・400kHz、既存機能、D2/D12 LOWも維持。
新しい制御・監視・試験機能は追加しません。ヒーター・両SSR負荷の切り離し、可動部周囲の安全、非常停止が使用可能との確認をユーザーから受領しました。
commit・通常push後に本番スケッチを書き込み・フラッシュ検証し、起動時のToF初期化・確認フラグ1・待機PWM=0だけを確認します。開始操作・H/L送信・自動駆動は行わず、最初の運転はユーザー自身が行います。
正本verifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 39,024 bytes／静的RAM 2,570 bytes）。
以降の自動運転無効・一時スケッチ・確認待ちの記載は履歴です。現在の設定は本節とconfig.hを優先します。


## 現在の本番設定: Duty 100%・DIR確認済み、自動運転は無効（2026-10-28）

ユーザーの短時間駆動結果を受領: DIR=HIGH（H）は上昇、LOW（L）は下降。約50% Dutyでは遅いとの報告によりMOTOR_PWM_RUN=255（100%）へ変更しました。
MOTOR_DIR_UP_LEVEL=HIGH、下降LOWを実機確認済みとして記録します。これはユーザー報告で、今回エージェントによる駆動試験は行っていません。
既存5状態、開始時のA1/A2固定、上昇300mm、下降下限50mm、ToF無効時停止、停止・保持PWM=0は変更なし。
ACTUATOR_SETUP_CONFIRMED=falseを維持。上昇でToF距離が増えること、300mmと下限50mmが可動範囲内で接触限界を越えないことを確認後にのみtrueへ変更する方針です。
RIT無効・I2C 400kHz・センサ/表示/開始SW/ブザー・D2/D12のLOW固定を維持。テストプログラムや監視ロジックは追加しません。
今回、実機upload・モータ通電・自動運転は行いません。実機に書き込み済みなのは下記の方向確認用一時スケッチ（PWM128）のままで、本番のPWM255設定とは区別してください。
正本verifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 38,508 bytes／静的RAM 2,557 bytes）。
以下の「DIR未確認」「約50%」は過去の経緯です。最新の本番設定は本節を優先します。


## 現在の実機: 初回方向確認用の別スケッチ書き込み済み（2026-10-28）

ユーザーの書き込み指示と、モータ・ヒーター・両SSRの負荷電源切り離し／USB給電の確認後、COM4のMega 2560（VID=2341/PID=0042）を再検出しました。
`build/md20a_direction_check/` のソース・config・HEXをcompile時のSHA256と照合し、別スケッチをupload --verifyで1回書き込み成功（終了コード0、19,968 bytesのフラッシュ照合一致、ATmega2560署名1E 98 01）。
20:33:57（日本時間）から115200bpsで約7秒の起動ログを取得。MD20A_DIRECTION_CHECK_TEMPORARY、PWM=0、HOTPLATE_SSR=OFF、SSR=OFF、AUTO_CYCLE=DISABLED、TOF_INIT=OK、RIT_REQUEST=0／SET_API=0／GET_API=0／RIT_MATCH=YES、READYを確認しました。
H/Lは一切送信せず、モータ・ヒーター通電は未実施。方向・ToF距離増減・端子電圧は未確認です。Serialポートは閉じています。
本番スケッチとconfigは69cef55から変更なくACTUATOR_SETUP_CONFIRMED=false。現在の実機は本番の上下シーケンス版ではなく、H/Lで300ms動く一時スケッチです。
記録は同一時ディレクトリのupload-result.json、startup-115200.log、startup-result.jsonへ保存。HEX SHA256は2F6C260B59ECF32054F798F7A61D808ACE4A9E8D93907510F48097EE45884B75。
方向確認はヒーター切り離し・可動部から手を離す・既存非常停止を使える状態で、ユーザーが行います。報告と可動範囲等の確認まで本番設定を有効化しません。


## 初回方向確認用の別スケッチ準備（2026-10-28・未upload）

`build/md20a_direction_check/md20a_direction_check.ino` に一時スケッチを準備。本番スケッチ・configは69cef55から変更なしで、ACTUATOR_SETUP_CONFIRMED=falseを維持しています。
本番configのコピーを使用し、試験用定数は同ディレクトリのconfig.hへ追記しました。ソース・設定・説明・compile-result.jsonはGit管理外のbuild内に保存しています。
115200bpsのHでDIR=HIGH、LでLOW、PWM128を300ms出力して停止。駆動中は同期測距・Serial出力をせず、停止時にPWM=0を先に設定します。
駆動中の受信は破棄し、停止後も300ms入力が途切れるまで次を受け付けません。ToFはRIT無効・400kHzで、停止中だけBEFORE/AFTERと有効な差分を表示。取得失敗はNAです。
コードの配線割り当てはD8→DIR、D9→PWM、信号GND共通、D20/D21→ToF、D2/D12はLOW固定。実配線・端子電圧は未確認です。
別スケッチのMega 2560向けclean compile成功（Flash 19,968 bytes／RAM 1,172 bytes）。本番も正本verifyで成功（38,508 bytes／2,557 bytes）。
upload・シリアル接続・モータ/ヒーター通電は今回未実施。実機試験はヒーター電源切り離し、可動部から手を離し、既存非常停止が使用可能な状態で、ユーザーの別途指示後に行います。
H/Lの実方向とToF増減はユーザー報告待ち。報告後に本番DIR設定を反映し、さらに300mm位置と下降下限・目標の接触限界適合が確認できるまで、自動運転を有効化しません。


## 現在: MD20A上下制御実装・自動運転は未有効化（2026-10-28）

ユーザー指示によりI/O確認版へ最小の5状態を追加しました。開始SWの押下1回で、上昇300mm→下降目標→保持→上昇300mm→待機の1サイクルです。
上昇距離>=300mm、下降距離<=目標でPWM=0。開始時にA1保持時間とA2目標を固定し、再押下は無視します。保持はmillis()で管理します。
ToF無効時は停止してIDLEへ中断し、新しい開始押下を必要とします。起動直後やSW押しっぱなしでは開始しません。

- config: ACTUATOR_SETUP_CONFIRMED=false（既定で全状態PWM=0）、MOTOR_PWM_RUN=128（約50%）、MOTOR_DIR_UP_LEVEL=HIGH（仮値）、下降は逆レベル。
- 上昇停止ACTUATOR_UPPER_MM=300、下降下限ACTUATOR_LOWER_MIN_MM=50（暫定）。A2の0～300mm表示は維持し、サイクル用目標は下限～300mmへ制限。
- 50mmの測距可否・機械的接触限界、DIR極性、上昇で距離が増えること、可動範囲に300mmがあることは未確認。現物に合わせて下限・DIRを設定し、短い単体動作で確認後にのみ確認済みフラグをtrueにする。
- 現時点では自動運転を有効化しない。距離変化方向が想定と違う場合は本停止条件のまま運転しない。本版に単体試験操作は追加していない。
- D2/D12は全状態LOW、起動はLOWラッチ先行。D9は無効・停止・保持・ToF無効時0。DIRを変える前にもPWM=0とする。
- ToF/RIT無効・400kHz・PT100・A0/A1/A2・デバウンス・ブザー・OLEDの6行を維持。OLEDとSerialのPWMは指令値へ変更し、Serialに状態・確認フラグ・固定目標と保持時間を追記。
- 距離急変・連続正常判定・タイムアウト等は追加しない。別系統の非常停止を前提とする。既存100ms周期と同期測距・表示・Serial待ち時間は残る。
- 正本verifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 38,508 bytes／静的RAM 2,557 bytes）。今回upload・モータ通電・自動運転・加熱試験は未実施。

最新仕様は本節とREADMEの上下制御仕様を優先します。以降の「D9は常に0」「自動運転未実装」は以前のI/O確認版の履歴で、現在も既定の確認フラグfalseではPWM=0です。


## 現在の標準設定: RIT無効（2026-10-28正式採用）

ユーザーの指示により `TOF_RIT_ENABLED=false` を標準設定として正式採用し、I2Cは400kHzを維持します。
現在の測距処理・既存機能、安全出力D2/D12=LOW・D9のPWM=0は維持します。
距離急変監視・連続正常判定・追加停止ロジックや新しい検証項目・試験機能は追加しません。
ToFの測距不良対応はいったん完了とし、次の製作工程へ進みます。今回、追加の実機uploadは行いません。正本verifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 38,054 bytes／静的RAM 2,545 bytes）。
実機は下記履歴のRIT無効版を書き込み済みです。以降の一時変更・比較待ち・RIT有効が既定との記載は過去の経緯であり、現在は本節を優先します。


## 履歴: RIT無効の一時診断（2026-10-28・当時未commit／未push）

**当時は実機とローカルconfig.hをRIT=false、I2C=400kHzの一時条件とし、ユーザーの白紙比較待ちとしていました。**
HEADとorigin/mainは `987d0e921515eadf0b713e4b915d016662f51dd8` のまま。Git上の通常既定値はtrueです。
実機の書き込み条件はこのHEAD（ファームウェアソースはadbc9984と同一）に、config.hのTOF_RIT_ENABLEDだけをtrue→falseにした未コミット変更です。
センサ処理・その他の測距設定・OLED・安全出力は変更していません。今回commit・pushは行いません。

変更・実機確認の履歴:

1. 前回2026-10-28 20:00にadbc9984のRIT有効・400kHz版を書き込み・検証済み。
2. 今回ユーザーから、有効版でTOF_SETTINGS=OK、APIエラー0、57試行すべてS2・正常0との報告を受領。測定日時・実距離・対象姿勢等は未記録で、エージェントの実測とは区別する。
3. ローカルconfigのRITだけをfalseへ変更し、正本verifyでMega 2560向けクリーンcompile成功（終了コード0、Flash 38,054 bytes／静的RAM 2,545 bytes）。compile済みconfigの一致、HEADとの差分がRITの1行だけであることを確認。
4. COM4のMega 2560（USB VID=2341/PID=0042）を検出。当初はポート使用中のため書き込みせず、ユーザーがモニタを閉じた後、排他的に開けることとボードを再確認。
5. ユーザーがモータ・ヒーター・両SSRの負荷電源切り離しとArduinoのUSB給電を確認。20:06:43（日本時間）にCOM4へuploadを1回実行し、終了コード0、ATmega2560署名1E 98 01、`38054 bytes of flash verified` を確認。
6. 20:07:07から115200bps/8N1でDTRリセット後約12秒の起動ログを取得し、ポートを閉じた。
   0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、TOF_RIT_REQUEST=0、TOF_RIT_MATCH=YES、TOF_SETTINGS=OK。

| 設定 | 有効状態 | 読み戻し値 | RAW | API |
|---|---|---|---|---|
| RIT | 0（要求と一致） | 0.034485 MCPS/SPAD（値は保持、判定は無効） | 2260 | ENABLE=0、VALUE=0 |
| Signal Rate Limit | 1 | 0.250000 MCPS | 16384 | ENABLE=0、VALUE=0 |
| Sigma Limit | 1 | 18.000000 mm | 1179648 | ENABLE=0、VALUE=0 |
| Timing Budget | — | 34529 us | — | 0 |
| Pre-range VCSEL | — | 14 PCLK | — | 0 |
| Final-range VCSEL | — | 10 PCLK | — | 0 |

API列のENABLE/VALUEは各取得APIのエラーコード（すべて成功0）です。有効状態とは区別してください。
setterもRETURN=1/API=0。RIT有効状態以外は前回の有効版の起動読み戻しと一致しています。
起動・11行すべての定期ログでSSR=OFF、HOTPLATE_SSR=OFF、MOTOR_PWM=0。コード上もD2/D12=LOW、D9のPWM=0とLOW先行初期化を維持。端子電圧は未実測です。
最終の完全な定期行はTOF_WINDOW_MS=10161、N=82、OK=81、API_ERRS=0、BAD_RANGE=1、status分布0:81|1:1（S2=0）。
これは起動確認中の観測であり、実距離・紙・姿勢・照明が揃った比較ではありません。有効版のユーザー報告57試行との対照実験としては扱いません。
RIT関与の検討材料ですが、距離精度・再現性・測定限界・原因の確定はできません。白紙距離比較はユーザーが行います。
モータ・ヒーター・両SSRの負荷通電は行っていません。測定結果を取得するまでは一時状態を明示して保持し、無断でtrueへの復元・再書き込みはしません。

ログ・再現用記録（Git管理外）: `build/verify-a6fb90f1a87f4fa98ecca0e0bc377587/` の `upload-result.json`、
`startup-115200.log`、`startup-result.json`、`temporary-rit-off.patch`。
HEX SHA256: `49FDCB70CECEF0FB02A9D69F63408BDE1D84392C7FDE4D2206AA9628B6F28F2D`。
config SHA256: `BCF362DFECAF08B508B5E0BBA76EB900A9F5B2865B422CF57ED7F9B63A244B66`。
以下の記録は以前の実装・有効版の履歴です。現在の標準設定は冒頭の正式採用記録を参照してください。


2026-10-28現在、**第1段階 I/O・センサ確認版まで完成**。
このWindows PCでは `.arduino-local/` に開発環境を分離し、Mega 2560向けクリーンcompileに成功しました。
ユーザーからCOM4へのupload、起動ログ、ToF測距等の実機確認結果を受領しています。
2026-10-07 21:01（日本時間）までに、ユーザーの書き込み指示と電源切り離し確認を受け、
1kΩ修正版をCOM4へuploadし、115200bpsの起動・定期ログをエージェントが取得しました。
その後のユーザー確認でOLEDの誤配線を修正し、OLED・ToFの同一I2Cバスでの共存・初期化を確認しました。
開始SWは「離しても押してもLOW」との報告と外付け抵抗の未接続確認を受け、ユーザー指定で内蔵プルアップへ変更しました。
内蔵プルアップ版で通常HIGH／押下LOWとブザー実動作をユーザーが確認し、開始SWのLOW固定問題は解決済みです。
D9/D12の端子電圧実測とPT100校正は未実施です。
ユーザーの確認結果と、エージェントが実行したcompile・upload・ログ取得を区別して記録します。
2026-10-14にOLEDの通常表示を1画面へ集約し、開始SWの内蔵プルアップ設定とToFの原因別診断表示を追加しました。
その後、ユーザーの書き込み指示と電源切り離し確認を受け、COM4へのupload・フラッシュ検証・Serialログ取得に成功しました。
OLEDの6行表示の目視確認とMD20A試験は未実施です。
2026-10-21にホットプレート用SSRのD2をLOW固定で追加し、同日20:05（日本時間）にCOM4へのupload・フラッシュ検証成功。
起動・定期ログでHOTPLATE_SSR=OFFを確認しました。負荷の通電試験・端子電圧実測は未実施です。20:06の起動ではToFの0x29が未検出でした。
その後の確認では0x29検出・TOF_INIT=OK、API=0でS2が多発し、有効距離が安定していません。過去の距離申告は実測条件が未記録で、測定限界とは扱いません。
診断追加版は両速度でcompile済み、400kHz版はCOM4へ書き込み・フラッシュ検証済みです。2026-10-28に起動ログで400kHz設定、OLED・ToF初期化OK、診断値とOFF/0出力を確認しました。距離・速度比較と原因の特定は未完了です。

## RIT比較診断の追加（2026-10-28・実装時は未書き込み）

作業開始時はmainとorigin/mainが `cdb8f094d24e28acec6eb23b3583a1bfba1fcd52` で一致し、未コミット変更なし。
指定基準 `f3f5a9151b954955d4b715cf331101d6d612f7fb` 以降の変更は書き込み記録等の資料だけで、基準ファームウェアを維持して追加しました。

距離の訂正: 「40mm程度しか測距できない」は目測で、定規等による実測ではありません。40mmを測定限界と断定せず、
正常判定で約90mmを示したことも測距誤差とは断定しません。過去の50/80/100～150mm等も光学窓基準の実測条件が未記録で、限界や精度の根拠には使用しません。
確実なのは、初期化・API成功と診断値取得の下でS2が多数発生し、S0も一部あるが有効距離が安定しないことです。

- `config.h` に `TOF_RIT_ENABLED=true` を追加。比較時だけfalseへ変更する。I2Cは400000UL固定とし、速度比較とは混ぜない。
- DEFAULT初期化成功後に公開APIでRIT有効状態のみを設定。しきい値・Signal/Sigma・Timing Budget・VCSELは変更せず読み出す。
- setterはboolと直後の `tof.Status` の両方を確認。各getterも直後にStatusを保存し、失敗した項目はNAとエラーを記録する。
- 起動ログにRIT要求・設定API・有効状態・しきい値、Signal/Sigma有効状態・しきい値、Timing Budget、Pre/Final VCSELを追加。
  しきい値は16.16を65536で割る。RITはMCPS/SPAD、SignalはMCPS、Sigmaはmm。Timing Budgetは整数us、VCSELは整数PCLK。
- 読み戻し不一致・いずれかのAPI失敗ではTOF_SETTINGS=ERRORとし、測距診断は継続するが有効な比較条件として扱わない。
  未検出・初期化失敗では設定APIを呼ばずNOT_RUNと理由を記録。1秒行にRIT要求・起動時有効状態・設定確認結果を追記する。
- ST同梱実装を確認。RITは16.16 SignalRateと8.8 SPADから整数演算で `(256*SignalRate_RAW)/SPAD_RAW` を求め、正のRIT_RAW未満なら失敗。
  SPAD=0では計算値0。S2はセンサ内部status=4でも発生し、他の品質判定の優先順もあるため、Signal/SPADの相関だけでRIT原因と断定しない。
- 基準コミットと照合し、安全出力・PT100・可変抵抗・SW・ブザー・OLED・測距・既存Serial等20関数の一致を確認。
  D2/D12の全書き込みはLOWのみ、D9はLOWまたはPWM=0のみ。setup/loopとLOW先行初期化も変更なし。
- `scripts/verify_environment.ps1` により400kHz固定で有効・無効の両版をMega 2560向けクリーンcompile成功（各終了コード0）。
  有効版Flash 38,056 bytes、無効版38,054 bytes、静的RAMは両方2,545 bytes。無効版compile後は元のconfigのバイト列を復元し、既定はtrue・400kHz。
- 固定コア・ライブラリ版は変更なし。今回、COMポートを開く操作、upload、実機測定、モータ・SSR負荷通電・加熱試験は実施していない。
  実装・compile直後の実機はf3f5a91の旧診断400kHz版。その後、下記の指示によりadbc9984のRIT有効版を書き込み・起動確認した。

次は別途の指示後にRIT有効版／無効版を書き込み、白いA4コピー用紙を光学窓から定規等で測った50/100/150/300mmへ平行に配置して比較します。
照明・配線・電源・ジグ等を同じに保ち、各条件100試行以上。手順・起動ログ・単位・記録表は[ToF診断手順](TOF_DIAGNOSTICS.md)へ追記済みです。
RIT無効でS2が減っても、実距離との誤差・正常値のばらつき・再現性は別評価です。無効設定を自動位置制御へ採用しません。原因は未特定です。

## RIT有効版の書き込み・設定読み戻し（2026-10-28）

ユーザーがモータ・ヒーター・両SSRの負荷電源の切り離し、ArduinoのUSB給電を確認した後に実施しました。
対象はmain/origin/main一致のcommit `adbc9984be4a07662394373e27a71d948057b762`。未コミット変更なし。
`TOF_RIT_ENABLED=true`、`I2C_CLOCK_HZ=400000UL` を確認し、正本verifyでMega 2560向けクリーンcompile成功（終了コード0）。
Flash 38,056 bytes、静的RAM 2,545 bytes。compile済みconfigとの一致も確認しました。

COM4のMega 2560（USB VID=2341/PID=0042）を確認し、20:00:17（日本時間）に `upload --verify` を**1回だけ**実行。
終了コード0、ATmega2560の署名1E 98 01、`38056 bytes of flash verified` を確認しました。
20:00:41から115200bps/8N1でDTRリセット後約12秒の起動・定期ログを取得し、シリアルポートを閉じました。
0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、TOF_RIT_SET=OK（RETURN=1/API=0）、TOF_RIT_MATCH=YES、TOF_SETTINGS=OKを確認。

| 設定 | 有効状態 | 読み戻し値 | RAW | API結果 |
|---|---|---|---|---|
| RIT | 1（要求1と一致） | 0.034485 MCPS/SPAD | 2260 | ENABLE_API=0、VALUE_API=0 |
| Signal Rate Limit | 1 | 0.250000 MCPS | 16384 | ENABLE_API=0、VALUE_API=0 |
| Sigma Limit | 1 | 18.000000 mm | 1179648 | ENABLE_API=0、VALUE_API=0 |
| Timing Budget | — | 34529 us | — | 0 |
| Pre-range VCSEL | — | 14 PCLK | — | 0 |
| Final-range VCSEL | — | 10 PCLK | — | 0 |

しきい値はRAW/65536の小数6桁表示です。RIT・Sigmaの読み戻しはST API内部設定で、独立したレジスタ検証ではありません。
I2C_CLOCK_HZ=400000はソフト設定の確認で、SCL波形は未実測です。
起動ログと11行すべての定期ログでSSR=OFF、HOTPLATE_SSR=OFF、MOTOR_PWM=0を確認。
コード上のD2/D12=LOW、D9のPWM=0、出力ラッチLOW先行と毎loopの再設定も確認しました。端子電圧実測は未実施です。
最後の完全な定期行はN=81、OK=8、API_ERRS=0、BAD_RANGE=73、status分布0:8|2:73。
対象物・実距離・姿勢・照明は指定しておらず、この起動ログは白紙距離比較の結果ではありません。原因は未特定です。
RIT=falseへの変更、2回目の書き込み、モータ駆動・ヒーター・SSR負荷の通電試験は行っていません。
白紙の実距離比較はユーザーが行います。生ログ・書き込み結果はGit管理外のbuild内へ保存しました。

## 完了済み

- A0/A1/A2設定値の読取と平滑化、PT100抵抗・温度算出、ToF同期測距。
- I2Cスキャン、OLEDの6行・1画面表示（ページ切替なし）、115200bpsの定期Serialログ。
- 開始SWのデバウンスと押下時ブザー。
- 起動時とloopで安全出力を維持する処理。
- ホットプレート用SSRのD2追加。通常待機中を含めLOW固定、出力ラッチLOWを先に設定してからOUTPUTへ変更。
- 実環境を根拠にしたバージョン固定、Windows構築・検証スクリプトと構築文書。
- `.tools/` で使っていた専用環境の設定を `scripts/common.ps1` へ移植。
  setup/verifyが自動で `.arduino-local/` を選び、CLIのユーザーZIP配置も検出する。
- 汎用のVS Code設定とタスクをGit管理。Ctrl+Shift+Bで正本のverifyを直接実行。
- 固定抵抗の実物確認に基づき、PT100分圧用抵抗を1kΩ（`1000.0F`）へ修正。
- OLEDのSDA/SCK誤配線を修正し、0x3CのOLEDと0x29のToFの検出・初期化OKを確認。
- 1kΩ修正版のPT100読み取り・抵抗換算・温度換算をI/O確認用途として確認（校正は未実施）。
- ソースと過去の手書き資料を保存。バージョン・検証条件は[ENVIRONMENT](ENVIRONMENT.md)を参照。

## OLED表示版の引き継ぎ（2026-10-14）

OLEDは従来と同じ約250ms周期で更新し、設定温度・設定時間・設定高さ・PT100実測温度・ToF距離・
開始SW・SSR OFF・PWM 0を6行に常時表示します。2.5秒ごとのページ切替と関連する変数・定数は廃止しました。
Adafruit標準フォント `setTextSize(1)`、折り返しなし、Y=0/10/20/30/40/50の固定配置です。
正常時は最大16文字＝96ピクセル幅、ToF診断を含めて最大18文字＝108ピクセル幅、最下端Y=57で128×64内に収まります。
設定温度・高さは整数、設定時間・PT100温度は小数1桁、ToFは整数です。
PT100異常時は `Temp ERR`、ToF異常時は原因別ラベルを表示します。表示例・対応表は[READMEの表示仕様](../README.md#oledの表示仕様)を参照。

Serialの内容・1秒周期、PT100温度計算、A0/A1/A2の設定範囲・平滑化、ピン、SW・ブザー処理は変更していません。
SSR=LOW・モータPWM=0を設定時と毎loopで維持する安全処理も変更していません。
Mega 2560向けクリーンcompileとコード上のレイアウト確認後、2026-10-14 20:32（日本時間）に、
commit `647daec490491eaf9ba732d4306038257d48216b` の成果物をCOM4へ `--verify` 付きでupload成功（終了コード0）。
モータ・加熱電源の切り離しはユーザー確認済みです。115200bpsで約12秒の起動・定期ログを取得しました。
0x29/0x3Cの2台検出、OLED_INIT=OK、TOF_INIT=OK、SSR=OFF、MOTOR_PWM=0を確認しています。
OLEDの6行表示・重なり・更新の目視確認は未実施です。過去の実機記録と今回のupload・ログ取得を区別して引き継いでください。

## 開始SWの内蔵プルアップ版（2026-10-14）

1画面OLED版の書き込み後、ユーザーから「離しても押してもLOW」、外付けプルアップ抵抗は入っていないと確認。
従来のD4=`INPUT`・外付けプルアップ前提では開放時の入力が不定となるため、
ユーザーの指定に従い `pinMode(PIN_START_SWITCH, INPUT_PULLUP)` へ変更しました。
現行仕様はD4の内蔵プルアップ、D4とGNDの間のNO接点、通常HIGH／押下LOWです。外付け抵抗は不要です。
入力判定・30msデバウンス・100msブザー、OLED表示、Serialの内容・周期、安全出力、センサ計算は変更していません。
同日20:53（日本時間）、正本のverifyでMega 2560向けclean compile成功（終了コード0、Flash 33,872 bytes／静的RAM 1,483 bytes）。
COM4へ `--verify` 付きでupload成功（終了コード0）。モータ・加熱電源の切り離しは継続しています。
115200bpsで約15秒の起動・定期ログを取得し、0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、
START_SWのHIGH/LOW切替、SSR=OFF、MOTOR_PWM=0を確認しました。
ユーザーがOLEDで「離すとHIGH・押すとLOW」、押下時にブザーが鳴ることを確認し、LOW固定問題は解決済みです。
PT100はADC=99～101、R=107.14～109.54Ω、TEMP_ACT=18.3～24.5℃を表示。校正は未実施です。
切り分け中の変更前のログではTOF_STATUS=INIT_ERRを記録しましたが、内蔵プルアップ版の再起動後はTOF_INIT=OKでした。
今回の測距はTOF_STATUS=2（TOF=ERR）、対象条件と一時的な初期化不成立の原因は未記録・未確定です。

## ToF診断表示版（2026-10-14）

ユーザーから「ERRが続き、ときどき距離の数値が返る」と報告を受け、OLEDのToF行を原因別表示へ変更しました。
起動時未検出はNOT FOUND、検出後の初期化失敗はINIT ERR、測距APIの通信エラーはI2C ERR、時間切れはTIMEOUTです。
RangeStatus=1はUNSTABLE、2はWEAK/FAR?、3はNEAR?、4はRANGE?、5はSENSOR ERRとして、末尾にSとstatusを表示します。
255はNO DATA、status=0でも距離0ならBAD DATA、未知のstatusはUNKNOWNです。
APIエラーは戻り値も表示し、失敗したAPIの測定構造体を読まないようにしました。
過去に多かったstatus=2は反射信号の不足です。遠い等の可能性はありますが、通信エラーや遠すぎることを意味するとは断定しません。
`?` は推定です。固定の近距離／遠距離しきい値は追加せず、測距パラメータ・正常値の判定は維持しています。
Serialの既存フォーマット・1秒周期は変更していません。API失敗時はTOF_STATUS=255となるため、OLEDのAPIコードも確認します。
PT100、可変抵抗、開始SWのINPUT_PULLUP・ブザー、SSR=LOW・PWM=0、安全処理も維持しています。
根拠と表示幅はREADMEの対応表を参照。全256種類のstatus値・API戻り値の桁数でも、ToF行は最大18文字／108pxです。
正本のverifyでMega 2560向けclean compile成功（Flash 34,258 bytes／静的RAM 1,483 bytes）。
2026-10-14 21:06（日本時間）、COM4へ `--verify` 付きでupload成功（終了コード0）。
21:07から115200bpsで約12秒の起動・定期ログを取得し、0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、
TOF_STATUS=2、START_SW=HIGH、SSR=OFF、MOTOR_PWM=0を確認しました。
通信エラー・近距離側判定・各異常状態の実機での再現試験は未実施です。

## ホットプレート用SSR追加（2026-10-21）

`config.h` に `PIN_SSR_HOTPLATE=2` を追加しました。既存は焼印用SSR=`PIN_SSR=12`、モータPWM=`PIN_MOTOR_PWM=9`です。
D2は既存のピン割り当てと競合しません。制御側はD2→ホットプレート用SSR入力、D12→焼印用SSR入力の割り当てです。
HIGH有効のDC入力で直接駆動できると確認した場合の配線案はD2→SSR入力＋、Mega GND→SSR入力−です。
入力端子・GND接続・駆動回路の要否と負荷側配線は、採用部品の仕様・現物で確認して確定します。
HIGH=ON／LOW=OFFは想定で、実部品の入力極性は未確認です。

`setup()` はD2の出力ラッチをLOWにしてからOUTPUTへ切り替え、I2C等の初期化より前に安全出力を設定します。
`enforceSafeOutputs()` でD2/D12をLOW、D9をPWM=0に固定し、通常待機中も含め毎loopで維持します。
起動ログは `SAFETY: SSR=OFF, MOTOR_PWM=0, HOTPLATE_SSR=OFF`、
定期ログ末尾は `SSR=OFF,MOTOR_PWM=0,HOTPLATE_SSR=OFF`です。既存項目・1秒周期は維持しています。
OLEDの6行表示、ToF診断、PT100、D4の内蔵プルアップ・デバウンス、ブザー、可変抵抗の処理は変更していません。

将来の状態管理で実装する仕様（現行I/O版には未実装）:

| 動作状態 | ホットプレート用SSR |
|---|---|
| 電源投入直後・初期化中 | OFF |
| 初期化完了後の正常待機 | ON |
| 焼印下降中 | OFF |
| 焼印押し付け中 | OFF |
| 焼印上昇中 | OFF |
| 正常終了して待機状態に復帰 | ON |
| 異常停止・制御状態不明 | OFF |

焼印シーケンス開始時、下降前にOFFへ切り替え、下降・押し付け・上昇の全期間でONにしません。
正常終了後、正常待機状態への復帰を確認してからONにします。切替条件は将来の状態管理に基づいて実装します。
**今回は通常待機中ONを実装せず、D2は常にLOW固定です。**
Arduinoはホットプレートの電源ON/OFFのみを担当し、温度調節は本体側で行います。PT100によるホットプレートの温度制御は行いません。
ホットプレートが電源遮断後の再投入で自動復帰するかは未確認です。将来の電源ON/OFF運用の成立条件として確認します。

D2追加の実装作業時点では実機upload・SSR通電・モータ駆動・加熱制御試験は行っていませんでした。
その後のユーザー指示による書き込み・起動ログ確認の結果は、次節に区別して記録します。

同日、`scripts/verify_environment.ps1` でMega 2560向けクリーンcompile成功（終了コード0、Flash 34,316 bytes／静的RAM 1,483 bytes）。
FQBNは `arduino:avr:mega:cpu=atmega2560`。CLI 1.5.1、AVR 1.8.8、固定4ライブラリの専用環境を使用し、固定版は変更していません。
コード差分とMegaのピン定義を照合し、D2の競合なし、D2/D12=LOW・D9のPWM=0、資料とコードのピン対応の一致を確認しました。
OLED・センサ・開始SW・ブザー・周期処理を含む変更対象外の16関数は変更前と一致し、Serial既存項目も維持しています。

## D2追加版の書き込み・起動確認（2026-10-21）

同日19:39（日本時間）時点で、fetch後のmainとorigin/mainはともに
`e3545ef689b5b924adb3a997e282bfdca6b632cb`です。ソースを変更せず、
`scripts/verify_environment.ps1` でMega 2560向けクリーンcompile成功
（終了コード0、Flash 34,316 bytes／静的RAM 1,483 bytes）。固定版の専用環境を使用しました。
コード上でD2/D12への書き込みはLOWのみ、D9はLOWまたはPWM=0のみであること、
LOWラッチ設定がOUTPUT設定に先行し、setupと毎loopで安全出力を維持することを確認しました。

ユーザーはモータ・ヒーター・両SSRの負荷電源をすべて切り離し、USBのみであると確認済みです。
当初 `arduino-cli board list` と10秒間の再検出はいずれも `detected_ports=[]`、WindowsのCOMポートも0件でした。
ユーザーからマイコンが未接続だったと連絡があり、接続後はCOM4のArduino Mega 2560（VID 2341／PID 0042）をCLIとWindowsで確認しました。
最新main・ソース・クリーンcompileしたHEXのハッシュ一致を確認し、20:00:49（日本時間）に `upload --verify --verbose` を実行しましたが、
`cannot set com-state` と `stk500v2_getsync() failed` により終了コード1で失敗しました。フラッシュ書き込み開始には到達していません。
Windowsの.NET SerialPortでも115200bps・8N1で開けず、接続デバイスが機能していない旨のエラーを確認しました。
`mode COM4` はData Bits=0を表示し、115200bps・8N1の設定も受け付けませんでした。USBデバイス再起動も権限不足で失敗しました。
ユーザーのUSB再接続後は.NET SerialPortで115200bps・8N1の接続が成功し、コア・ライブラリ・ドライバの変更なしで通信が復旧しました。

20:05:37（日本時間）に同じ検証済みHEXで `upload --verify --verbose` を再実行し、20:05:50に終了コード0で完了しました。
デバイス署名 `1E 98 01 (ATmega2560)` と `34316 bytes of flash verified` を確認しています。
20:06:14からDTRによる再起動後のログを115200bps・8N1で約12秒取得し、開始マーカー・安全出力・初期化結果と11行の定期ログを確認しました。

```text
DORAYAKI_IO_CHECK_START
SAFETY: SSR=OFF, MOTOR_PWM=0, HOTPLATE_SSR=OFF
I2C_DEVICE=0x3C
I2C_DEVICE_COUNT=1
OLED_INIT=OK
TOF_INIT=ERR
TEMP_SET=150.1C,TEMP_ACT=21.4C,PT100_R=108.34ohm,PT100_ADC=100,TIME_SET=3.6s,HEIGHT_SET=300mm,TOF=ERR,TOF_STATUS=INIT_ERR,START_SW=HIGH,SSR=OFF,MOTOR_PWM=0,HOTPLATE_SSR=OFF
```

今回の起動は0x3Cのみ検出し、ToFの0x29は未検出です。接続・電源条件は未確認で、センサ故障や測距対象条件の問題とは断定しません。
PT100はADC=99～101、R=107.14～109.54Ω、TEMP_ACT=18.3～24.5℃でした。基準温度との比較・校正は行っていません。
開始SWはログ上HIGHで、押下操作・ブザー・OLED表示の目視確認は今回行っていません。
Serialポートは取得後に閉じています。ログのOFF/0は出力設定の表示であり、端子電圧の実測ではありません。
モータ・ヒーター・SSR負荷の通電試験とD2/D9/D12の端子電圧実測は実施していません。

## ToF測距不良の診断追加（2026-10-21）

ユーザーから受領した確認結果（エージェントによる今回の実測ではありません）:

- Mega 2560 Rev3、M5Stack Unit ToF（VL53L0X）、Adafruit_VL53L0X、OLEDとのI2C共用。
- ユニットの電源端子は約4.78V、SDA/SCLのアイドルは約3.3V、0x29検出、TOF_INIT=OK。
- 当時の申告は手で約50mm、白い紙で約80mm、100～150mm付近でS2多発。ただし光学窓基準の実測条件は未記録で、上記訂正に従い限界の根拠にはしない。
- ホットプレートは冷えており、当日の通電なし。センサはジグ取付済みで、ジグは光学窓より前に突出していない。
- 保護フィルムは付いていないと思われるとの報告で、除去済みとは確定していない。
- 測定の正確な時刻、照明、紙の寸法・姿勢、窓からの基準距離、電源・I2C波形、成功回数は未記録。

20:06のエージェント取得ログの未検出とは条件が異なります。切り分け対象は、初期化成功後のSignal Fail多発と有効距離の不安定さです。
固定版のソースを調べ、ToFの `begin()` がWireを再初期化し、OLEDの既定設定が転送後100kHzへ戻すことを確認しました。
今回、`config.h` の `I2C_CLOCK_HZ` 一つで共有バスを切り替え、ToF初期化後の再適用とOLEDの転送中・復帰時の統一を実装しています。
既定値は400000UL、比較時は100000ULです。初期化内部のWire再初期化はライブラリのままで、初期化後の測距速度を揃えます。

1秒周期Serialに、APIの戻り値、診断用距離、信号・周囲光のMCPS、有効SPAD数、各RAW整数、取得不能理由を追加しました。
rateは16.16形式を65536で、SPADは8.8形式を256で割ります。API失敗時は構造体を読まず、診断値は `NA` とします。
従来のTOF_STATUS=255（API失敗時の未取得用値）は維持し、API失敗回はstatus分布に入れません。
全API呼び出しのN・成功・API失敗・品質不良と0～255のstatus分布を集計し、Serialの `r` / `R` で次の通常測距直前に区間をリセットできます。
最新1回の1秒スナップショットと、全試行の集計を区別してください。status=0かつ距離>0の成功条件と、測距設定・品質チェックは変更していません。
OLEDの6行表示・異常ラベル、PT100、可変抵抗、開始SW・ブザー、安全出力は維持します。

100kHz・400kHzの両設定で `scripts/verify_environment.ps1` によるMega 2560向けクリーンcompile成功、終了コード0。
どちらもFlash 36,272 bytes／静的RAM 2,541 bytes。固定版は変更せず、configは既定の400kHzへ戻しています。
静的照合で既存14関数の一致、setup/loopの追加が診断処理だけであること、Serial既存項目・値・順序の維持を確認しました。
D2/D12はLOWのみ、D9はLOWまたはPWM=0のみで、出力ラッチLOWからOUTPUTへ切り替える順序も維持しています。
実装・compileの段階では診断版のuploadと実機測定は行っていません。その後、下記の400kHz版の書き込みと起動ログ確認を実施しました。負荷通電は行っていません。
ユーザーの負荷切り離し確認・書き込み指示を受け、診断400kHz版も書き込み済みです。
白いA4用紙を50/80/100/150/300mmで平行に固定し、速度以外を揃えた10条件で最低30秒・100試行以上を記録する予定です。
手順・単位・記録表・判断方法・次の候補は[ToF診断手順](TOF_DIAGNOSTICS.md)を参照。
現時点で原因は未特定です。光学窓・フィルム・ジグや周辺物の反射、周囲光、電源ノイズ、波形、測距設定とRIT、個体差を一つずつ確認します。

## ToF診断400kHz版の書き込みと起動確認

- 対象はcommit `f3f5a9151b954955d4b715cf331101d6d612f7fb`。書き込み前にHEAD・origin/mainとの一致、compile済みconfig・スケッチ・HEXのSHA256一致を確認しました。
- ユーザーの「全部外したから書き込んで」を受けて実施。USB VID=2341/PID=0042のMega 2560 COM4を検出し、115200bps/8N1でポートを開けることを確認しました。
- 2026-10-21 20:56（日本時間）、正本verifyでcompile済みの400kHz成果物を `upload --verify` で書き込み成功、終了コード0。ATmega2560の署名 `1E 98 01` と `36272 bytes of flash verified` を確認。100kHz版は書き込んでいません。
- 2026-10-28 19:23（日本時間）、COM4を115200bpsで開き、DTRリセット後約12秒の起動・定期ログを取得し、ポートを閉じました。0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、I2C_CLOCK_HZ=400000を確認。クロックはソフト設定の確認で、SCL波形の実測ではありません。
- 起動ログと11行すべての定期ログでSSR=OFF、MOTOR_PWM=0、HOTPLATE_SSR=OFFを確認。コード上もD2/D12=LOW固定、D9のPWM=0とLOW先行初期化を維持。端子電圧実測は未実施です。
- 診断値と集計の出力を確認。最後の完全な定期行はTOF_WINDOW_MS=10216、TOF_N=81、TOF_OK=36、TOF_API_ERRS=0、TOF_BAD_RANGE=45、TOF_STATUS_COUNTS=0:36|2:45でした。1秒スナップショットの有効距離は29～32mm、S2時もAPI=0・TOF_VALID=0で無効扱いです。
- 例: 正常時29mm・SIGNAL=0.921875 MCPS・AMBIENT=0.031250 MCPS・SPAD=8.0156。S2時31mm・SIGNAL=0.062500 MCPS・AMBIENT=0.031250 MCPS・SPAD=8.0156。S2時の距離は診断用返値で、有効距離として扱いません。
- 対象物・実距離・姿勢・照明を固定／記録していないため、この取得を距離別成功率や速度比較の結果には使用しません。S2が混在しており、測距不良の原因は未特定です。
- 2026-10-28の記録更新時にも正本のverifyでMega 2560向けclean compile成功（終了コード0、Flash 36,272 bytes／静的RAM 2,541 bytes）。今回の変更は資料のみで、書き込み済みファームウェアのソースは変更していません。
- 開始SW操作、ブザー・OLEDの目視確認、端子電圧実測、モータ駆動・SSR負荷通電・加熱試験は実施していません。生ログ・書き込み結果はGit管理外のbuild内に保存しました。

## 安全上の重要事項

**D2/D12は全状態でLOW。D9は既定の確認フラグfalse、停止・保持・ToF無効時にPWM=0。実機確認後の有効な移動時のみDuty 100%。**
D8は暫定LOWですが、上下方向の対応は未確定です。
SSRはHIGH=ON／LOW=OFFを想定していますが、実部品の極性は確認が必要です。安全状態を切り替える試験コードをこの版へ混ぜないでください。
開始SWは既定の確認フラグfalseではブザーのみ。有効化後に限り上下1サイクルを開始し、加熱制御は行いません。

`enforceSafeOutputs()` は設定後と毎loopで安全出力を再設定します。
ただし、電源投入からスケッチ開始までの端子状態、配線誤り、SSR・ドライバ故障はソフトだけでは保証できません。
最初は両SSRの負荷・モータ電源を切り離し、D2/D9/D12を実測してから負荷側の接続を判断します。
SerialログのOFF/0だけで端子が安全と判断しないでください。
LOWの設定だけで物理的な安全性は保証できません。電源投入・リセット・ブートローダ動作中から初期化完了までの出力状態、
外付けプルダウンの要否・抵抗値・実装、SSR入力極性・入力電圧・駆動電流とMega出力の適合を実機で確認します。
SSRの負荷電圧・電流・突入電流・放熱を含む負荷定格も未確認です。

PT100は5V・1kΩ固定抵抗の直接分圧です。従来文書の100Ωは実物確認結果により訂正しました。
`PT100_SERIES_RESISTOR_OHM=1000.0F`、PT100の0℃基準抵抗は `PT100_R0_OHM=100.0F` です。
抵抗の詳細な実測値・公差、基準温度との比較、自己発熱・抵抗定格・測定精度は確認が必要です。
コードのPT100有効判定は保護制御ではありません。加熱制御へ転用する前に異常処理を設計してください。

## 2026-10-14までの実機確認結果

2026-10-07に受領したユーザー確認によると、OLEDが認識されなかった原因は、
OLED SDAとSCKが両方Mega D20へ接続されていた誤配線でした。
Mega 2560ではD20=SDA、D21=SCLを使用するため、OLED SDA→D20、OLED SCK→D21へ修正しました。
OLEDは秋月電子115870（SSD1306、128×64、I2Cアドレス0x3C）、ToFはVL53L0X（0x29）です。
修正後の起動ログは以下のとおりで、OLEDの配線・初期化問題は解決済みです。
OLEDとToFは同一I2Cバスで正常に共存しています。

```text
I2C_DEVICE=0x29
I2C_DEVICE=0x3C
I2C_DEVICE_COUNT=2
OLED_INIT=OK
TOF_INIT=OK
```

OLED修正後ログを含め、PT100_ADC=98～100、PT100_R=105.95～108.34Ω、TEMP_ACT=15.2～21.4℃の表示を確認。
実物確認済みの1kΩと `PT100_SERIES_RESISTOR_OHM=1000.0F` は一致し、設定は正しいままです。
読み取り・抵抗換算・温度換算はI/O確認用途として正常動作と判断します。
基準温度計との比較・校正、最終温度制御精度の確認は未実施です。

ToFはTOF_INIT=OKで、過去に62～65mm・TOF_STATUS=0の正常測距を確認しています。
2026-10-14のログでは対象条件によってTOF_STATUS=2、TOF=ERRの場合もあります。
センサ故障とは扱わず、取付位置・対象面・距離条件を決めた後に測距条件を評価します。
2026-10-07の受領時点では、開始SWはユーザーによる配線修正済みで、修正後の通常HIGH／押下LOWの遷移ログとブザー動作結果は未記録でした。
この受領分の配線修正・ログ確認の詳しい測定日時、ToFの対象条件、PT100の基準温度は未記録です。

2026-10-14の1画面表示版のSerialログではPT100_ADC=98～100、PT100_R=105.95～108.34Ω、TEMP_ACT=15.2～21.4℃を確認。
ToFはTOF_INIT=OK、定期ログはTOF_STATUS=2（TOF=ERR）で、今回の測距対象条件は未記録です。
20:32の旧版ログではSTART_SWのHIGH/LOW両方を記録した段階で、スイッチの実際の操作との対応とブザー実動作は未確認でした。
このログ取得では端子電圧実測、PT100校正、OLEDの目視確認は行っていません。

## 次の実機確認順序

モータ・両SSRの負荷電源を切り離し、現行I/O版のD2/D12=LOW、D9のPWM=0を維持して進めます。
D2追加版は2026-10-21にCOM4へのupload・フラッシュ検証・起動ログ確認済みです。ボード・ポート・対象の版を再照合し、以降の必要な書き込みは別途判断します。

1. D2/D9/D12の端子電圧実測。電源投入・リセット・初期化中・通常待機・開始SW押下時もD9のPWM=0、D2/D12=LOWを確認。
   外付けプルダウン、SSR入力極性・駆動電流・負荷定格を確認する。ホットプレートの再投入時自動復帰は、別途明示した通電試験の条件として確認する。
2. START_SWの通常HIGH／押下LOWの遷移ログとブザー動作確認。D4・内蔵プルアップ（INPUT_PULLUP）・NO接点の仕様を照合する。
   30msデバウンスと100msブザーを確認し、ブザー種別・論理を確定する。
3. A0/A1/A2の両端・中間確認。温度150～300℃、時間1～10秒、高さ0～300mm。
4. PT100を基準温度計と比較。分圧抵抗実測値、ADC、自己発熱、誤差を記録。断線・短絡時ERRも確認。
5. ToFはRIT無効・400kHzを標準設定として正式採用し、測距不良対応はいったん完了。次の製作工程へ進む。
6. 以上の記録を残した後、**別の明示的なMD20A単体試験**に進む。現行I/O版はPWM=0のまま保存する。

MD20A単体試験は未実施です。上下1サイクルはPWM約50%で実装済みですが、確認済みフラグはfalse。単体動作・機構確認後まで自動運転は無効です。
方向・可動域・停止手段を現物確認して試験用コードを準備します。
DIR HIGH/LOWと上下方向、ToF距離の増減と移動方向、全ストローク所要時間を実測して確定します。
約60秒はユーザー申告の目安です。上端・下端距離、負荷、速度条件、経過時間を記録してください。

## 未完了・未確定

- 1画面OLED表示版の6行表示・重なり・更新の目視確認。実機書き込み・フラッシュ検証・起動ログ確認は成功。
- ToF診断表示の各異常状態の実機再現、取付・対象条件を決めた後の近距離／遠距離の切り分け。
- D2/D9/D12の起動時を含む端子電圧実測と負荷接続前の配線・SSR入力仕様の確認。D2追加版の書き込み・フラッシュ検証・起動ログ確認は成功。
- ToFのRIT無効・400kHzを正式採用し、測距不良対応はいったん完了。過去の原因候補・比較記録は履歴として保存する。
- A0/A1/A2の両端・中間の実機確認記録。
- PT100の基準温度計との比較・校正、最終温度制御精度の確認。1kΩ修正版のuploadと数値表示は確認済み。
- ToF取付位置、測定対象面、原点距離、距離から高さへの換算・有効測距範囲。
- リミットSWなしでの原点復帰、測距異常時停止、移動タイムアウト、再起動・異常復帰。
- DIR極性、実ストローク・移動時間・適切なPWM。50%は試験予定値。
- PT100精度・校正・入力回路の妥当性、ブザーの種別。
- 両SSRの型番、電源・駆動側配線、外付けプルダウン、入力極性・駆動電流・負荷定格。焼印側の温度制御・温度異常時停止。
- ホットプレート本体の電源再投入時の自動復帰。将来の状態管理による電源ON/OFF（温度調節は本体側）。
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
D2/D12=LOWと、ACTUATOR_SETUP_CONFIRMED=false時のD9 PWM=0を維持してください。上下1サイクルは実装済みですが、方向・距離変化・可動範囲・下限の実機確認まで有効化しないでください。
PT100の分圧用固定抵抗は実物確認済みの1kΩで、設定は1000.0Fです。
実環境を調査し、不足する環境をNEW_PC_SETUP.mdとenvironment.lock.jsonに従って構築し、
VS Code推奨拡張を導入してscripts/setup_windows.ps1とscripts/verify_environment.ps1を実行してください。
両スクリプトが自動選択する.arduino-local/を使い、既存Arduino環境を上書きしないでください。
.tools/やPC固有のキャッシュは不要です。Ctrl+Shift+Bのタスクも確認してください。
管理者操作とログインが必要なら具体的な手順を示してください。
ソースを変更せず、Mega 2560向けクリーンcompile結果と版番号を報告してください。
COM4は前PCのポートです。実機uploadやモータ駆動・加熱動作は行わないでください。
OLEDはSDA→D20、SCK→D21への修正後、0x3C/0x29の検出とOLED/ToFの初期化OKを確認済みです。
1画面OLED表示版は前PCのCOM4へ書き込み・フラッシュ検証済みですが、6行表示の目視確認は未実施です。
START_SWは外付け抵抗の未接続を確認し、D4をINPUT_PULLUPへ変更済みです。NO接点をD4とGNDにつなぎ、通常HIGH／押下LOWです。
内蔵プルアップ版はCOM4へ書き込み済みで、ユーザーが通常HIGH／押下LOWと押下時ブザーを確認済みです。
OLEDのToF異常表示は原因別ラベルへ変更済みです。WEAK/FAR? S2等の疑問符は推定を示し、近い／遠いを断定しません。
D2追加版は2026-10-21に前PCのCOM4へ書き込み・フラッシュ検証済みで、HOTPLATE_SSR=OFFを起動ログで確認済みです。
通常待機中もLOW固定で、将来の正常待機中ONは実装していません。同日20:06の起動ではOLED_INIT=OK、ToFの0x29は未検出でTOF_INIT=ERRでした。
その後の確認は0x29検出・TOF_INIT=OK、API=0でS2多発です。40mm程度は目測との訂正があり、距離限界や約90mm表示の誤差は断定しません。
ToF診断ログとI2C_CLOCK_HZによる100kHz/400kHz比較を追加済みです。診断版は両速度でclean compile済み、400kHz版はCOM4へ書き込み・フラッシュ検証済みです。2026-10-28の起動ログは初期化OK、400000Hz、APIエラー0、status 0と2が混在。対象条件を揃えた距離・速度比較は未実施です。
現在の標準設定はTOF_RIT_ENABLED=false、I2C_CLOCK_HZ=400000ULです。無効版の書き込み・フラッシュ検証・設定読み戻しは確認済みです。ToFの測距不良対応はいったん完了とし、既存機能を維持して次の製作工程へ進んでください。追加ロジック・試験機能は実装していません。
D2/D9/D12端子電圧実測、SSRの極性・駆動電流・負荷定格・外付けプルダウン、本体の電源再投入時自動復帰の確認、SW遷移・ブザーの再確認、
A0/A1/A2両端・中間、PT100比較・校正、ToF取付条件決定後の測距確認の順に残件を引き継いでください。
MD20A単体試験は未実施で、将来の初期PWMは50%予定です。
```

実機試験の記録を得たら、下表とREADMEの現在段階・未確定項目を更新してください。

| 項目 | 状態 | 日時・測定条件・結果 |
|---|---|---|
| このPCのcompile | 成功 | 2026-10-07（日本時間）、Windows x64／PowerShell 5.1。CLI 1.5.1、AVR 1.8.8、固定4ライブラリを `.arduino-local/` に導入。1kΩ修正後に正本のverifyでclean compile成功、終了コード0。Flash 34,146 bytes／静的RAM 1,488 bytes。 |
| RIT比較診断のcompile | 両条件成功・有効版のみ書き込み済み | 2026-10-28、正本verifyで400kHz固定のRIT有効／無効を各clean compile成功。有効38,056 bytes／無効38,054 bytes、RAMは各2,545 bytes。既定trueへ復元。RIT以外の測距設定・安全出力・既存機能を維持。実装時はupload未実施。その後20:00に有効版をCOM4へ1回upload・フラッシュ検証成功、TOF_INIT=OK・RIT_MATCH=YES・SETTINGS=OK。白紙比較・負荷通電・無効版uploadは未実施。 |
| ToF診断追加版のcompile / upload / 起動 | 両速度compile成功・400kHz書き込み済み | 2026-10-21、正本のverifyで100kHz・400kHzの両設定をクリーンcompileし終了コード0。Flash 36,272 bytes／静的RAM 2,541 bytes。既定は400kHz。20:56にCOM4へ400kHz版をupload・フラッシュ照合成功。2026-10-28 19:23の起動ログはOLED/ToF初期化OK、400000Hz、診断値・集計・OFF/0出力を確認。距離・速度比較、負荷通電は未実施。 |
| D2追加版のcompile / upload / 起動 | 書き込み・検証・ログ確認成功 | 2026-10-21、commit `e3545ef689b5b924adb3a997e282bfdca6b632cb` を正本のverifyでclean compile成功。Flash 34,316 bytes／静的RAM 1,483 bytes。全負荷電源を切り離したUSBのみのMega 2560 COM4へ20:05に `--verify` 付きupload成功、終了コード0、34,316 bytesのフラッシュ照合成功。初回の通信エラーはUSB再接続で復旧。20:06から115200bpsで約12秒の起動・定期ログを取得し、HOTPLATE_SSR=OFF、SSR=OFF、MOTOR_PWM=0、OLED_INIT=OK、TOF_INIT=ERR（0x29未検出）を確認。 |
| 1画面OLED版のcompile | 成功 | 2026-10-14、同じ専用環境と固定版を使い `scripts/verify_environment.ps1` でMega 2560向けclean compile成功、終了コード0。Flash 33,872 bytes／静的RAM 1,483 bytes。最大桁数・ERR表示は128×64内に収まり、表示以外の処理と安全出力の不変をコード上で確認。 |
| 1画面OLED版のupload / 起動 | 書き込み・検証成功、目視確認待ち | 2026-10-14 20:32（日本時間）、commit `647daec490491eaf9ba732d4306038257d48216b` の成果物をMega 2560 COM4へ `--verify` 付きでupload成功、終了コード0。モータ・加熱電源切り離しはユーザー確認済み。115200bpsで約12秒のSerialログ取得、0x29/0x3C検出、OLED_INIT=OK、TOF_INIT=OK、SSR=OFF、MOTOR_PWM=0を確認。OLEDの6行表示の目視確認は未実施。 |
| 内蔵プルアップ版のcompile / upload | 成功 | 2026-10-14 20:53（日本時間）、正本のverifyでMega 2560向けclean compile成功。Flash 33,872 bytes／静的RAM 1,483 bytes。COM4へ `--verify` 付きでupload成功、終了コード0。約15秒のログでOLED_INIT=OK、TOF_INIT=OK、SSR=OFF、MOTOR_PWM=0、SWのHIGH/LOW切替を確認。 |
| Mega upload / 起動 | 修正版確認済み | 配線修正前の2026-10-07 21:01（日本時間）までに、commit `25ca1f47b65dd31bf45c5661865212611dcabde7` の1kΩ版をclean compileし、Mega 2560 COM4へupload成功、終了コード0。モータ・加熱電源切り離しはユーザー確認済み。115200bpsで約12秒の起動・定期ログを取得。前回ユーザー確認分の書き込み元の版・測定日時は未記録。 |
| D9 / D12 | ログ上安全・端子電圧実測待ち | SSR=OFF、MOTOR_PWM=0を確認済み。現行I/O版はD12=LOW、D9のPWM=0を維持。実端子電圧測定は未実施。 |
| D2 / ホットプレート用SSR | 書き込み・起動ログ確認済み、端子電圧実測待ち | 2026-10-21、COM4へのupload・フラッシュ検証成功、HOTPLATE_SSR=OFFを確認。現行I/O版はLOW先行初期化と毎loopのLOW固定。SSR通電・モータ駆動・加熱試験は未実施。起動時出力・プルダウン・入力極性・駆動電流・負荷定格と本体の再投入時自動復帰を確認する。 |
| OLED / I2C | 配線修正済み・初期化OK | SDAとSCKが両方D20につながっていた誤配線をSDA→D20、SCK→D21へ修正。過去に0x29/0x3Cの共存・初期化OKを確認。2026-10-21 20:06は0x3Cのみ検出。2026-10-28は0x29/0x3C検出、OLED/ToFとも初期化OK、400kHzのソフト設定を確認。 |
| ToF | 初期化OK・S2多発・距離条件未確定 | 20:06のログは0x29未検出。その後のユーザー確認は0x29検出・TOF_INIT=OK、電源約4.78V、SDA/SCLアイドル約3.3V。過去の距離申告は実測条件が未記録で、測定限界としては未確定。診断400kHz版は書き込み済み。2026-10-28の起動ログは初期化OK、APIエラー0、status 0と2が混在。対象条件は未記録で、距離・速度比較は未実施。原因は未特定。 |
| A0 / A1 / A2 | 未記録 | 両端・中間の表示 |
| PT100 | 1kΩ修正版の数値表示確認済み・校正未実施 | 実物確認済みの固定抵抗1kΩに対し1000.0Fは正しい。20:32までのログはADC=98～100、R=105.95～108.34Ω、TEMP_ACT=15.2～21.4℃。20:53の内蔵プルアップ版ではADC=99～101、R=107.14～109.54Ω、TEMP_ACT=18.3～24.5℃。読み取り・抵抗換算・温度換算はI/O確認用途として正常。基準温度計との比較・校正、最終温度制御精度の確認は未実施。 |
| 開始SW / ブザー | LOW固定問題は解決済み | 外付け抵抗の未接続を確認し、ユーザー指定でD4をINPUT_PULLUPへ変更。ユーザーがOLEDで通常HIGH／押下LOWと押下時ブザーを確認。約15秒のSerialログでもHIGH/LOW切替を記録。NO接点をD4とGNDにつなぎ、外付け抵抗は不要。 |
| MD20A単体試験 | 未実施 | 将来の初期PWMは50%予定。DIR、ToF増減、ストローク時間、PWM、停止方法を別試験で確認する。今回の作業では実施しない。 |
## ToF 1-second filter and OLED target (2026-10-28)

Added `TOF_FILTER_TIME_CONSTANT_MS=1000` and a time-aware EMA for valid status-0 positive ToF ranges. Temporary errors hold the last filtered distance for less than one second; after one second without a valid range the actuator stops and returns to IDLE. Position thresholds use the filtered distance. OLED row 2 retains Hset and shows the current target as `Tgt` (`---` in IDLE, 300 mm while moving up, and the frozen A2 target while moving down or holding). Raw distance/status diagnostics are retained. No upload or motor test was performed.

