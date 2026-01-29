#!/usr/bin/env python3

#
# This file is part of LiteX-Boards.
#
# Copyright (c) 2020 David Corrigan <davidcorrigan714@gmail.com>
# Copyright (c) 2020 Alan Green <avg@google.com>
# Copyright (c) 2020-21 gatecat <gatecat@ds0.me>
#
# SPDX-License-Identifier: BSD-2-Clause

import os
import argparse

from migen import *
from migen.genlib.resetsync import AsyncResetSynchronizer

from litex_boards.platforms import lattice_crosslink_nx_vip

from litehyperbus.core.hyperbus import HyperRAM

from litex.soc.cores.ram import NXLRAM
from litex.soc.cores.spi import SPIMaster
from litex.build.io import CRG
from litex.build.generic_platform import *

from litex.soc.interconnect import wishbone

from litex.soc.cores.clock import *
from litex.soc.integration.soc_core import *
from litex.soc.integration.soc import SoCRegion
from litex.soc.integration.builder import *
from litex.soc.cores.led import LedChaser
from litex.soc.cores.bitbang import I2CMaster
from litex.soc.cores.freqmeter import FreqMeter
from litex.soc.cores.gpio import GPIOIn, GPIOOut

from litex.build.lattice.oxide import oxide_args, oxide_argdict

from dphy_wrapper import DPHY_CSIRX_CIL
from mipi_csi import *

kB = 1024
mB = 1024*kB


# CRG ----------------------------------------------------------------------------------------------

class _CRG(Module):
    def __init__(self, platform, sys_clk_freq):
        self.rst = Signal()
        self.clock_domains.cd_sys = ClockDomain()
        self.clock_domains.cd_por = ClockDomain()

        # TODO: replace with PLL
        # Clocking
        self.submodules.sys_clk = sys_osc = NXOSCA()
        sys_osc.create_hf_clk(self.cd_sys, sys_clk_freq)
        platform.add_period_constraint(self.cd_sys.clk, 1e9/sys_clk_freq)
        # use cam_reset here because it's also hardwired to the reset of the cameras
        rst_n = platform.request("cam_reset")

        # Power On Reset
        por_cycles  = 4096
        por_counter = Signal(log2_int(por_cycles), reset=por_cycles-1)
        self.comb += self.cd_por.clk.eq(self.cd_sys.clk)
        self.sync.por += If(por_counter != 0, por_counter.eq(por_counter - 1))
        self.specials += AsyncResetSynchronizer(self.cd_por, ~rst_n)
        self.specials += AsyncResetSynchronizer(self.cd_sys, (por_counter != 0) | self.rst)


# BaseSoC ------------------------------------------------------------------------------------------

class BaseSoC(SoCCore):
    SoCCore.mem_map = {
        "rom":              0x00000000,
        "sram":             0x40000000,
        "main_ram":         0x50000000,
        "csr":              0xf0000000,
    }
    def __init__(self, sys_clk_freq=int(75e6), hyperram="none", toolchain="radiant", **kwargs):
        platform = lattice_crosslink_nx_vip.Platform(toolchain=toolchain)
        platform.add_platform_command("ldc_set_sysconfig {{MASTER_SPI_PORT=SERIAL}}")

        _lcd_pmod_ios = [
            ("lcd_spi", 0,
                Subsignal("clk",  Pins("PMOD0:1")),
                Subsignal("mosi",  Pins("PMOD0:2")),
                Subsignal("miso",  Pins("PMOD0:3")),
                Subsignal("cs_n",  Pins("PMOD0:4")),
                Misc("SLEWRATE=FAST"),
                IOStandard("LVCMOS33"),
             ),
            ("lcd_gpio", 0,
                Pins("PMOD0:5 PMOD0:6"),
                Misc("SLEWRATE=FAST"),
                IOStandard("LVCMOS33"),
             ),
        ]
        platform.add_extension(_lcd_pmod_ios)

        # Disable Integrated SRAM since we want to instantiate LRAM specifically for it
        kwargs["integrated_sram_size"] = 0

        # SoCCore -----------------------------------------_----------------------------------------
        SoCCore.__init__(self, platform, sys_clk_freq,
            ident          = "LiteX SoC on Crosslink-NX VIP Input Board",
            ident_version  = True,
            **kwargs)

        # CRG --------------------------------------------------------------------------------------
        self.submodules.crg = _CRG(platform, sys_clk_freq)

        # 128KB LRAM (used as SRAM) ------------------------------------------------------------
        size = 128*kB
        self.submodules.spram = NXLRAM(32, size)
        self.bus.add_slave("sram", slave=self.spram.bus, region=SoCRegion(origin=self.mem_map["sram"],
                size=size))
        # Use HyperRAM generic PHY as main ram -----------------------------------------------------
        size = 8*1024*kB
        hr_pads = platform.request("hyperram", 0)
        self.submodules.hyperram = HyperRAM(hr_pads)
        self.bus.add_slave("main_ram", slave=self.hyperram.bus, region=SoCRegion(origin=self.mem_map["main_ram"],
                size=size, mode="rwx"))
        # Leds -------------------------------------------------------------------------------------
        self.submodules.leds = LedChaser(
            pads         = Cat(*[platform.request("user_led", i) for i in range(4)]),
            sys_clk_freq = sys_clk_freq)
        self.add_csr("leds")

        refclk = platform.request("clk27_0")
        cam_mclk = platform.request("camera_mclk", 0)
        self.comb += cam_mclk.eq(refclk)

        self.submodules.i2c = I2CMaster(platform.request("i2c", 0))
        self.add_csr("i2c")

        self.submodules.lcd_spi = SPIMaster(platform.request("lcd_spi", 0),
            data_width=16, sys_clk_freq=sys_clk_freq, spi_clk_freq=4000000, mode="aligned")
        self.add_csr("lcd_spi")

        self.submodules.lcd_gpio = GPIOOut(platform.request("lcd_gpio", 0))
        self.add_csr("lcd_gpio")

        self.submodules.dphy = DPHY_CSIRX_CIL(
            pads        = platform.request("camera", 0),
            num_lanes   = 4,
            clk_mode    = "ENABLED",
            deskew      = "DISABLED",
            gearing     = 8,
            loc         = "TDPHY_CORE2",
        )

        self.comb += [
            self.dphy.sync_clk.eq(ClockSignal()),
            self.dphy.sync_rst.eq(ResetSignal()),
            self.dphy.pd_dphy.eq(0),
            self.dphy.hs_rx_en.eq(1),
        ]

        self.submodules.clk_byte_freq = FreqMeter(period=sys_clk_freq, clk=self.dphy.clk_byte)
        self.add_csr("clk_byte_freq")
        self.submodules.hs_rx_data = GPIOIn(pads=self.dphy.hs_rx_data)
        self.add_csr("hs_rx_data")
        self.submodules.hs_rx_sync = GPIOIn(pads=self.dphy.hs_rx_sync)
        self.add_csr("hs_rx_sync")

        self.clock_domains.cd_mipi = ClockDomain()
        self.comb += self.cd_mipi.clk.eq(self.dphy.clk_byte)
        dphy_header = Signal(32)
        for i in range(0, 4):
            prev_sync = Signal()
            self.sync.mipi += prev_sync.eq(self.dphy.hs_rx_sync[i])
            self.sync.mipi += If(prev_sync, dphy_header[(8*i):(8*(i+1))].eq(self.dphy.hs_rx_data[(8*i):(8*(i+1))]))
        self.submodules.dphy_header = GPIOIn(pads=dphy_header)
        self.add_csr("dphy_header")

        wa = WordAligner(lane_width=8, num_lanes=4, depth=3)
