# LiteX MIPI CSI testing on CrossLink NX VIP

Build and load the design:

```
python crosslink_nx_vip.py --build --toolchain oxide --nexus-es-device
ecpprog -S build/crosslink_nx_vip/gateware/crosslink_nx_vip.bit
```

Build and load the software:

```
cd software
make load
```

Type `serialboot` to start the boot. If the camera I2C is working correctly, you should see:

```
IMX258_REG_CHIP_ID 0258
```

Initialisation currently takes a while due to the slow PicoRV32+HyperRAM combo, this can be improved.
Once at the prompt, type `packet` to print the first 128 words of the last received MIPI CSI-2 packet.

Examples:

```
2b09601d
31233136
bd393a35
382e243b
32ba3930
3c3037d3
403e3735
7b282d30
42384a39
38c83a35
2b3531dd
3b363434
02393b3d
4e3be144
3496392c
```

the `2b09601d` header corresponds to a RAW10 packet with 2400 bytes (1920 pixels) of data.
