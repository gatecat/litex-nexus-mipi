from migen import *

# MIPI DPHY core; configured as MIPI CSI-2 receiver with control and interface logic
class DPHY_CSIRX_CIL(Module):
    def __init__(self, pads, num_lanes=4, clk_mode="ENABLED", deskew="DISABLED", gearing=8, loc="TDPHY_CORE2"):
        data_width = num_lanes * gearing
        self.sync_clk = Signal()
        self.sync_rst = Signal()
        self.pd_dphy = Signal()
        # TODO: LMMI (should create a LiteX peripheral)
        self.hs_rx_en = Signal()
        self.hs_rx_data = Signal(data_width)
        self.hs_rx_sync = Signal(num_lanes)
        self.clk_byte = Signal()
        self.ready = Signal()

        num_lanes_dict = {
            1: "ONE_LANE",
            2: "TWO_LANES",
            3: "THREE_LANES",
            4: "FOUR_LANES"
        }

        gear_dict = {
            8: "0b00",
            16: "0b01",
            32: "0b10",
            64: "0b11"
        }

        tx_esc_clk = Signal(reset=0)
        self.clock_domains.cd_sync_clk = ClockDomain()
        self.comb += self.cd_sync_clk.clk.eq(self.sync_clk)
        self.sync.sync_clk += tx_esc_clk.eq(~tx_esc_clk)

        int_data = [Signal(16) for _ in range(4)]
        int_sync = [Signal(4) for _ in range(4)]

        conns = dict(
            p_GSR="ENABLED",
            p_AUTO_PD_EN="POWERED_UP",
            p_CFG_NUM_LANES=num_lanes_dict[num_lanes],
            p_CM="0b00000000",
            p_CN="0b00000",
            p_CO="0b000",
            p_CONT_CLK_MODE=clk_mode,
            p_DESKEW_EN=deskew,
            p_DSI_CSI="CSI2_APP",
            p_EN_CIL="CIL_ENABLED",
            p_HSEL="DISABLED",
            p_LANE0_SEL="LANE_0",
            p_LOCK_BYP="GATE_TXBYTECLKHS",
            p_MASTER_SLAVE="SLAVE",
            p_PLLCLKBYPASS="BYPASSED",
            p_RSEL="0b00",
            p_RXCDRP="0b01",
            p_RXDATAWIDTHHS=gear_dict[gearing],
            p_RXLPRP="0b001",
            p_TEST_ENBL="0b000000",
            p_TEST_PATTERN="0b00000000000000000000000000000000",
            p_TST="0b0000",
            p_TXDATAWIDTHHS="0b00",
            p_U_PRG_HS_PREPARE="0b00",
            p_U_PRG_HS_TRAIL="0b000000",
            p_U_PRG_HS_ZERO="0b000000",
            p_U_PRG_RXHS_SETTLE="0b000011",
            p_UC_PRG_RXHS_SETTLE="0b000111",
            i_LMMIRESET_N=1,
            i_BITCKEXT=1,
            i_CLKREF=1,
            i_PDDPHY=self.pd_dphy,
            i_PDPLL=1,
            i_UTXDHS=0xFFFFFFFF,
            i_U1TXDHS=0xFFFFFFFF,
            i_U2TXDHS=0xFFFFFFFF,
            i_U3TXDHS=0xFFFFFFFF,
            i_UCENCK=~self.pd_dphy,
            i_UED0THEN=~self.pd_dphy,
            i_U1ENTHEN=~self.pd_dphy,
            i_U2END2=~self.pd_dphy,
            i_U3END3=~self.pd_dphy,
            i_URXCKINE=self.sync_clk,
            i_UTXCKE=tx_esc_clk,
            i_SCCLKIN=1,
            o_URXDHS=int_data[0],
            o_U1RXDHS=int_data[1],
            o_U2RXDHS=int_data[2],
            o_U3RXDHS=int_data[3],
            o_URXSHS=int_sync[0],
            o_U1RXSHS=int_sync[1],
            o_U2RXSHS=int_sync[2],
            o_U3RXSHS=int_sync[3],
            o_URWDCKHS=self.clk_byte,
            o_LMMIREADY=self.ready,
        )

        if pads is not None:
            conns["io_CKN"]=pads.clkn
            conns["io_CKP"]=pads.clkp

        for i in range(num_lanes):
            if pads is not None:
                conns["io_DP{}".format(i)] = pads.dp[i]
                conns["io_DN{}".format(i)] = pads.dn[i]
            self.comb += self.hs_rx_data[i*gearing:(i+1)*gearing].eq(int_data[i][0:gearing])
            self.comb += self.hs_rx_sync[i].eq(int_sync[i][0] | int_sync[i][1] | int_sync[i][2] | int_sync[i][3])
        # connect all these ports to constant 0
        const0_ports = ['UCTXREQH', 'UTRD0SEN', 'U1TXREQH', 'U2TXREQH', 'U3TXREQH', 'UTDIS', 'U1TDIS', 'U2TDIS',
            'U3TDISD2', 'UTRNREQ', 'U1TREQ', 'U2TREQ', 'U3TREQD2', 'UTXMDTX', 'U1FTXST', 'U2FTXST','U3FTXST',
            'UFRXMODE', 'U1FRXMD', 'U2FRXMD', 'U3FRXMD', 'UCTXUPSC', 'UTXULPSE', 'U1TXUPSE', 'U2TXUPSE', 'U3TXULPS',
            'UTXENER', 'U1TXLPD', 'U2TPDTE', 'U3TXLPDT', 'UDE0D0TN', 'UDE1D1TN', 'UDE2D2TN', 'UDE3D3TN', 'UDE4CKTN',
            'UDE5D0RN', 'UDE6D1RN', 'UDE7D2RN', 'U1TDE0D3', 'U1TDE1CK', 'U1TDE2D0', 'U1TDE3D1', 'U1TDE4D2',
            'U1TDE5D3', 'U1TDE6', 'U1TDE7', 'U2TDE0D0', 'U2TDE1D1', 'U2TDE2D2', 'U2TDE3D3', 'U2TDE4CK', 'U2TDE5D0',
            'U2TDE6D1', 'U2TDE7D2', 'U3TDE0D3', 'U3TDE1D0', 'U3TDE2D1', 'U3TDE3D2', 'U3TDE4D3', 'U3TDE5CK', 
            'U3TDE6', 'U3TDE7', 'UTXTGE0', 'UTXTGE1', 'UTXTGE2', 'UTXTGE3', 'U1TXTGE0', 'U1TXTGE1', 'U1TXTGE2',
            'U1TXTGE3', 'U2TXTGE0', 'U2TXTGE1', 'U2TXTGE2', 'U2TXTGE3', 'U3TXTGE0', 'U3TXTGE1', 'U3TXTGE2',
            'U3TXTGE3', 'UCTXUPSX', 'UTXUPSEX', 'U1TXUPSX', 'U2TXUPSX', 'U3TXUPSX', 'UTXSKD0N', 'U1TXSK',
            'U2TXSKC', 'U3TXSKC', 'UTXRD0EN', 'U1TXREQ', 'U2TXREQ', 'U3TXREQ', 'UTXVDE', 'U1TXVDE', 'U2TXVDE',
            'U3TXVD3', 'LTSTEN', 'LTSTLANE', 'LMMICLK', 
        ]
        for p in const0_ports:
            conns["i_{}".format(p)] = 0
        self.specials.dphy = Instance("DPHY", **conns)
        self.dphy.attr.add(("LOC", loc))
