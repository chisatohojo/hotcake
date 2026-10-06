# 第1段階 I/O・センサ確認スケッチ

Arduino Mega 2560 Rev3向けの既存スケッチです。
SSR（D12）は絶対にLOW、モータPWM（D9）は絶対に0を維持します。
開始SWで作動するのはブザーのみです。

ピン・調整値の正本は [config.h](config.h) です。
固定版の導入・ビルド・upload手順は[ルートREADME](../README.md)、
[新PC構築手順](../docs/NEW_PC_SETUP.md)、[HANDOFF](../docs/HANDOFF.md)を参照してください。
