# UPDI4AVR Software

UPDI (Unified Program and Debug Interface) host firmware over serial\
For AVR DA/DB, megaAVR-0, tinyAVR-2 series from using avrdude, Arduino IDE\
/// High-Voltage programing FUSE fixed supported (hardware support required) ///

> UPDI対応AVR系列のためのシリアル通信ホストファームウェア\
> AVR Dx, megaAVR, tinyAVR系列を対象に avrdude と Arduino IDEで応用可能\
> /// 高電圧プログラミングによる FUSE 書換対応（要ハードウェア支援） ///

`UPDI4AVR`プロダクトには以下の三種類があるが、このブランチはその最初のものである。

- UPDI対応Arduino互換機で動作する汎用ソフトウェアとしての __UPDI4AVR Software__
- 専用HV制御回路を含むオープンソースハードウェアとして設計された [__UPDI4AVR Programmer__](https://askn37.github.io/product/UPDI4AVR/)
- その専用ハードウェア用に作成された [__UPDI4AVR Firmware__](https://github.com/askn37/multix-zinnia-updi4avr-firmware-builder/)

このブランチの2023/03現在の活動は鈍い。
最新の更新は [__UPDI4AVR Firmware__](https://github.com/askn37/multix-zinnia-updi4avr-firmware-builder/) が先行している。

## 特徴

- UPDI対応 AVR のための、プログラミングホストファームウェア
  - megaAVR-0系列、tinyAVR-2系列、AVR DA/DB/DD/EA系列用
  - ホスト側、ターゲット側の双方を同系列の UPDI対応デバイスで統一できる
- Arduino IDE および avrdude コマンドラインから使用可能
  - JTAGmkIIプロトコル準拠（JTAG2UPDI 互換）
  - ホスト/ターゲット間は対象系列固有のハードウェアUART伝送
  - ホスト/PC間は 1.5M bps 対応（要 avrdude 7.0、ホスト側F_CPU=16Mhz以上）
  - デバッガ機能なし（不揮発メモリプログラミング＆FUSE書換専用）
  - フラッシュメモリ以外の EEPROM / USERROW 等の特殊領域書換も可能
- 高電圧プログラミング対応（要ハードウェア支援）
  - HV印加は自動制御（avrdude -F オプション付加時）
  - チャージポンプ用PWM逆相ピン出力付
- UARTパススルー対応
  - ライタとして動作しない間はターゲットの UARTが PCと繋げられる

### UPDI4AVR hardware

[<img src="https://askn37.github.io/product/UPDI4AVR/2221_Zinnia-UPDI4AVRF-MZU2216B/Zinnia-UPDI4AVRF-MZU2216B_top.svg" width="160">](https://askn37.github.io/product/UPDI4AVR/2221_Zinnia-UPDI4AVRF-MZU2216B/Zinnia-UPDI4AVRF-MZU2216B_top.svg)

[[ハードウェアとしての UPDI4AVR の紹介はこちら]](https://askn37.github.io/product/UPDI4AVR)

## Arduino IDE への導入

[askn37 / Multix Zinnia Product SDK](https://askn37.github.io/)をインストール済の場合、
このライブラリは既に用意されている。

- `ファイル` -> `スケッチ例` -> `UPDI4AVR` を選ぶ\
  重要！__ビルド可能なボード選択をしていなければ、メニューにこの選択肢は表示されない__

そうでなければ次のようにする。

1. .ZIPアーカイブをダウンロードする。[Click here](https://github.com/askn37/UPDI4AVR-Software/archive/master.zip)
1. ライブラリマネージャで読み込む\
  `スケッチ` -> `ライブラリをインクルード` -> `.ZIP形式のライブラリをインストール...`
1. ツールメニューのボード選択で、UPDIホストにする 適切なターゲットを選ぶ（次節）
1. `ファイル` -> `スケッチ例` -> `UPDI4AVR` を選ぶ\
  重要！__ビルド可能なボード選択をしていなければ、メニューにこの選択肢は表示されない__

## ライタホストに選択可能なボード種別

このファームウェアは次の要件を満たす AVR にアップロード（インストール）できる。

megaAVR-0 シリーズ、tinyAVR-2 シリーズ、AVR DA/DB シリーズのうち、

- 8KB 以上の FLASH 領域
- 1KB 以上の SRAM 容量
- 2組の ハードウェアUSART サポート

を有していること。

> __リファレンス AVR は ATtiny824 とその上位品種である。__

その他にこのファームウェアは 以下の製品でインストールできる。

[Arduiono / Arduino megaAVR Boards](https://docs.arduino.cc/software/ide-v1/tutorials/getting-started/cores/arduino-megaavr)

- Arduino UNO WiFi Rev 2 (ATmega4809)
- Arduino Nano Every (ATmega4809) -- __注意事項有り。（後述）__

[askn37 / Multix Zinnia Product SDK](https://askn37.github.io/)
-- megaAVR-0, tinyAVR-0/1/2, AVR DA/DB/DD/EA series support

> デフォルトSDK。HV書込対応。

- megaAVR-0 / tinyAVR-2 / AVR DA/DB/DD/EA
- Microchip Curiosity Nano ATmega4809 (DM320115)
- Microchip Curiosity Nano ATtiny1627 (DM080104)
- Microchip Curiosity Nano AVR128DA48 (DM164151)
- Microchip Curiosity Nano AVR128DB48 (EV35L43A)

> Microchip Curiosity Nano 系列製品中、対象となるのは上記4種。

[MCUdude / MegaCoreX](https://github.com/MCUdude/MegaCoreX)
-- megaAVR-0 series support

- megaAVR-0

[SpenceKonde / megaTinyCore](https://github.com/SpenceKonde/megaTinyCore)
-- tinyAVR-0,1,2 series support

- tinyAVR-2

[SpenceKonde / DxCore](https://github.com/SpenceKonde/DxCore)
-- AVR DA,DB,DD series support

- AVR DA/DB/DD

`Arduino megaAVR Borads` はボードマネージャーから追加インストールする。\
その他はそれぞれのサポートサイトの指示に従って、インストールする。

サブメニューで主クロック設定が可能な場合は、\
16Mhz、20Mhz、24Mhz (オーバークロック時 28Mhz、32Mhz) から選択する。

### 卵と鶏の関係

この UPDI4AVRファームウェアを対象MCUに書き込むには、他のUPDI書込環境が必要である。
つまりまだ何も用意していない状況で、第一選択に UPDI4AVR を選ぶことは出来ない。
従ってまず次の選択をすべきである。

- SerialUPDI 装置を用意/自作する。
  - おそらくもっとも簡単かつ低コスト。
  - 適当なUSB-UARTアダプタと配線材、ダイオード、ブレッドボード等があれば作れる。
- オンボード書込器を搭載した既製品を購入/入手する。
  - Microchip Curiosity Nano製品のうち、前述の型番のものなど。
  - 一見簡単そうだがそうでもない。これらをライタにするには制御信号が被るので快適にはならない。
- UPDI書込器完成品を入手する。
  - 色々あることはあるが、HV書込対応品（ましてAVR DD対応）は世界全体を見回しても稀有。

> UPDI難民という言葉すらあるのがこの界隈である。

### Arduino Nano Every について

オンボード搭載プログラムライタチップが JTAG2UPDI over UART 下位互換の特殊品である。
UPDI4AVR を書き込んでも制御が横取りされるために期待した動作をしないことが多い。

> オンボードUSB を使わずに追加の UART を JTAG通信用に別途用意し、DTR/RTS信号も配線すれば良いが
ほぼ同じ部材で SerialUPDI 書込器を作れてしまうので、それだと価値がない。

## ターゲットに選択可能な modernAVR 品種

2022年前期時点で発表済の品種のみ記載。実物検証済は __太字__ で示す。

|ファミリ|2KB|4KB|8KB|16KB|32KB|主な外観|
|---------|----------|----------|----------|-----------|-----------|-------------|
|tinyAVR-0|__ATtiny202__|ATtiny402 |          |           |           |SOP8         |
|         |ATtiny204 |ATtiny404 |ATtiny804 |ATtiny1604 |           |SOP14        |
|         |          |ATTiny406 |ATtiny806 |ATtiny1606 |           |VQFN20 SOP20 |
|         |          |          |ATTiny807 |ATtiny1607 |           |VQFN24       |
|tinyAVR-1|ATtiny212 |__ATtiny412__|          |           |           |SOP8         |
|         |ATtiny214 |ATtiny414 |__ATtiny814__|__ATtiny1614__|           |SOP14        |
|         |          |ATtiny416 |ATtiny816 |__ATtiny1616__|__ATtiny3216__|VQFN20 SOP20 |
|         |          |ATtiny417 |ATtiny817 |ATtiny1617 |ATTiny3217 |VQFN24       |
|tinyAVR-2|          |ATtiny424 |__ATtiny824__|ATtiny1624 |ATtiny3224 |SOP14 TSSOP14|
|         |          |ATtiny426 |ATtiny826 |__ATtiny1626__|ATtiny3226 |VQFN20 SOP20 |
|         |          |ATtiny427 |ATtiny827 |ATtiny1627 |ATTiny3227 |VQFN24       |
||8KB|16KB|32KB|48KB|||
|megaAVR-0|ATmega808 |ATmega1608|ATmega3208|__ATmega4808__|           |TSOP28 TQFP32|
|         |ATmega809 |ATmega1609|ATmega3209|__ATmega4809__|           |DIP40 TQFP48 |
|||16KB|32KB|64kB|128KB||
|AVR DA   |          |          |AVR32DA28 |AVR64DA28  |AVR128DA28 |DIP28 TSOP28 |
|         |          |          |__AVR32DA32__|AVR64DA32  |AVR128DA32 |TQFP32 VQFN32|
|         |          |          |AVR32DA48 |AVR64DA48  |AVR128DA48 |TQFP48 VQFN48|
|         |          |          |          |AVR64DA64  |AVR128DA64 |VQFN64       |
|AVR DB   |          |          |AVR32DB28 |AVR64DB28  |AVR128DB28 |DIP28 TSOP28 |
|         |          |          |AVR32DB32 |AVR64DB32  |__AVR128DB32__|TQFP32 VQFN32|
|         |          |          |AVR32DB48 |AVR64DB48  |__AVR128DB48__|TQFP48 VQFN48|
|         |          |          |          |AVR64DB64  |AVR128DB64 |VQFN64       |
|AVR DD   |          |AVR16DD14 |__AVR32DD14__|AVR64DD14  |           |SOP14 TSSOP14|
|         |          |AVR16DD20 |AVR32DD20 |AVR64DD20  |           |VQFN20 SOP20 |
|         |          |AVR16DD28 |AVR32DD28 |AVR64DD28  |           |DIP28 TSOP28 |
|         |          |AVR16DD32 |AVR32DD32 |__AVR64DD32__|           |TQFP32 VQFN32|
|AVR EA   |          |          |          |AVR64DD28  |           |VQFN28|
|         |          |          |          |__AVR64DD32__|           |TQFP32 VQFN32|
|         |          |          |          |AVR64DD48  |           |TQFP48 VQFN48|
> 似たような型番だが以下の品種は本表のUPDI系ファミリに該当しない PDI/TPI系の別系統別品種。\
Attiny102/104 ATtiny828/1628/3228

## 基本的な使い方

もっとも簡単な ターゲット 〜 UPDI4AVR 間配線は、VCC、GND、UPDI の3本を配線するだけ。

```plain

Target AVR     UPDI4AVR      Host PC
  +------+     +------+     +------+
  |  UPDI|<--->|PROG  |     |      |
  |      |     |    TX|---->|RX    |
  |   VCC|<----|VCC   |     |      |
  |      |     |    RX|<----|TX    |
  |   GND|---->|GND   |     |      |
  +------+     +------+     +------+

Fig.1 Programing Bridge Wireing
```

UPDI4AVR の UPDIプログラムピンは、品種（およびコンパイルオプション）によって異なる。以下は一例。

|HOST|PIN|PORT|
|----|---|----|
|Arduino UNO WiFi Rev.2|D6|PF4|
|Curiosity Nano AVR128|--|PC0|
|ATtiny824|--|PB2|

その他のピン接続は
[設定ファイル](examples/UPDI4AVR/configuration.h)、
[詳細な使い方](document/advanced.md)
を参照のこと

Arduino IDE ツールメニューでは 書込装置に __JTAG2UPDI__ を指定する。

> 現行の Arduino IDE 1.8.x 同梱の *avrdude* は古いため、
最高通信速度は 115200 bps に制限される。

ターゲット品種を適切に指定し、ブートローダーの書込に成功すれば正常に動作している。\
実際に書込を試みる前に、正常接続性を確認するには [avrdude操作例](document/Avrdude.md) を参照のこと。

## 注意

- AS IS（無保証、無サポート、使用は自己責任で）
- UNO WiFi、Nano Every などは PCとの
UART通信開始時の自動リセットを無効化することができない。
このため UPDI4AVR 通信開始時に自分自身のリセットを含む、数秒以上の無応答遅延時間がある。
  - オンボードデバッガによって直接リセットされるため、外付部品追加での回避はできない。
  - より高度な機能を活用するにはベアチップと専用の回路とで構築するほうが好ましい。
- Arduino IDE からは高電圧プログラミング、デバイス施錠、デバイス解錠、
EEPROM書換などの高度な指示をすることは原則出来ない。
これらはコマンドラインから *avrdude* を操作することで対応する。
  - [Multix Zinnia Product SDK](https://askn37.github.io/) はサブメニューで選べる。
- デバッガインタフェースとしては機能しない。特殊領域書換とチップ消去にのみ対応。
  - UPDIオンボードデバッグ制御プロトコルは非公開のプロプライエタリ。
- 高電圧プログラミングには、追加のハードウェア支援が絶対に必要。（必然的に）

## うまく動かない場合

- メニュー設定を確認する
  - AVR のボードマネージャー種別
    - megaAVR-0、tinyAVR-2、AVR DA/DB の各系統以外ではビルドできない
    - ATmega328P 等の旧世代 AVR は使用不能
  - サブメニュー選択
  - シリアルポートの選択
  - 書込装置の選択
- 配線を確認する
  - 電源供給はホスト側かターゲット側のどちらか一方からのみ供給する（電圧を揃える）
  - ファームウェアを書く時、FUSEを書く時は ターゲットの UART線は外す
    - UARTパススルーを併用しない（ターゲットがGPIOを使っているとJTAG通信を妨害する）
  - なるべく短い線材を使う
- 復旧に高電圧プログラミングが必要になる FUSE設定を安易に試さない
  - そういう FUSE をくれぐれも間違って書かない

## 更新情報

- 2023/04/14版
  - AVR_EA系統の書換に対応。（UPDI4AVR Firmwareからのバックポート）
  - ファイル配置変更。
  - `FUSE`書換は以前と同じ値なら何もしないように対応。

- 2023/03/26版
  - 標準通信速度を 912.6kbps に訂正。（多数のUSB-UART変換器での実用限界速度）
  - AVR_Dx系統以降の`EEPROM`書換不具合修正。（*avrdude.conf*改定）
  - AVR_Dx系統以降の`USERROW`通常書換に対応。
  - AVR_Dx系統以降の`FLASH`領域への`-D`使用時の部分書換に対応。（かわりに最高書込速度低下）

## 関連リンク

- [askn37 / Multix Zinnia Product SDK](https://askn37.github.io/)
-- megaAVR-0, tinyAVR-0/1/2, AVR DA/DB/DD/EA series support
- [MCUdude / MegaCoreX](https://github.com/MCUdude/MegaCoreX)
-- megaAVR-0 series support
- [SpenceKonde / megaTinyCore](https://github.com/SpenceKonde/megaTinyCore)
-- tinyAVR-0,1,2 series support
- [SpenceKonde / DxCore](https://github.com/SpenceKonde/DxCore)
-- AVR DA,DB,DD series support

## その他の情報

- [avrdude操作例](document/avrdude.md)
- [詳細な使い方・高電圧プログラミング](document/advanced.md)

## 著作表示

Twitter: [@askn37](https://twitter.com/askn37) \
GitHub: [https://github.com/askn37/](https://github.com/askn37/) \
Product: [https://askn37.github.io/](https://askn37.github.io/)

Copyright (c) askn (K.Sato) multix.jp \
Released under the MIT license \
[https://opensource.org/licenses/mit-license.php](https://opensource.org/licenses/mit-license.php) \
[https://www.oshwa.org/](https://www.oshwa.org/)
