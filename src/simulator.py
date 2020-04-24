from tkinter import *
import sys

class Router:
	def __init__(self, id, x, y):
		self.id = id
		self.x = x
		self.cx = x + router_width/2
		self.y = y
		self.cy = y + router_height/2

	def draw(self):
		canvas.create_rectangle(self.x, self.y, self.x+router_width, self.y+router_height, width=2, fill="sky blue")
		canvas.create_text(self.cx, self.cy, text=str(self.id), fill="black", font="Helvetica 12 bold")

class Processor:
	def __init__(self, id, x, y):
		self.id = id
		self.x = x
		self.cx = x + processor_width/2
		self.y = y
		self.cy = y + processor_height/2

	def draw(self):
		canvas.create_oval(self.x, self.y, self.x+processor_width, self.y+processor_height, width=2, fill="light salmon")
		canvas.create_text(self.cx, self.cy, text=str(self.id), fill="black", font="Helvetica 12 bold")

class Channel:
	def __init__(self, node_A, node_B):
		self.node_A = node_A
		self.node_B = node_B

	def draw(self):
		canvas.create_line(self.node_A.cx, self.node_A.cy, self.node_B.cx, self.node_B.cy, fill="red", width=2)

class Simulator:
	def __init__(self):
		self.num_routers = 0
		self.num_processors = 0
		self.routers = []
		self.processors = []
		self.channels = []
		self.router_to_router_channels = []
		self.router_to_processor_channels = []

	def get_cpp_output(self):
		lines = []
		for line in sys.stdin:
			lines.append(line)
		return lines

	def extract_num_nodes(self, cpp_output):
		self.num_routers = 0
		self.num_processor = 0
		for line in cpp_output:
			items = line.split()
			if items[0] == "NUM_ROUTERS":
				self.num_routers = int(items[1])
			if items[0] == "NUM_PROCESSORS":
				self.num_processors = int(items[1])
			if self.num_routers != 0 and self.num_processors != 0:
				break
		self.nodes_per_row = (self.num_processors**0.5)
		self.x_space = inner_canvas_width / self.nodes_per_row
		self.y_space = inner_canvas_width / self.nodes_per_row	

	def create_routers(self, cpp_output):
		self.routers = []
		self.router_to_router_channels = []
		curr_line = 0
		while(curr_line < len(cpp_output)):
			items = cpp_output[curr_line].split()
			if items[0] == "ROUTER":
				router_id = int(items[1])
				router_x = inner_canvas_x_start + (router_id % self.nodes_per_row)*self.x_space
				router_y = inner_canvas_y_start + (router_id//self.nodes_per_row)*self.x_space
				self.routers.append(Router(router_id, router_x, router_y))
				curr_line += 1
				next_items = cpp_output[curr_line].split()
				assert(next_items[0] == "ROUTER_CONNECTIONS")
				for neighbor in next_items[1].split(','):
					if neighbor == '': break
					neighbor_id = int(neighbor)
					if router_id < neighbor_id:
						self.router_to_router_channels.append((router_id, neighbor_id))
			curr_line += 1

	def create_processors(self, cpp_output):
		self.processors = []
		self.router_to_processor_channels = []
		curr_line = 0
		while(curr_line < len(cpp_output)):
			items = cpp_output[curr_line].split()
			if items[0] == "PROCESSOR":
				processor_id = int(items[1])
				curr_line += 1
				next_items = cpp_output[curr_line].split()
				assert(next_items[0] == "ROUTER_CONNECTIONS")
				for router in next_items[1].split(','):
					if router == '': break
					router_id = int(router)
					router_x = self.routers[router_id].x
					router_y = self.routers[router_id].y
					processor_x = router_x - 30
					processor_y = router_y - 30
					self.router_to_processor_channels.append((router_id, processor_id))
				self.processors.append(Processor(processor_id, processor_x, processor_y))
			curr_line += 1

	def create_channels(self):
		self.channels = []
		for pair in self.router_to_router_channels:
			self.channels.append(Channel(self.routers[pair[0]], self.routers[pair[1]]))
		for pair in self.router_to_processor_channels:
			self.channels.append(Channel(self.routers[pair[0]], self.processors[pair[1]]))

	def simulate(self):
		cpp_output = self.get_cpp_output()
		self.extract_num_nodes(cpp_output)
		self.create_routers(cpp_output)
		self.create_processors(cpp_output)
		self.create_channels()

		for channel in self.channels:
			channel.draw()
		for router in self.routers:
			router.draw()
		for processor in self.processors:
			processor.draw()

if __name__ == '__main__':
	router_width = 30
	router_height = 30
	processor_width = 10
	processor_height = 20

	canvas_width = 600
	canvas_height = 600
	inner_canvas_width = canvas_width - 2*router_width
	inner_canvas_height = canvas_height - 2*router_height
	inner_canvas_x_start = router_width
	inner_canvas_y_start = router_height

	display = Tk()
	frame = Frame(display)
	frame.pack(fill=BOTH)
	canvas = Canvas(frame, width = canvas_width, height = canvas_height)
	canvas.pack(fill=BOTH)

	sim = Simulator()
	sim.simulate()

	display.mainloop()