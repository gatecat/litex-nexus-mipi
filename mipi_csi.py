# The higher level parts of a MIPI CSI-2 receiver; non arch specific

from migen import *
class WordAligner(Module):
	def __init__(lane_width=8, num_lanes=4, depth=3):
		data_width = lane_width * num_lanes
		self.data_in = Signal(data_width)
		self.sync_in = Signal(num_lanes)
		self.data_out = Signal(data_width)
		self.sync_out = Signal()
		data_shift = Array([Signal(data_width) for i in depth])
		sync_shift = Array([Signal(num_lanes) for i in depth])
		pointers = [Signal(max=depth) for i in num_lanes]
		# shift registers
		self.sync += data_shift[0].eq(data_in)
		self.sync += sync_shift[0].eq(sync_in)
		for i in range(depth-1):
			self.sync += data_shift[i+1].eq(data_shift[i])
			self.sync += sync_shift[i+1].eq(sync_shift[i])
		# update pointers when trigger is met (any of sync[depth-2] set)
		trigger = Signal()
		self.comb += trigger.eq(sync_shift[depth - 2] != 0)
		for i in range(num_lanes):
			for j in range(depth):
				self.sync += If(trigger & sync_shift[j][i], pointers[i].eq(j+1))
		# output based on pointers
		self.sync += self.sync_out.eq(1)
		for i in range(num_lanes):
			self.sync += self.data_out[(i*lane_width):((i+1)*lane_width)].eq(data_shift[pointers[i]][(i*lane_width):((i+1)*lane_width)])
			self.sync += If(~sync_shift[pointers[i]][i], self.sync_out.eq(0))