#        swapped_data = Cat(self.dphy.hs_rx_data[24:32], self.dphy.hs_rx_data[8:16], self.dphy.hs_rx_data[16:24], self.dphy.hs_rx_data[0:8])
        self.sync.mipi += [
            wa.data_in.eq(self.dphy.hs_rx_data),
            wa.sync_in.eq(self.dphy.hs_rx_sync)
        ]
        self.submodules.wa = wa

        packet_cap = PacketCapture(data=wa.data_out, data_sync=wa.sync_out, depth=1024)
        self.submodules.packet_cap = packet_cap
        packet_io = wishbone.SRAM(self.packet_cap.mem, read_only=True)
        self.submodules.packet_io = packet_io
        self.bus.add_slave("packet_io", slave=packet_io.bus, region=SoCRegion(origin=0xb0000000, size=0x4000, mode="rw", cached=False))

        image_cap = ImageCapture(data=wa.data_out, data_sync=wa.sync_out, subsample_x=5, subsample_y=9, out_width=96, out_height=108)
        self.submodules.image_cap = image_cap
        image_io = wishbone.SRAM(self.image_cap.mem, read_only=True)
        self.submodules.image_io = image_io
        self.bus.add_slave("image_io", slave=image_io.bus, region=SoCRegion(origin=0xb0010000, size=0x10000, mode="rw", cached=False))

# Build --------------------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="LiteX SoC on Crosslink-NX VIP Board")
    parser.add_argument("--build",         action="store_true", help="Build bitstream")
    parser.add_argument("--load",          action="store_true", help="Load bitstream")
    parser.add_argument("--toolchain",     default="radiant",   help="FPGA toolchain: radiant (default) or prjoxide")
    parser.add_argument("--sys-clk-freq",  default=75e6,        help="System clock frequency (default: 75MHz)")
    parser.add_argument("--with-hyperram", default="none",      help="Enable use of HyperRAM chip: none (default), 0 or 1")
    parser.add_argument("--prog-target",   default="direct",    help="Programming Target: direct (default) or flash")
    builder_args(parser)
    oxide_args(parser)
    args = parser.parse_args()

    soc = BaseSoC(
        sys_clk_freq = int(float(args.sys_clk_freq)),
        hyperram     = args.with_hyperram,
        toolchain    = args.toolchain,
        cpu_type     = "picorv32",
        cpu_variant  = "minimal",
        integrated_rom_size = 32768,
    )
    builder = Builder(soc, **builder_argdict(args))
    builder_kargs = oxide_argdict(args) if args.toolchain == "oxide" else {}
    builder.build(**builder_kargs, run=args.build)

    if args.load:
        prog = soc.platform.create_programmer(args.prog_target)
        prog.load_bitstream(os.path.join(builder.gateware_dir, soc.build_name + ".bit"))

if __name__ == "__main__":
    main()
