# The higher level parts of a MIPI CSI-2 receiver; non arch specific

from migen import *
from litex.soc.interconnect import wishbone

class WordAligner(Module):
	def __init__(self, lane_width=8, num_lanes=4, depth=3):
		data_width = lane_width * num_lanes
		self.data_in = Signal(data_width)
		self.sync_in = Signal(num_lanes)
		self.data_out = Signal(data_width)
		self.sync_out = Signal()
		data_shift = Array([Signal(data_width) for i in range(depth)])
		sync_shift = Array([Signal(num_lanes) for i in range(depth)])
		pointers = [Signal(max=depth) for i in range(num_lanes)]
		# shift registers
		self.sync.mipi += data_shift[0].eq(self.data_in)
		self.sync.mipi += sync_shift[0].eq(self.sync_in)
		for i in range(depth-1):
			self.sync.mipi += data_shift[i+1].eq(data_shift[i])
			self.sync.mipi += sync_shift[i+1].eq(sync_shift[i])
		# update pointers when trigger is met (any of sync[depth-2] set)
		trigger = Signal()
		self.comb += trigger.eq(sync_shift[depth - 2] != 0)
		for i in range(num_lanes):
			for j in range(depth):
				self.sync.mipi += If(trigger & sync_shift[j][i], pointers[i].eq(j+1))
		# output based on pointers
		# delay sync by one cycle
		delayed_sync = Signal()
		self.sync.mipi += delayed_sync.eq(1)
		for i in range(num_lanes):
			self.sync.mipi += self.data_out[(i*lane_width):((i+1)*lane_width)].eq(data_shift[pointers[i]][(i*lane_width):((i+1)*lane_width)])
			self.sync.mipi += If(~sync_shift[pointers[i]][i], delayed_sync.eq(0))
		self.sync.mipi += self.sync_out.eq(delayed_sync)

class PacketCapture(Module):
	def __init__(self,  data, data_sync, depth=128):
		self.specials.mem = Memory(len(data), depth)
		port = self.mem.get_port(write_capable=True, clock_domain="mipi")
		self.specials += port
		write_ptr = Signal(32)
		self.sync.mipi += [
			If(data_sync, write_ptr.eq(0)).Else(write_ptr.eq(write_ptr + 1)),
			port.dat_w.eq(data)
		]
		self.comb += [
			port.adr.eq(write_ptr),
			port.we.eq(write_ptr < depth)
		]
