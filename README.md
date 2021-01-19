# Lumi Router (JN5169)

This firmware is a replacement for the original firmware for the Zigbee chip JN5169 on Xiaomi Gateway DGNWG05LM and allows to use the gateway as a router (repeater-like) in any Zigbee network instead of the stock coordinator firmware for the propriate Xiaomi MiHome Network.

### This instruction assumes:
- That you already have ssh access to the gateway. If you have not done this, use the instructions [https://4pda.ru/forum/index.php?act=findpost&pid=99314437&anchor=Spoil-99314437-1](https://4pda.ru/forum/index.php?act=findpost&pid=99314437&anchor=Spoil-99314437-1)
- That you already have an alternative OpenWRT firmware installed. If you have not done this, use the instructions [https://github.com/openlumi/owrt-installer](https://github.com/openlumi/owrt-installer)

## Firmware

1. Connect to device via SSH.
2. Issue the following commands in the command line.

```shell
wget https://github.com/igo-r/Lumi-Router-JN5169/releases/download/20210119/Lumi_Router_JN5169_20210119.bin -O /tmp/Lumi_Router.bin 
jnflash /tmp/Lumi_Router.bin
```

## Pairing

Issue the following command in the command line.

```shell
jntool erase_pdm
```
After this the device will automatically join.
