# avrdude操作例

2022/05時点の Arduino 1.8.x で使われる *avrdude* は、`version 6.3-20201216`\
[https://github.com/avrdudes/avrdude/](https://github.com/avrdudes/avrdude/) 公開版は `version 7.0-20220508`

`vsrsion 7.2` 以降は大幅に仕様が変わったため、本文書は一部参考となる。

## コマンドライン例

UPDI4AVRを介してターゲットMCUと UPDI通信を行うには、以下の構文が最低限必要となる。
これはターゲットに対して具体的には何もしないが、正常接続の確認は行う。

```sh
avrdude -C avrdude.conf -c jtag2updi -P /dev/cu.usbserial-1420 -p atmega4808
```

*avrdude.conf* には __jtag2updi__ サポートと、ターゲットMCUの定義が含まれていなければならない。
成功したならば、ターゲットMCU のシグネチャが読めたことを報告する。

```pre
avrdude: Device signature = 0x1e9650 (probably m4808)
```

以下のエラーが返る場合は、プログラマー定義がない。

```pre
avrdude: Can't find programmer id "jtag2updi"
```

この場合には以下の設定を *avrdude.conf* のどこかに挿入する。

```conf
# avrdude 6.3
programmer
  id    = "jtag2updi";
  desc  = "JTAGv2 to UPDI bridge";
  type  = "jtagmkii_pdi";
  connection_type = serial;
  baudrate = 115200;
;
```

- baudrate 値は、`-b`オプションで任意に変更できる。programmer定義に書かれた速度は `-b`省略時の既定値。
- baudrate 値は Arduino IDE 付属の *avrdude 6.8* では 115200 が最大となる。
これより増速するには、*avrdude 7.0* 以上が必要。

よくあることだが、以下の警告は ターゲット定義に記述の不足があることを示す。

```pre
avrdude: jtagmkII_initialize(): Cannot locate "flash" and "boot" memories in description
```

これは UPDI対応MCUの定義では `"flash"` 設定は必須であるものの `"boot"` 定義を欠いているために発生する。
このままでも害はないが、次の記述を *avrdude.conf* の該当箇所を次のようにすれば表示されなくなる

```conf
part
    id        = ".avr8x";
    desc      = "AVR8X family common values";
    has_updi  = yes;
    nvm_base  = 0x1000;
    ocd_base  = 0x0F80;

    # 以下を追記
    memory "boot"
        size = 0;
    ;

(略)

part
    id        = ".avrdx";
    desc      = "AVR-Dx family common values";
    has_updi  = yes;
    nvm_base  = 0x1000;
    ocd_base  = 0x0F80;
    rampz     = 0x0B;

    # 以下を追記
    memory "boot"
        size = 0;
    ;
```

- 以上の修正を加えた *avrdude.conf* を *appendix* ディレクトリに用意してあるので活用すると良い。
詳細は後述。

代表的な optiboot 書込操作は次のようになるだろう。
`\`で改行して読みやすくしているが、実際は一行で書く。
（特に Windows DOSプロンプトでの場合）

```sh
avrdude -C avrdude.conf \
 -p m4808 \
 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -e \
 -U fuse0:w:0x00:m \
 -U fuse1:w:0x54:m \
 -U fuse2:w:0x02:m \
 -U fuse4:w:0x00:m \
 -U fuse5:w:0xC9:m \
 -U fuse6:w:0x06:m \
 -U fuse7:w:0x00:m \
 -U fuse8:w:0x02:m \
 -U flash:w:Optiboot_x.hex:i
```

具体的なFUSEの設定などは煩雑なので、Arduino IDE で実行したときのメッセージ出力をコピーして保存しておき、雛形にすると良い。

- 環境設定メニューで `より多くの情報を表示する：書き込み` にチェックを付けておく。
- MegaCoreX 等では、消去＆FUSE書き込みと、ブートローダー/スケッチ書き込みが 2行2操作に分かれているので注意。

## 特殊な操作が必要な場面

さて、次のようなエラーが表示されるのは Device signature の読み出しに失敗した場合である。

```pre
avrdude: jtagmkII_program_enable(): bad response to enter progmode command: RSP_ILLEGAL_MCU_STATE

avrdude: Device signature = 0xffffff (probably XXXXXX)
avrdude: Yikes!  Invalid device signature.
         Double check connections and try again, or use -F to override
         this check.
```

返却されるデバイス識票（signature）が異常値で埋められていることに注意されたい。
これは UPDI4AVR がターゲットから情報を読み取れなかったことを示す。

これには以下の理由がある；

1. UPDI通信許可手順に失敗した。電気的特性に問題がある。
2. UPDI通信許可手順は正しいが、そもそも配線が繋がっていなかったりターゲットに無視された。
3. UPDI通信許可手順は成功したが、プログラミング操作遷移が拒否された。

(1)の場合、UPDI4AVR では `signature = 0x000000` を返す。
これは正しいデータが送信できなかったか、受信データが正しくない（文字化けした）ことを示す。
多くの場合配線に問題があるだろう。
ターゲットの UPDI が無効でかつ GPIO に使われ、LOW を出力しつづけている場合もこれになる。

(2)の場合、UPDI4AVR では `signature = 0xffffff` を返す。
これは正しいデータは送信できたが、ターゲットが応答しなかったことを示す。
多くの場合これは高電圧プログラミングが必要な状態だろう。

(3)の場合、UPDI4AVR では __擬似署名__ を返す。
これは正しいデータが送信でき、ターゲットも応答したが、メモリを読めなかった。
多くの場合これはデバイス施錠が行われていることを示す。
できることはチップ消去と USERROW 書き込みだけである。

> 擬似署名は 0x1e と 2文字の識別コードからなる。これは System Informarion Block の要約で、デバイスファミリー＋NVM制御器バージョン番号からなる。

## 高電圧印加 FUSE 書換

先の(2)のケースの場合、高電圧プログラミングには追加のハードウェア回路が必要となる。
もしそれが手元にあるならば、正しく初期状態に復元すべき `-U fuse:w` 設定と、
`-e` および `-F` オプションを列挙することで復元できる。

```sh
# tinyAVR または megaAVR 系列での UPDI復旧 FUSE
avrdude -C avrdude.conf \
 -p t202 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -U fuse5:w:0xF6:m \
 -U flash:w:Optiboot_x.hex:i
 -e -F

# AVR Dx 系列での UPDI復旧 FUSE
avrdude -C avrdude.conf \
 -p avr32da32 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -U fuse5:w:0xC0:m \
 -U flash:w:Optiboot_dx.hex:i
 -e -F
```

`-e` `-U` `-F` の三つ組はセットで記述すべきである。
UPDI4AVRは、`-F` で強制実行が指定され、かつ `-e` でチップ消去が指示され、
なおかつ通常の UPDI開始手順失敗を検出した場合に、高電圧印加を試行する。\
（このとき `HVEN` ピン出力が最大 1ms の間 HIGH にトリガーされる）\
成功すれば、以後は通常の UPDI通信が可能な状態（高電圧印加を必要としない）に復旧している。

- 厳密な高電圧プログラミングにおいてチップ消去は必須ではないが、
UPDI4AVRでは誤操作に対するフェイルセーフとして、
チップ消去コマンドを実行許可トリガーとしている。

- 標準の *avrdude* では任意のオプションで高電圧プログラミングを試行したり強制することが出来ない。
任意変更可能なのは `-b baudrate` と `-e/-D` のみである。

## デバイス施錠・デバイス解錠

(3)の状態は デバイス施錠 を行うことで、意図的に発生させることができる。

```sh
# 注意！この記述はデバイス施錠を実施します！
avrdude -C avrdude.conf \
 -p t202 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -U lock:w:0:m \
```

施錠されたデバイスは正常に動作を続けるものの、
もはやUPDI経由でのフラッシュおよび EEPROM読み書き操作を受け付けない状態になる。
これを解錠するには同時にチップ消去を伴わなければならないため、
ファームウェア内容の抜き出しが困難になる。（物理的にIC内部を暴かない限り）

UPDI4AVR でデバイス解錠を行うには、これもまた `-e` `-U` `-F` の三つ組同時指定を必要とする。
（UPDI通信自体は拒否されていないので、高電圧印加は自動的に不要と判断される）

```sh
# tinyAVR または megaAVR 系列でのデバイス解錠とチップ消去
avrdude -C avrdude.conf \
 -p t202 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -U lock:w:0xC5:m \
 -U flash:w:Optiboot_x.hex:i
 -e -F

# AVR Dx 系列でのデバイス解錠とチップ消去
avrdude -C avrdude.conf \
 -p avr32da32 -c jtag2updi \
 -P /dev/cu.usbserial-1420 \
 -U lock:w:0x5C,0xC5,0xC5,0x5C:m \
 -U flash:w:Optiboot_dx.hex:i
 -e -F
```

## 状況ステータス現示

avrdude の通常の出力では、JTAG通信に現れない UPDI4AVR 内部の動作状況などを知る由はない。
しかしながら `-v` を4つ（`-v -v -v -v`）連ねて実行すると、以下のような追加出力を得ることができる。

```pre
avrdude: jtagmkII_recv(): Got message seqno 18 (command_sequence == 18)
avrdude: Recv: . [80] . [03] . [03]

Raw message:
0x80 0x03 0x01
OK
```

Recv / Raw message 出力の1バイト目は JTAGレスポンスコードで、0x80~0x9F は成功応答を示し、0xA0 以上なら失敗応答である。
次の2バイト目と3バイト目が UPDI4AVR 独自の追加ステータスコードで、それぞれのバイトの各ビットは以下の意味を持つ。

|JTAG2::CONTROL|value|各ビットの意味|
|--------------|-----|----|
|HOST_SIGN_ON|0x01|SIGN_ON制御を受け付けた|
|USART_TX_EN|0x02|JTTXは出力状態にある|
|CHANGE_BAUD|0x04|速度変更制御を受け付けた|
|ANS_FAILED|0x80|制御失敗を経験した|

- `HOST_SIGN_ON` は JTAG通信中はセットされ、SIGN_OFF時にクリアされる。
- `USART_TX_EN` のクリアが表面上観測されることはない（avrdudeに応答が到達しているので）
- `CHANGE_BAUD` は ボーレート変更指示を受け付けたらセットされる。
次の制御受付から新しいボーレートとなり、このフラグはクリアされる。
- `ANS_FAILED` は一連のシーケンス（連続するseqno）中に失敗応答を返したことを示す。
新たなシーケンスを開始するか、シーケンス途中でも HV制御等での原状回復が成功すればクリアされうる。
これはシーケンス終了時の `UPDI_STAT` 出力状態に一致する。

|UPDI::CONTROL|value|各ビットの意味|
|-------------|-----|----|
|UPDI_ACTIVE|0x01|SIB取得に成功した|
|ENABLE_NVMPG|0x02|プログラミングモードに遷移した|
|CHIP_ERASE|0x04|チップ消去を正常実行した|
|HV_ENABLE|0x08|HV制御が実行された|
|UPDI_LOWBAUD|0x20|接続速度低下中|
|UPDI_TIMEOUT|0x40|制限時間内にUPDIが応答しない|
|UPDI_FALT|0x80|UPDI制御に失敗|

- `UPDI_ACTIVE` は UPDI接続が正常確立したならセットされる。HV制御も不要。
- `ENABLE_NVMPG` はプログラミングモードに遷移できたことを示す。
- `CHIP_ERASE` はチップ消去指示を受け付け、制御に成功したことを示す。
- `HV_ENABLE` は HV制御を試行したことを示す。
- `UPDI_LOWBAUD` は UPDI接続開始時に再試行が発生したことを示す。
- `UPDI_TIMEOUT` は UPDI接続中に制限時間超過があったことを示す。
- `UPDI_FALT` は以上諸々の理由で UPDI接続が失敗したことを示す。応答後にクリアされる。

`UPDI_FALT`+`UPDI_ACTIVE` がセットされ `ENABLE_NVMPG` がクリアなら、デバイスは施錠された状態である。\
`UPDI_FALT`+`UPDI_LOWBAUD` がセットされ `UPDI_ACTIVE` がクリアなら、チップ消去時に高電圧制御を試行する。\
`UPDI_FALT`+`UPDI_TIMEOUT` がセットされ `UPDI_ACTIVE`+`ENABLE_NVMPG` がクリアなら、配線未接続あるいはターゲット電源オフの可能性を示す。\
`UPDI_FALT`+`UPDI_TIMEOUT`+`ENABLE_NVMPG` がセットなら、処理途中でターゲットの制御を失ったことを示す。\
`UPDI_ACTIVE`+`CHIP_ERASE`+`HV_ENABLE`がセットされれば、HV制御に成功したことを示す。
連続して FUSE を正しく上書きすれば、デバイス解錠あるいは HV制御不要な状態に復旧できる。

## JTAG通信高速化

2022/05時点の Arduino 1.8.x に添付されている *avrdude* は `version 6.3-20201216`\
その `jtagmkII.c` の記述；

```c
  static struct {
    long baud;
    unsigned char val;
  } baudtab[] = {
    { 2400L, PAR_BAUD_2400 },
    { 4800L, PAR_BAUD_4800 },
    { 9600L, PAR_BAUD_9600 },
    { 19200L, PAR_BAUD_19200 },
    { 38400L, PAR_BAUD_38400 },
    { 57600L, PAR_BAUD_57600 },
    { 115200L, PAR_BAUD_115200 },
    { 14400L, PAR_BAUD_14400 },
  };
```

2022/05時点の [GitHub](https://github.com/avrdudes/avrdude/)公開版は `version 7.0-20220508`\
その `jtagmkII.c` の記述；

```c
static struct {
  long baud;
  unsigned char val;
} baudtab[] = {
  { 2400L, PAR_BAUD_2400 },
  { 4800L, PAR_BAUD_4800 },
  { 9600L, PAR_BAUD_9600 },
  { 19200L, PAR_BAUD_19200 },
  { 38400L, PAR_BAUD_38400 },
  { 57600L, PAR_BAUD_57600 },
  { 115200L, PAR_BAUD_115200 },
  { 14400L, PAR_BAUD_14400 },
  /* Extension to jtagmkII protocol: extra baud rates, standard series. */
  { 153600L, PAR_BAUD_153600 },
  { 230400L, PAR_BAUD_230400 },
  { 460800L, PAR_BAUD_460800 },
  { 921600L, PAR_BAUD_921600 },
  /* Extension to jtagmkII protocol: extra baud rates, binary series. */
  { 128000L, PAR_BAUD_128000 },
  { 256000L, PAR_BAUD_256000 },
  { 512000L, PAR_BAUD_512000 },
  { 1024000L, PAR_BAUD_1024000 },
  /* Extension to jtagmkII protocol: extra baud rates, decimal series. */
  { 150000L, PAR_BAUD_150000 },
  { 200000L, PAR_BAUD_200000 },
  { 250000L, PAR_BAUD_250000 },
  { 300000L, PAR_BAUD_300000 },
  { 400000L, PAR_BAUD_400000 },
  { 500000L, PAR_BAUD_500000 },
  { 600000L, PAR_BAUD_600000 },
  { 666666L, PAR_BAUD_666666 },
  { 1000000L, PAR_BAUD_1000000 },
  { 1500000L, PAR_BAUD_1500000 },
  { 2000000L, PAR_BAUD_2000000 },
  { 3000000L, PAR_BAUD_3000000 },
};
```

上記の通り、Arduino IDE添付版では標準の最大速度が 115200 bps までしか対応していない。
しかし version 7.0 に更新すると、最大 3M bps まで指定することが可能になる。

UPDI4AVR は、この新しい定義設定に対応している。

実用上の最高/最低速度はホスト側のUSBドライバに依存しており、
限度を超えると以下の警告が出てリトライが多発し正常に動作しなくなる。
1M bps 以上と 9600 bps 未満はおおくの場合正しく利用できない。
安全な設定値はおそらく`-b 500000`が最大だろう。
（この値はデバイス動作クロックをちょうど割り切れるため通信誤差が少ない）

```pre
avrdude: jtagmkII_paged_load(): timeout/error communicating with programmer (status -1)
```

> 1Mbps以上でUSB-シリアル変換通信をする場合は、パケットサイズが概ね200バイト程度以下でないとUSBパケットロストが多発して安定しなくなる。しかしNVM操作で要求されるパケットサイズはこれを超えてしまうので、シリアルコンソールに比べて上限速度が制約されてしまう。どうしても必要な場合は`readsize`設定を小さくすることで一応回避可能だ。

[\<-- 戻る](../README.md)

## Copyright and Contact

Twitter(X): [@askn37](https://twitter.com/askn37) \
BlueSky Social: [@multix.jp](https://bsky.app/profile/multix.jp) \
GitHub: [https://github.com/askn37/](https://github.com/askn37/) \
Product: [https://askn37.github.io/](https://askn37.github.io/)

Copyright (c) 2023 askn (K.Sato) multix.jp \
Released under the MIT license \
[https://opensource.org/licenses/mit-license.php](https://opensource.org/licenses/mit-license.php) \
[https://www.oshwa.org/](https://www.oshwa.org/)
