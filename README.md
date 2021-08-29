## 概要
[Strawberry Linux Co.,Ltd.](http://strawberry-linux.com/)のUSBガイガーカウンタキット Ver.2(USB-GEIGER V2)をLinux環境で扱うためのプログラムです。

カーネルのアップデートの影響を受けにくい[HIDAPI](https://github.com/signal11/hidapi)により実装しています。

## 開発環境
Ubuntu Desktop 20.04 LTS

## ビルド
     $ apt install -y build-essential libhidapi-dev
     $ make

make installは未実装です。

一般ユーザーで動作させる場合は、99-hidraw-usbgeiger.rulesを/etc/udev/rules.d/に配置するなどして、該当のhidrawデバイスにアクセス許可を与えてください。

## 使用方法
usbgeiger [-dlfVRGH]
    (オプションなし)				# 日付・時刻、カウント数・経過時間(秒)を表示
    -d  : Enable debugging			# デバッグ情報の表示
    -h  : Show usage			# 使用方法の表示
    -l  : Show device list		# 接続デバイスの一覧表示
    -sn : Specify device number (n=0:all)	# 複数接続時のデバイス指定(n=0:すべて)
    -V  : Show firmware version	# ファームウェアバージョンの表示
    -C  : Clear dev_counter		# カウンター値・経過時間のクリア

## 実行例
     $ ./usbgeiger
     tm:2020/10/10-10:20:20 ec1:9 et1:60 
