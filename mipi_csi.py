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

class ImageCapture(Module):
	def __init__(self,  data, data_sync, subsample_x=5, subsample_y=20, out_width=120, out_height=54):
		is_pixels = Signal()
		subx_ctr = Signal(max=subsample_x)
		suby_ctr = Signal(max=subsample_y)
		out_x = Signal(16)
		out_y = Signal(16)
		x_hit = Signal()
		y_hit = Signal()
		self.specials.mem = Memory(16, out_width * out_height)
		port = self.mem.get_port(write_capable=True, clock_domain="mipi")
		self.specials += port
		data_type = data[24:32]
		self.sync.mipi += [
			x_hit.eq(subx_ctr == 0),
			y_hit.eq(suby_ctr == 0),
			If(x_hit,
				out_x.eq(out_x + 1)
			),
			If(subx_ctr == subsample_x - 1,
				subx_ctr.eq(0)
			).Else(
				subx_ctr.eq(subx_ctr + 1)
			),
			If(data_sync,
				If(data_type == 0x2B, # RAW10 pixel data
					If(suby_ctr == subsample_y - 1,
						suby_ctr.eq(0),
						out_y.eq(out_y + 1)
					).Else(
						suby_ctr.eq(suby_ctr + 1)
					),
					out_x.eq(0),
					subx_ctr.eq(0),
					is_pixels.eq(1),
				).Else(
					is_pixels.eq(0),
					If(data_type == 0x00, # frame start
						suby_ctr.eq(0),
						out_y.eq(0xFFFF),
					)
				)
			),
			port.adr.eq(out_y * out_width + out_x),
			port.we.eq(is_pixels & x_hit & y_hit & (out_y < out_height) & (out_x < out_width)),
			port.dat_w.eq(data[0:16]) # we aren't doing any RAW10 decoding; assume subsample_x is a multiple of 5
		]
