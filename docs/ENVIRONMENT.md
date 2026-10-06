# 現在PCの環境調査

調査日: 2026-10-06（日本時間）。版番号は実際のコマンド出力で確認しました。
ユーザーの提示値をそのまま採用したものではありません。

## ツールとコア

| 対象 | 観測値 | 確認方法 |
|---|---|---|
| Git | 2.55.0.windows.2 | git --version |
| Arduino CLI | 1.5.1、commit 01f3d4f2b | arduino-cli version |
| Arduino AVR Boards | 1.8.8 | arduino-cli core list |
| VS Code | 1.140.0、x64、commit 07f806f999227108933c2e30515b26eecc1fda74 | code --version |
| Git credential helper | manager | git config --get credential.helper |

Arduino CLIは通常のPATHから実行でき、`arduino-cli config dump --format json` は
`{"config": {}}` でした。独自の絶対パス設定は再現手順に持ち込みません。
ビルドを再現するCLI・コア・必要ライブラリだけを固定し、Git・VS Codeは観測版として記録します。

## Arduinoライブラリ

`arduino-cli lib list` にある全ユーザーライブラリ:

| 登録名 | インストール版 | 本プロジェクトでの扱い |
|---|---|---|
| Adafruit BusIO | 1.17.4 | 必要な依存、固定 |
| Adafruit GFX Library | 1.12.6 | 必要、固定 |
| Adafruit SSD1306 | 2.5.17 | 必要、固定 |
| Adafruit_VL53L0X | 1.2.5 | 必要、固定。アンダースコアを含む登録名 |
| DFRobotDFPlayerMini | 1.0.6 | 現行ソースで未使用、セットアップ対象外 |

Wire/SPIはAVRコア付属です。セットアップは明示した4ライブラリを `--no-deps` で導入し、
暗黙に最新版の依存へ更新することを避けます。

## VS Code拡張

`code --list-extensions --show-versions` から、用途に必要な推奨を選びました。

| 推奨ID | 観測版 | 用途 |
|---|---|---|
| ms-vscode.cpptools | 1.34.4 | C/C++編集・補完 |
| openai.chatgpt | 26.930.61225 | Codex。実際のインストールIDを採用 |
| ms-ceintl.vscode-language-pack-ja | 1.131.2026090407 | 日本語UI、任意 |

拡張はビルド依存ではなく、上記版を強制しません。現在の公開版を導入します。
PlatformIOは既存PCにありましたが、現行はArduino CLIを使用し `platformio.ini` がないため推奨に含めません。
CMake、C#、Python、Web、テーマ、拡張パック、音声用Codex等も本プロジェクトの構築には不要です。
cpptoolsの補完はビルド成功の判定に使用しません。

## 開始時のファイル・Git状態

- 作業フォルダはGitリポジトリではなく、`git status` は「not a git repository」。親フォルダも同様。
- 既存remoteなし。指定URLへの `git ls-remote` は終了コード0で参照なし。空リポジトリを確認。
- 既存ソース: `dorayaki_io_check/dorayaki_io_check.ino`、`config.h`、スケッチ内README。
- 既存build成果物は引き継ぎ対象から除外。現在ソースから再ビルドする。
- 既存の2画像は手書きシーケンス案とピンメモ。`docs/references/` に原本コピーを保存。
- グローバルGit author設定は未設定。初回commit用のローカル設定は接続GitHubプロフィールの
  表示名 `chisato` と公開用noreplyメール `281641389+chisatohojo@users.noreply.github.com` を使用。
  新PCのauthor設定は新PCの利用者が実施し、このPCの `.git/config` は配布しない。

## ビルド再現

確認コマンド:

```powershell
arduino-cli compile --clean --fqbn arduino:avr:mega:cpu=atmega2560 --build-path .\build\baseline .\dorayaki_io_check
```

成功: Flash 34,146 / 253,952 bytes（13%）、静的RAM 1,488 / 8,192 bytes（18%）、
残りRAM 6,704 bytes。残りには実行時の動的確保も必要です。
スケッチ・config.hの内容は引き継ぎ作業で変更していません。

| ソース | 調査時SHA-256 |
|---|---|
| dorayaki_io_check.ino | 0284ae809c3c09a5c86d74ee608e895e732ca246c8ab1de3b561be7759bb23a1 |
| config.h | 00c0401b3d29b184730bb2b55b3485d8fc48644a36dd9136f4876bd508aa11f0 |

実機upload・通電・測定は本調査では実施していません。

## 引き継ぎスクリプトの検証記録

現在PCのWindows PowerShell 5.1上で、以下を確認しました。

- `verify_environment.ps1`: 指定版、ソース、クリーンcompileの検証に成功、終了コード0。
- `setup_windows.ps1 -InstallExtensions`: 導入済み指定版と拡張を保持して再実行、終了コード0。
- `.arduino-local/` の空のdata/downloads/userを環境変数で指定し、コア・必要4ライブラリを
  ダウンロード・導入してクリーンcompileまで成功、終了コード0。
  この分離環境でもFlash 34,146 bytes／静的RAM 1,488 bytes。
  Windows自体の新規インストール検証ではなく、Arduino環境を空にした再現検証です。
- 文書のCLI 1.5.1 ZIPダウンロード例: 公式SHA-256一致、展開と1.5.1の起動を確認。
- 既存ライブラリ版の競合を別コピーのlockで再現し、インストールせず終了コード1となることを確認。
- 別コピーのスケッチに意図的なcompileエラーを入れ、verifyがFAILと終了コード1を返すことを確認。
- スクリプト・文書内PowerShell例の構文、ローカル文書リンク、関連文書の固定版・安全条件、
  Git差分・空白チェックと追跡対象を確認。ビルド・テスト用コピー・ローカル環境はGit無視対象。

試験は無視対象の別ディレクトリで行い、既存スケッチ・config.h・グローバルArduinoパッケージは変更していません。
