from pathlib import Path



class ProtoParser():
	def __init__(self, start_id, recv_prefix, send_prefix):
		self.recv_pkt = []
		self.send_pkt = []
		self.total_pkt = []
		self.start_id = start_id
		self.id = start_id
		self.recv_prefix = recv_prefix
		self.send_prefix = send_prefix

	def parse_proto(self, path):
		# 기존 CP949 proto와 UTF-8 proto를 모두 읽고 파일 핸들은 즉시 닫는다.
		data = Path(path).read_bytes()
		try:
			text = data.decode('utf-8-sig')
		except UnicodeDecodeError:
			text = data.decode('cp949')
		lines = text.splitlines()

		for line in lines:
			if line.startswith('message') == False:
				continue

			pkt_name = line.split()[1].upper()
			if pkt_name.startswith(self.recv_prefix):
				self.recv_pkt.append(Packet(pkt_name, self.id))
			elif pkt_name.startswith(self.send_prefix):
				self.send_pkt.append(Packet(pkt_name, self.id))
			else:
				continue

			self.total_pkt.append(Packet(pkt_name, self.id))
			self.id += 1


class Packet:
	def __init__(self, name, id):
		self.name = name
		self.id = id