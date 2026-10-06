# Codex作業ルール

このリポジトリは、たい焼き焼印装置のArduino Mega 2560 Rev3プロジェクトです。
着手時に README.md、docs/HANDOFF.md、docs/NEW_PC_SETUP.md を読んでください。

- 現在は第1段階 I/O・センサ確認版。SSR（D12）は絶対にLOW、モータPWM（D9）は絶対に0を維持する。
- 安全系出力を勝手に有効にしない。実機確認前にSSR・モータを自動動作させない。
- MD20A単体試験は別途明示された試験作業として扱い、現行I/O版へ自動運転を混ぜない。
- DIR極性、ToF原点・取付位置など未確定仕様を推測して確定扱いしない。
- ピン番号・調整値は dorayaki_io_check/config.h へ集約する。
- delay()に依存せずmillis()ベースを優先する。ToFライブラリ内部の同期測距による待ち時間には留意する。
- 変更後は scripts/verify_environment.ps1 でMega 2560向けクリーンcompileを実行する。
- 実機が接続されていなければuploadを無理に行わない。接続だけを根拠に自動uploadしない。
- 実機試験結果・測定条件・未解決事項は README.md または docs/HANDOFF.md へ反映する。
- 固定バージョンの正本は scripts/environment.lock.json。変更する場合は実測とcompile結果を根拠にし、関連文書も更新する。
- setupは既存の異版コア・ライブラリを上書きしない。認証情報、ローカル絶対パス、ビルド生成物をcommitしない。
